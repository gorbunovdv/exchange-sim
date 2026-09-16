# Architecture

This document describes the architecture of current project.

## Abstract Pipeline

The exchange is a pipeline of three stages connected by typed message streams:

- `OrderGeneratorWrapper` sends `request::RequestType` to `MatchingEngine`
- `MatchingEngine` sends `engine_events::EventType` to `ExchangeDispatcher`
- `ExchangeDispatcher` sends `market_events::EventType` and `report::ReportType` to `OrderGeneratorWrapper`

Every message in these streams is `std::variant<PODTypes...>`. A stage is a class that satisfies `ProcessorOf` concept. 
Each stage emits objects into sink that satisfies `SinkOf`. All concurrency or absence of it is achieved 
using specialized wrappers, processors do not know anything about the specific setup. The same set of classes could
yield a completely different design.

Wiring utilities include:

- `CallbackSink` - `push` to the sink calls a stored `std::function` straight ahead.
- `CallbackReceiver` is the sink itself which calls `BaseClass::process` on the stored `BaseClass`. A push to this `Sink` immediately invokes the `Processor`
- `ForwardSink` - `push` forwards the object through to a different `Sink` by pointer.
- `ThreadReceiver<EventType, ProcessorType>` - constructs `Processor`, `SPSCQueue` and `std::jthread` pinned to its own leased core. 
This thread constantly pops objects from `SPSCQueue` and feeds them to `Processor`.

Those are three designs that implement the same stage graph:

| Design           | Transport                                                                                                           |
|------------------|---------------------------------------------------------------------------------------------------------------------|
| `callback`       | `CallbackReceiver`/`CallbackSink`s are used, so all events are passed to next stage immediately                     |
| `event_loop`     | `WrapVector` is used as `Sink`, so each next stage is fed by all events that were pushed by previous stage in batch |
| `multi_threaded` | `ThreadReceiver` is used, so each stage is placed on separate thread pinned to separate core                        |

## Pipeline

### Client side

`OrderGenerator` decides what order it should send to the exchange. It sends a stream of Add/Replace/Cancel. `OrderGeneratorWrapper` provides 
`OrderGenerator` with `GeneratorInterface` to provide a convenient interface to send orders to the exchange. Several skills can be connected to
`GeneratorInterface` to maintain the correct order state using `OrderStorage`, for example, or to maintain counters for pending transactions/active orders.

### Matching

`MatchingEngine` validates that `security_id` is valid and security of request exists. Next, it routes the request to that security's
`L3Book`. It also owns the global order and trade id counters.

`L3Book` implements the whole matching logic: `RiskChecker` checks requests validity (max amount, lot size, price tick), while `L3Book` checks duplicate
`{client_id, client_order_id}` and implements FIFO price-time matching logic against the opposite book half and possible resting order logic.

Book storage itself is behind the `BookContainer` concept, `StlL3BookContainer` is the reference implementation.

### Output

`ExchangeDispatcher` converts internal engine events emitted by `L3Book`/`MatchingEngine` into two streams: market data events and reports.

### Connectors

The exact implementation of each stage is separated from the pipeline. There are three pipeline designs available: `multithreaded`, `event_loop` and `callback`.

It is possible to wire any implementations of each stage to any pipeline design. `run_multi_threaded_app` is a templated function, so it's possible to use any OrderGenerator, 
any MatchingEngine and any EventDispatcher with it.

### OrderGenerator

Although it's possible to wire any implementation of `OrderGenerator`, it's more convenient to use implemented `OrderGeneratorWrapper` along with `GeneratorInterface`.

GeneratorInterface is a simple interface that allows simplifying the code by providing a common interface for different OrderGenerator implementations. This common
interface allows not worrying about ClientOrderID sent by the OrderGenerator. It also allows connecting different Skills to it. Current available skills are `DebugOrders` 
(which prints all requests, market data and reports at DEBUG level), `StoreOrders` (which maintains a list of currently active orders), `MaintainBook` (which maintains a full L3 book) and
`PendingTransactions` (which maintains the number of pending transactions).
Those skills are implemented in `exchange_sim::generator_interface_skills` and could be used via templates.

`OrderGeneratorWrapper` itself is a wrapper that links `GeneratorInterface` with `OrderGenerator`. It also maintains journaling of all requests, market data and reports for future
evaluation and benchmarking.

### MatchingEngine

`MatchingEngine` along with `L3Book` are actually responsible for matching orders and maintaining the order book. They are templated, so any
underlying containers could be used with them. Currently, the only implemented container is `StlL3BookContainer` which uses `std::map<Price, std::list<Order>>` as underlying data
structure.
