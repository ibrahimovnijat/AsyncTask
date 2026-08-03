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
./run.sh debug    # starts and waits for instructions. Pass debug | release during start. Default is debug if you don't specify.
```

The program reads commands from standard input:

| Command                | Effect                                                        |
| ---------------------- | ------------------------------------------------------------- |
| `start`                | Start a task of the default type (`count`) and print its ID   |
| `start <task_type_id>` | Start a task of the given type (`count` or `primes`)          |
| `pause <task_id>`      | Pause a running task                                          |
| `resume <task_id>`     | Resume a paused task                                          |
| `stop <task_id>`       | Stop a running or paused task                                 |
| `status`               | List ID, type, status and progress of every task              |
| `status <task_id>`     | As above, for a single task                                   |
| `help`                 | Print the help message                                        |
| `quit`                 | Stop all tasks and shut down gracefully (Ctrl-D works too)    |

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