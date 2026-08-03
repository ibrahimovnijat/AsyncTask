# tasklib

A small C++23 library for managing an arbitrary number of asynchronous tasks.
Tasks can be started, safely paused, resumed and stopped, and their status and
progress can be queried at any time. An example command-line application
(`asynctask_cli`) demonstrates one possible use of the library.

The library has no dependencies beyond the C++ standard library. GoogleTest is
used for the unit tests only.

## Requirements

- A C++23 compiler. Tested with GCC 14.2 on Ubuntu 24.04;
- CMake 3.22 or newer, plus any generator it supports (Make, Ninja).
- POSIX platform (Linux, macOS).
- For the tests: GoogleTest. A system installation is used if found; otherwise
  a pinned release is downloaded automatically at configure time (requires
  network access on the first configure).

## Building

```sh
cd scripts
./build.sh debug|release    # Default is debug if you don't specify preset
```

## Running the example application

```sh
cd scripts
./run.sh debug|release    # Starts and waits for instructions. Default is debug if you don't specify preset.
```

Run the tests:

```sh
cd scripts
./unit_tests.sh
```


The program reads commands from standard input:

| Command                | Effect                                                        |
| ---------------------- | ------------------------------------------------------------- |
| `start`                | Start a task of the default type (`count`) and print its ID   |
| `start <task_type_id>` | Start a task of the given type (see the table below)          |
| `pause <task_id>`      | Pause a running task                                          |
| `resume <task_id>`     | Resume a paused task                                          |
| `stop <task_id>`       | Stop a running or paused task                                 |
| `status`               | List ID, type, status and progress of every task              |
| `status <task_id>`     | As above, for a single task                                   |
| `help`                 | Print the help message                                        |
| `quit`                 | Stop all tasks and shut down gracefully (Ctrl-D works too)    |

The example application ships four task types, each showing a different way for a task
to cooperate with the library (`help` lists them at runtime):

| Task type  | Shape     | What it demonstrates                                              |
| ---------- | --------- | ----------------------------------------------------------------- |
| `count`    | wait-bound| 100 steps of 50 ms; checkpoints every step                        |
| `primes`   | CPU-bound | trial division below 2'000'000; checkpoints every 1024 candidates |
| `download` | I/O-bound | 8 MiB in 64 KiB chunks; waits interruptibly on `stop_token()`     |
| `fail`     | failing   | throws halfway; ends as `Failed` with the message in `status`     |

Example session:

```
> start
Started task 1 (count)
> start primes
Started task 2 (primes)
> pause 1
Task 1 paused
> status
  ID  TYPE      STATUS      PROGRESS
   1  count     paused           23%
   2  primes    running          61%
> resume 1
Task 1 resumed
> quit
Shutting down, stopping remaining tasks...
```

## Using the library

```cpp
#include <tasklib/taskManager.hpp>

tasklib::TaskManager manager;

auto id = manager.start([](tasklib::TaskContext& ctx) {
    constexpr int steps = 1000;
    for (int i = 0; i < steps; ++i) {
        if (!ctx.checkpoint())      // blocks while paused; false once stopped
            return;
        do_one_unit_of_work(i);
        ctx.set_progress(double(i + 1) / steps);
    }
}, "my-task-type");

manager.pause(id);                   // std::expected<void, TaskError>
manager.resume(id);
tasklib::TaskInfo info = *manager.status(id);
manager.stop(id);
```