#include <tasklib/taskManager.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <latch>
#include <memory>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;
using tasklib::TaskError;
using tasklib::TaskManager;
using tasklib::TaskStatus;

namespace {

/// Polls `pred` until it holds or `timeout` elapses. Used only for conditions
/// that are guaranteed to become true; the generous timeout is a safety net
/// against hanging the test suite, not a tuning knob.
template <typename Pred>
[[nodiscard]] bool eventually(Pred&& pred, std::chrono::milliseconds timeout = 5s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pred())
            return true;
        std::this_thread::sleep_for(1ms);
    }
    return pred();
}

/// State shared with a probe task, used to observe its execution
/// deterministically (no reliance on timing for correctness).
struct ProbeState {
    std::atomic<long> iterations{0};
    std::latch started{1}; ///< Counted down on the task's first iteration.
    std::latch exited{1};  ///< Counted down when the task function returns.
};

/// A task that loops until stopped, incrementing a counter per iteration.
tasklib::TaskFn make_probe_task(std::shared_ptr<ProbeState> state) {
    return [state](tasklib::TaskContext& ctx) {
        state->started.count_down();
        while (ctx.checkpoint()) {
            state->iterations.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(1ms);
        }
        state->exited.count_down();
    };
}

/// Waits until the probe's iteration counter is stable across an observation
/// window, i.e. the worker has parked at the paused checkpoint. At most one
/// in-flight iteration can land after pause(), so this converges immediately.
void wait_until_parked(const ProbeState& state) {
    ASSERT_TRUE(eventually([&] {
        const long before = state.iterations.load();
        std::this_thread::sleep_for(25ms);
        return state.iterations.load() == before;
    }));
}

} // namespace

TEST(TaskManagerTest, TaskRunsToCompletion) {
    TaskManager manager;
    const auto id = manager.start([](tasklib::TaskContext& ctx) {
        constexpr int steps = 10;
        for (int i = 0; i < steps; ++i) {
            if (!ctx.checkpoint())
                return;
            ctx.set_progress(static_cast<double>(i + 1) / steps);
        }
    });

    const auto final_status = manager.wait(id);
    ASSERT_TRUE(final_status.has_value());
    EXPECT_EQ(*final_status, TaskStatus::Completed);

    const auto info = manager.status(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->status, TaskStatus::Completed);
    EXPECT_DOUBLE_EQ(info->progress, 1.0);
    EXPECT_EQ(info->type, "default");
    EXPECT_TRUE(info->error.empty());
}

TEST(TaskManagerTest, StartAssignsUniqueIdsAndStatusesAreSortedById) {
    TaskManager manager;
    const auto a = manager.start([](tasklib::TaskContext&) {}, "alpha");
    const auto b = manager.start([](tasklib::TaskContext&) {}, "beta");
    const auto c = manager.start([](tasklib::TaskContext&) {}, "gamma");
    EXPECT_LT(a, b);
    EXPECT_LT(b, c);
    EXPECT_EQ(manager.size(), 3u);

    (void)manager.wait(a);
    (void)manager.wait(b);
    (void)manager.wait(c);

    const auto infos = manager.statuses();
    ASSERT_EQ(infos.size(), 3u);
    EXPECT_EQ(infos[0].id, a);
    EXPECT_EQ(infos[1].id, b);
    EXPECT_EQ(infos[2].id, c);
    EXPECT_EQ(infos[0].type, "alpha");
}

TEST(TaskManagerTest, PauseFreezesExecutionAndResumeContinuesIt) {
    TaskManager manager;
    auto state = std::make_shared<ProbeState>();
    const auto id = manager.start(make_probe_task(state), "probe");
    state->started.wait();

    ASSERT_TRUE(manager.pause(id).has_value());
    EXPECT_EQ(manager.status(id)->status, TaskStatus::Paused);

    wait_until_parked(*state);
    const long frozen = state->iterations.load();
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(state->iterations.load(), frozen) << "paused task must make no progress";

    ASSERT_TRUE(manager.resume(id).has_value());
    EXPECT_EQ(manager.status(id)->status, TaskStatus::Running);
    EXPECT_TRUE(eventually([&] { return state->iterations.load() > frozen; }));

    ASSERT_TRUE(manager.stop(id).has_value());
    state->exited.wait();
}

TEST(TaskManagerTest, StopEndsRunningTask) {
    TaskManager manager;
    auto state = std::make_shared<ProbeState>();
    const auto id = manager.start(make_probe_task(state), "probe");
    state->started.wait();

    ASSERT_TRUE(manager.stop(id).has_value());
    state->exited.wait(); // The task function must return promptly.

    const auto final_status = manager.wait(id);
    ASSERT_TRUE(final_status.has_value());
    EXPECT_EQ(*final_status, TaskStatus::Stopped);

    const long after_exit = state->iterations.load();
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(state->iterations.load(), after_exit) << "stopped task must not keep working";
}

TEST(TaskManagerTest, StopWakesAndEndsPausedTask) {
    TaskManager manager;
    auto state = std::make_shared<ProbeState>();
    const auto id = manager.start(make_probe_task(state), "probe");
    state->started.wait();

    ASSERT_TRUE(manager.pause(id).has_value());
    wait_until_parked(*state);

    ASSERT_TRUE(manager.stop(id).has_value());
    state->exited.wait(); // Must be woken from the paused checkpoint.
    EXPECT_EQ(manager.status(id)->status, TaskStatus::Stopped);
}

TEST(TaskManagerTest, InvalidTransitionsAreRejected) {
    TaskManager manager;
    auto state = std::make_shared<ProbeState>();
    const auto id = manager.start(make_probe_task(state), "probe");
    state->started.wait();

    // Running: resume is invalid, pause is not.
    auto result = manager.resume(id);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), TaskError::InvalidTransition);

    ASSERT_TRUE(manager.pause(id).has_value());

    // Paused: a second pause is invalid.
    result = manager.pause(id);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), TaskError::InvalidTransition);

    // Stopped is terminal: no further transitions.
    ASSERT_TRUE(manager.stop(id).has_value());
    state->exited.wait();
    for (auto operation : {&TaskManager::pause, &TaskManager::resume, &TaskManager::stop}) {
        result = (manager.*operation)(id);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), TaskError::InvalidTransition);
    }
}

TEST(TaskManagerTest, CompletedIsTerminal) {
    TaskManager manager;
    const auto id = manager.start([](tasklib::TaskContext&) {});
    ASSERT_EQ(*manager.wait(id), TaskStatus::Completed);

    for (auto operation : {&TaskManager::pause, &TaskManager::resume, &TaskManager::stop}) {
        const auto result = (manager.*operation)(id);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), TaskError::InvalidTransition);
    }
}

TEST(TaskManagerTest, UnknownIdReportsNotFound) {
    TaskManager manager;
    EXPECT_EQ(manager.pause(42).error(), TaskError::NotFound);
    EXPECT_EQ(manager.resume(42).error(), TaskError::NotFound);
    EXPECT_EQ(manager.stop(42).error(), TaskError::NotFound);
    EXPECT_EQ(manager.status(42).error(), TaskError::NotFound);
    EXPECT_EQ(manager.wait(42).error(), TaskError::NotFound);
}

TEST(TaskManagerTest, UncaughtExceptionYieldsFailedStatusWithMessage) {
    TaskManager manager;
    const auto id = manager.start(
        [](tasklib::TaskContext&) { throw std::runtime_error("boom"); }, "faulty");

    ASSERT_EQ(*manager.wait(id), TaskStatus::Failed);
    const auto info = manager.status(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->status, TaskStatus::Failed);
    EXPECT_NE(info->error.find("boom"), std::string::npos);
}

TEST(TaskManagerTest, ProgressIsVisibleWhileRunning) {
    TaskManager manager;
    auto reported = std::make_shared<std::latch>(1);
    auto release = std::make_shared<std::latch>(1);
    const auto id = manager.start([reported, release](tasklib::TaskContext& ctx) {
        ctx.set_progress(0.25);
        reported->count_down();
        release->wait();
    });

    reported->wait();
    auto info = manager.status(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->status, TaskStatus::Running);
    EXPECT_DOUBLE_EQ(info->progress, 0.25);

    release->count_down();
    ASSERT_EQ(*manager.wait(id), TaskStatus::Completed);
    EXPECT_DOUBLE_EQ(manager.status(id)->progress, 1.0);
}

TEST(TaskManagerTest, StopRequestIsObservableViaStopToken) {
    TaskManager manager;
    auto started = std::make_shared<std::latch>(1);
    const auto id = manager.start([started](tasklib::TaskContext& ctx) {
        started->count_down();
        const auto token = ctx.stop_token();
        while (!token.stop_requested())
            std::this_thread::sleep_for(1ms);
    });

    started->wait();
    ASSERT_TRUE(manager.stop(id).has_value());
    EXPECT_EQ(*manager.wait(id), TaskStatus::Stopped);
}

TEST(TaskManagerTest, ManyTasksRunConcurrentlyToCompletion) {
    constexpr int task_count = 32;
    TaskManager manager;
    std::atomic<int> completed{0};

    std::vector<tasklib::TaskId> ids;
    ids.reserve(task_count);
    for (int i = 0; i < task_count; ++i) {
        ids.push_back(manager.start([&completed](tasklib::TaskContext& ctx) {
            if (!ctx.checkpoint())
                return;
            completed.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (const auto id : ids)
        EXPECT_EQ(*manager.wait(id), TaskStatus::Completed);
    EXPECT_EQ(completed.load(), task_count);
    EXPECT_EQ(manager.size(), static_cast<std::size_t>(task_count));
}

TEST(TaskManagerTest, StopAllStopsEveryActiveTask) {
    TaskManager manager;
    auto first = std::make_shared<ProbeState>();
    auto second = std::make_shared<ProbeState>();
    const auto a = manager.start(make_probe_task(first));
    const auto b = manager.start(make_probe_task(second));
    const auto done = manager.start([](tasklib::TaskContext&) {});
    ASSERT_EQ(*manager.wait(done), TaskStatus::Completed);
    first->started.wait();
    second->started.wait();

    ASSERT_TRUE(manager.pause(b).has_value()); // stop_all must handle paused tasks too.
    manager.stop_all();

    first->exited.wait();
    second->exited.wait();
    EXPECT_EQ(manager.status(a)->status, TaskStatus::Stopped);
    EXPECT_EQ(manager.status(b)->status, TaskStatus::Stopped);
    EXPECT_EQ(manager.status(done)->status, TaskStatus::Completed) << "terminal states persist";
}

TEST(TaskManagerTest, DestructorStopsAndJoinsRunningTasks) {
    auto state = std::make_shared<ProbeState>();
    {
        TaskManager manager;
        manager.start(make_probe_task(state));
        state->started.wait();
    } // ~TaskManager must stop the task and join its thread.
    EXPECT_TRUE(state->exited.try_wait());
}
