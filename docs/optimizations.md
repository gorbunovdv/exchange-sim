# Optimizations

This document covers four optimization stories that occurred during the development of this repo. Each story is split
into what was found, what was changed and what was rejected.

## 4K store-load aliasing in the event loop

After the implementation of the correct app evaluation, the results showed that p50 drifted by +-16 ns between identical
runs: roughly 1 run out of 5 was a bit slower.

What was found:

- The drift depended on ASLR: after turning it on, the drift disappeared.
- The introduction of mimalloc and 1 GB huge pages did not fix the issue.
- The `ld_blocks.address_alias` counter was elevated exactly during the bad run.
- `perf report` showed that a significant number of these aliases appeared during the clearing of the four event-loop
  sinks. They are cleared every iteration, so their cursors restart from the vector bases and walk a hot prefix of
  several elements.

What was changed:

- The sinks were moved to the heap with guard padding so that `delta % 4096` stays within `[1344, 4096 - 1344]` for each
  pair.
- The layout is pinned by a `static_assert` on a `constexpr` sum of `sizeof`.

Result:

- `address_alias` per request went down from about 3 to about 1.
- p50 drift went down from +-16 to +-3 ns.

## Memory orders and cached indices in SPSCQueue

The queue between the stages and under the journals used the default `seq_cst` atomics and read the other side's index
on each operation.

What was found:

- On x86 a `seq_cst` store compiles to `xchg`, an implicit full barrier that drains the store buffer, so every push and
  every pop paid a barrier it did not need.
- Every push also loaded the consumer's index, pulling in a cache line the consumer had just written.
- The same disassembly session found a `div` from `% capacity` on every push, that was fixed later.

What was changed:

- `seq_cst` has been replaced with `acquire`/`release` and a `relaxed` load of the own index.
- Cached copies of the opposite index have also been introduced so that the producer and the consumer rarely touch each
  other's cache line.

Result:

- Single-threaded apps gained 4.5-12% on p50, about 20 ns per trade, since the journals push on every event.
- The multi-threaded app gained 31-45% on p50 and about 48% on p99.

## Core hop against pipelining in the multi-threaded app

Initially the multi-threaded app was designed so that each stage was on a separate core. However, it could be beneficial
to place `ExchangeDispatcher` and `MatchingEngine` on one core.

What was found:

- The comparison against master put a price on both sides.
- `first_report` improved by 65-110 ns everywhere: one inter-core hop is worth about 80 ns.
- `last_report` on deep ADDs got worse by 35/61/85 ns as the per-fill slope grew from 237 to 263 ns, because the
  dispatcher no longer converts fill k while the engine runs fill k+1.
- The throughput went down by 4.4%.

What was rejected:

- Placing `ExchangeDispatcher` and `MatchingEngine` on one core. The shipped multi-threaded wiring keeps the dispatcher
  on its own core.

## decimal_fast vs decimal

`Price` and `Amount` are `boost::decimal::decimal64_t` and `boost::decimal::decimal32_t` respectively.

What was found:

- Google Benchmark on `std::map::find` over price levels has put `decimal_fast_64_t` at about 1.5x faster than
  `decimal64_t`.
- However, in the system it was slower: ADD round trips got slower by 11-57 ns.
- The unpacked value is 16 bytes instead of 8 and price/amount live inside each request, event, report and journal
  record, so the larger value is copied through each sink.

What was rejected:

- `decimal_fast`, because the copying costs more than the cheaper compare saves.
