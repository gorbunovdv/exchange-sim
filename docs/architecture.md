# Architecture

This document describes the architecture of current project.

## Abstract Pipeline

The exchange is a pipeline of three stages connected by typed message streams:

- OrderGeneratorWrapper sends request::RequestType to MatchingEngine
- MatchingEngine sends engine_events::EventType to ExchangeDispatcher
- ExchangeDispatcher sends market_events::EventType and report::ReportType to OrderGeneratorWrapper

Every message is `std::variant<PODTypes...>`. A stage is a class that satisfies `ProcessorOf` concept. 
Each stage emits objects into sink that satisfies `SinkOf`. All concurrency or absence of it is achieved 
using specialized wrappers, processors do not know anything about the specific setup. Same set of classes could
yield a completely different design.

Wiring utilities include:

- `CallbackSink` - `push` to the sink calls a stored `std::function` straight ahead.
- `ForwardSink` - `push` forwards the object through to a different `Sink` by pointer.
- `CallbackReceiver<EventType, ProcessorType>` - constructs `Processor` along with a `CallbackSink` connected to it: 
a push to this `Sink` immediately invokes the `Processor`
- `ThreadReceiver<EventType, ProcessorType>` - constructs `Processor`, `SPSCQueue` and `std::jthread` pinned to its own leased core. 
This thread constantly pops objects from `SPSCQueue` and feeds them to `Processor`.

Those are three applications that implement the same stage graph:

| Application                      | Transport                                                                                                           |
|----------------------------------|---------------------------------------------------------------------------------------------------------------------|
| `single_threaded_callback_app`   | `CallbackReceiver`/`CallbackSink`s are used, so all events are passed to next stage immediately                     |
| `single_threaded_event_loop_app` | `WrapVector` is used as `Sink`, so each next stage is fed by all events that were pushed by previous stage in batch |
| `multi_threaded_app`             | `ThreadReceiver` is used, so each stage is placed on separate thread pinned to separate core                        |

## Pipeline

### Client side

SimpleOrderGenerator decides what order it should send to the exchange. It sends a random stream of Add/Replace/Cancel with 
random security, price and amount chosen at random as well. OrderGeneratorWrapper matches SimpleOrderGenerator with GeneratorInterface
to provide convenient interface to send orders to the exchange, maintain correct order state using OrderStorage and maintain counters for
pending transactions/active orders.

### Matching

MatchingEngine validates that security_id is valid and security of request exists. Next, it routes the request to that security's
L3Book. It also owns the global order and trade id counters.

L3Book implements the whole matching logic: RiskChecker checks requests validity (max amount, lot size, price tick), while L3Book checks duplicate
`{client_id, client_order_id}` and implements FIFO price-time matching logic against the opposite book half and possible resting order logic.

Book storage itself is behind the `BookContainer` concept, `StlL3BookContainer` is the reference implementation.

### Output

ExchangeDispatcher converts internal engine events emitted by L3Book/MatchingEngine into two streams: market data events and reports.