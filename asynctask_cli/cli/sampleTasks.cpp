#include "sampleTasks.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
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

constexpr std::array<SampleTaskType, 2> kTaskTypes{{
    {"count", "100 steps of 50 ms each (~5 s); wait-bound", &make_count_task},
    {"primes", "counts primes below 2'000'000 by trial division; CPU-bound", &make_primes_task},
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
