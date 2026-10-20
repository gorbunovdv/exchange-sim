# Testing

## Testing and correctness

`just test` and `just test-from-scratch` build the `clang-debug-asan` preset and run all gtest binaries with ctest.
There are 17 test binaries, the most important ones are:

- `matching_output_test` runs 13 scenarios from `exchange/tests/test_input.h` through `MatchingEngine` and
  `ExchangeDispatcher`. It compares every market data event and every report with the expected ones.
- `maintain_book_skill_test` feeds the same scenarios to `MaintainBook` that rebuilds the L3 book from the L3 market
  data feed only and compares the snapshot with the book of `MatchingEngine`.
- `risk_checker_test` checks every rejection reason for add and replace.
- `matching_engine_test` checks that an unknown security is rejected for add, replace and cancel.
- `order_storage_test` checks every legal transition of the client-side order state machine and that every illegal one
  fails.
- `base` has its own tests for `SPSCQueue`, `Journal` (including torn records and wrong modes), `IndexedMap`,
  `CoreAllocator`, config, logging, TSC and `AllocatorTracking`.

Lots of invariants are checked at runtime with `CHECK` instead of tests. It's never compiled out and the app throws an
exception and terminates on the first error. For instance, `ReplayOrderGenerator` checks that the generated ids match
the replayed ones, the multi-threaded app checks that all queues are indeed drained on shutdown. The apps also terminate
on an allocation on the hot path, a full journal queue, a full SPSC queue or an empty book arena.

The rest is checked at compile time. Every `run_*` function `static_assert`s the components against the concepts, every
queue message must fit into a cache line.

CI runs on every push: `clang-format`, `ruff`, `mypy` and `mdformat` check the code style. The repo is built with
`-Wall -Wextra -Werror -Wpedantic` and interface header set verification, tests are run under the `clang-debug-asan`,
`clang-debug-tsan`, `clang-debug` and `gcc-debug-asan` presets. `clang-tidy` is also run.

Please note that the apps themselves are not run in CI, the benchmarks require an isolated machine.
