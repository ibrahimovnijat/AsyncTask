#include "sampleTasks.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace cli {
using namespace std::chrono_literals;

/**
 * @brief A wait-bound task: 100 steps of 50 ms each (~5 s total). Checkpoints every step,
 * so pause/stop take effect within one step.
 */
tasklib::TaskFn make_count_task() {
    return [](tasklib::TaskContext &ctx) {
        constexpr int steps = 100;
        for (int i = 0; i < steps; ++i) {
            if (!ctx.checkpoint())
                return;
            std::this_thread::sleep_for(50ms);
            ctx.set_progress(static_cast<double>(i + 1) / steps);
        }
    };
}

/**
 * @brief Checks whether the number is prime of not.
 */
bool is_prime(std::uint64_t n) {
    if (n < 2)
        return false;
    for (std::uint64_t d = 2; d * d <= n; ++d)
        if (n % d == 0)
            return false;
    return true;
}

/**
 * @brief Published result of the last primes task. Besides making the result observable, the atomic
 * store keeps the compiler from optimising the computation away.
 */
std::atomic<std::uint64_t> g_last_prime_count{0};

/**
 * @brief A CPU-bound task: counts primes below a fixed limit by trial division. Checkpoints every
 * 1024 candidates - frequent enough that pause/stop feel immediate, rare enough that the
 * synchronization cost is negligible.
 */
tasklib::TaskFn make_primes_task() {
    return [](tasklib::TaskContext &ctx) {
        constexpr std::uint64_t limit = 2'000'000;
        std::uint64_t found = 0;
        for (std::uint64_t n = 2; n < limit; ++n) {
            if (n % 1024 == 0) {
                if (!ctx.checkpoint())
                    return;
                ctx.set_progress(static_cast<double>(n) / limit);
            }
            if (is_prime(n))
                ++found;
        }
        g_last_prime_count.store(found, std::memory_order_relaxed);
        ctx.set_progress(1.0);
    };
}

/**
 * @brief An I/O-shaped task: transfers 8 MiB in 64 KiB chunks (~5 s), pretending each chunk costs
 * 40 ms of network wait. The wait is interruptible through ctx.stop_token(), so a stop is observed
 * immediately instead of after the current chunk - the pattern to follow whenever a task blocks on
 * something rather than computing.
 */
tasklib::TaskFn make_download_task() {
    return [](tasklib::TaskContext &ctx) {
        constexpr std::uint64_t chunk_size = 64 * 1024;
        constexpr std::uint64_t total_size = 8 * 1024 * 1024;

        // condition_variable_any is the cancellation-aware flavour: the wait
        // returns as soon as the stop token is signalled. There is nothing to
        // notify it here, so the predicate is only ever true after a stop.
        std::mutex mutex;
        std::condition_variable_any idle;
        std::unique_lock lock(mutex);

        for (std::uint64_t received = 0; received < total_size; received += chunk_size) {
            if (!ctx.checkpoint())
                return;
            idle.wait_for(lock, ctx.stop_token(), 40ms, [&ctx] { return ctx.stop_requested(); });
            ctx.set_progress(static_cast<double>(received + chunk_size) / total_size);
        }
    };
}

/**
 * @brief A task that fails half way through, so the failure path is easy to exercise from the CLI.
 * The exception escaping the task function is caught by the library, which reports
 * TaskStatus::Failed and hands the message back through TaskInfo::error.
 */
tasklib::TaskFn make_failing_task() {
    return [](tasklib::TaskContext &ctx) {
        constexpr int steps = 20;
        for (int i = 0; i < steps; ++i) {
            if (!ctx.checkpoint())
                return;
            std::this_thread::sleep_for(100ms);
            ctx.set_progress(static_cast<double>(i + 1) / steps);
            if (i == steps / 2)
                throw std::runtime_error("simulated failure halfway through");
        }
    };
}

constexpr std::array<SampleTaskType, 4> kTaskTypes{{
    {"count", "100 steps of 50 ms each (~5 s); wait-bound", &make_count_task},
    {"primes", "counts primes below 2'000'000 by trial division; CPU-bound", &make_primes_task},
    {"download", "transfers 8 MiB in 64 KiB chunks (~5 s); interruptible waits",
     &make_download_task},
    {"fail", "throws halfway through (~1 s); ends as Failed with a message", &make_failing_task},
}};

std::span<const SampleTaskType> sample_task_types() {
    return kTaskTypes;
}

const SampleTaskType *find_task_type(std::string_view id) {
    for (const auto &type : kTaskTypes)
        if (type.id == id)
            return &type;
    return nullptr;
}

} // namespace cli
