# Matching

This document describes what the exchange does with a request: validation, risk checks, matching, events emitted and so
on.

## Securities

Securities come from the `securities` section of the config. `SecurityStorage` assigns a unique `SecurityID` to each
security in the config order starting from zero. It checks each security on startup: `tick_size > 0`,
`high_price > low_price`, `(high_price - low_price) % tick_size == 0`, `max_amount > 0`, `max_amount % lot_size == 0`,
symbols are unique and at most 8 characters long. A bad config terminates the application.

Each security belongs to a `section`. Sections are sorted by name and numbered, `SecurityStorage` assigns a unique
`SectionID` to each section. Sections are used only in the multi-threaded design where sharding by section is required.

`Price` is `boost::decimal::decimal64_t`. `Amount` is `boost::decimal::decimal32_t`, both are wrapped in `StrongType`.

### Requests

| Request               | Fields                                                                                                                        |
| --------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `OrderAddRequest`     | `origin_time`, `client_id`, `client_order_id`, `security_id`, `time_in_force`, `order_type`, `price`, `quantity`, `direction` |
| `OrderReplaceRequest` | `origin_time`, `client_id`, `client_order_id`, `client_replacement_id`, `security_id`, `price`, `quantity`, `direction`       |
| `OrderCancelRequest`  | `origin_time`, `client_id`, `client_order_id`, `client_cancellation_id`, `security_id`                                        |

An order is uniquely identified by `client_id`, `client_order_id` and `security_id`. It gets an `ExchangeOrderID`
assigned by the matching engine. `client_replacement_id` is checked only for not colliding with resting orders and
`client_order_id`.

## Processing

`MatchingEngine::process` checks that `security_id` exists (otherwise it emits `OrderRejected` with `SECURITY_NOT_FOUND`
reason), checks that the current engine owns a book for the given `security_id` (necessary only for the multi-threaded
design, a misrouted request is a bug) and calls `L3Book::add/replace/cancel`. The engine owns the `ExchangeOrderID` and
`ExchangeTradeID` counters, so the same ids may be used in other sections as well, they're only unique within a single
section (this applies only to the multi-threaded design, in the single-threaded design there's one `MatchingEngine`).

### Validation

This table describes the validation rules for each request type.

| Step | Check                                                                                           | Reason                            | Applies to           |
| ---- | ----------------------------------------------------------------------------------------------- | --------------------------------- | -------------------- |
| 1    | `security_id` exists                                                                            | `SECURITY_NOT_FOUND`              | add, replace, cancel |
| 2    | `quantity <= max_amount`                                                                        | `MAX_AMOUNT_EXCEEDED`             | add, replace         |
| 3    | `quantity > 0` and `quantity % lot_size == 0`                                                   | `INVALID_QUANTITY`                | add, replace         |
| 4    | `low_price <= price <= high_price`                                                              | `PRICE_OUT_OF_RANGE`              | add, replace         |
| 5    | `(price - low_price) % tick_size == 0`                                                          | `PRICE_NOT_MULTIPLE_OF_TICK_SIZE` | add, replace         |
| 6    | `{client_id, client_order_id}` is not in the book                                               | `DUPLICATE_CLIENT_ORDER_ID`       | add                  |
| 7    | `{client_id, client_replacement_id}` is not in the book, skipped if it equals `client_order_id` | `DUPLICATE_CLIENT_ORDER_ID`       | replace              |
| 8    | `{client_id, client_order_id}` is in the book                                                   | `ORDER_NOT_FOUND`                 | replace, cancel      |

The duplicate check only looks at orders that currently exist in the book, so the id of a filled or cancelled order is
not considered a duplicate.

### Matching

Matching is FIFO price-time priority against the opposite side of the book.

- While the new order has a quantity greater than zero and the best opposite price crosses the new order's price, take
  the best level.
- Fill against the orders of that level in FIFO order. Every fill creates a `Trade` at the resting order's price, then
  either `OrderFilled(PASSIVE_FILL)` if the resting order is done or `OrderReduced` with the remaining quantity.
- What is left of the new order's quantity is placed on the book or the quantity expires if the request is IOC.

The new order is the aggressor in every trade. There is no self-trade prevention.

### Add

`L3Book::add` for a valid request creates: `OrderAccepted`, the matching events (`Trade`, `OrderFilled(PASSIVE_FILL)`,
`OrderReduced`) and one of `OrderBookEntry` (if the new order is on the book), `OrderFilled(AGGRESSIVE_FILL)` (in case
the new order is fully filled) or `OrderExpired(IOC_REMAINDER_EXPIRED)` (in case the new order is IOC and has a
remainder).

### Replace

`L3Book::replace` is basically a cancel and an add in a single request. The request passes the same risk checks as an
add, then the old order is removed from the book and `ReplacementAccepted` is emitted. The new order gets a new
`order_id`, `client_order_id = client_replacement_id` and price, quantity and side from the replacement request. The
quantity is the full new quantity. It is matched like a fresh new `LIMIT` order.

A replaced order always loses its time priority. A rejected replace leaves the original order untouched.

### Cancel

`L3Book::cancel` for a valid request extracts `{client_id, client_order_id}` from the book and emits either
`OrderCancelled` or `CancellationRejected(ORDER_NOT_FOUND)` if there's no such order in the book.

### Terminal report

Each request produces exactly one terminal report.

| Request | Terminal report                                                                                                                    |
| ------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| add     | `OrderRejected`, `OrderBookEntry`, `OrderFilled(AGGRESSIVE_FILL)` or `OrderExpired`                                                |
| replace | `ReplacementRejected`, `OrderBookEntry` or `OrderFilled(AGGRESSIVE_FILL)` of the new order (`ReplacementAccepted` is not terminal) |
| cancel  | `OrderCancelled` or `CancellationRejected`                                                                                         |

`PendingTransactions` on the client side counts exactly these reports.

## Engine events

All engine events are in `exchange/domain/engine_events.h`. Engine events are the internal engine output of `L3Book` and
`MatchingEngine`.

| Event                  | Emitted when                                            | Fields                                                                                                                                                                                                                                   |
| ---------------------- | ------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `OrderAccepted`        | an add request passed validation                        | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `price`, `quantity`, `direction`, `time_in_force`                                                                                                              |
| `Trade`                | a fill happened                                         | `origin_time`, `security_id`, `trade_id`, `aggressor_order_id`, `aggressor_client_id`, `aggressor_client_order_id`, `recipient_order_id`, `recipient_client_id`, `recipient_client_order_id`, `price`, `quantity`, `aggressor_direction` |
| `OrderBookEntry`       | an order (new or replaced) rested in the book           | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `price`, `quantity` (remaining), `direction`                                                                                                                   |
| `OrderFilled`          | an order has no quantity left                           | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `price`, `direction`, `type` (`PASSIVE_FILL` for the resting order, `AGGRESSIVE_FILL` for the incoming one)                                                    |
| `OrderReduced`         | a resting order was partially filled                    | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `price`, `remaining_quantity`, `direction`                                                                                                                     |
| `OrderExpired`         | an IOC order has quantity left after matching           | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `reason` (`IOC_REMAINDER_EXPIRED`)                                                                                                                             |
| `OrderCancelled`       | a cancel request found the order                        | `origin_time`, `order_id`, `client_id`, `client_order_id`, `client_cancellation_id`, `security_id`                                                                                                                                       |
| `OrderRejected`        | an add request failed validation                        | `origin_time`, `security_id`, `client_id`, `client_order_id`, `reason`                                                                                                                                                                   |
| `CancellationRejected` | a cancel request failed                                 | `origin_time`, `client_id`, `client_order_id`, `client_cancellation_id`, `security_id`, `reason`                                                                                                                                         |
| `ReplacementAccepted`  | a replace request found the order and passed validation | `origin_time`, `order_id`, `new_order_id`, `client_id`, `client_order_id`, `client_replacement_id`, `security_id`, `price`, `quantity`, `direction`                                                                                      |
| `ReplacementRejected`  | a replace request failed                                | `origin_time`, `client_id`, `client_order_id`, `client_replacement_id`, `security_id`, `reason`                                                                                                                                          |

`price` in `OrderFilled` is the order's own limit price, not a trade price. Each event carries the `origin_time` of the
original request.

## Market data events

Market data events are the L3 feed of the matching engine. They are located in `exchange/domain/market_events.h`. They
are anonymous and contain no client information.

| Event               | Fields                                                                                                    |
| ------------------- | --------------------------------------------------------------------------------------------------------- |
| `BookOrderEntry`    | `origin_time`, `order_id`, `security_id`, `price`, `quantity`, `direction`                                |
| `BookOrderChange`   | `origin_time`, `order_id`, `security_id`, `price`, `new_quantity`, `direction`                            |
| `BookOrderDeletion` | `origin_time`, `order_id`, `security_id`                                                                  |
| `Trade`             | `origin_time`, `security_id`, `trade_id`, `aggressor_order_id`, `recipient_order_id`, `price`, `quantity` |

Those feed events are enough to rebuild the full L3 book. `MaintainBook` does exactly that.

## Reports

Reports are located in `exchange/domain/reports.h`.

| Report                 | Meaning for the client                                                            | Fields                                                                                                                                                           |
| ---------------------- | --------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `OrderAccepted`        | the add request passed validation, the order got `order_id`                       | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `price`, `quantity`, `direction`, `time_in_force`                                      |
| `Trade`                | the order was filled by `quantity` at `price`, one report per trade for each side | `origin_time`, `security_id`, `client_id`, `client_order_id`, `trade_id`, `aggressor_order_id`, `recipient_order_id`, `price`, `quantity`, `aggressor_direction` |
| `OrderBookEntry`       | the order rests in the book with `quantity` left                                  | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `price`, `quantity`, `direction`                                                       |
| `OrderFilled`          | the order has no quantity left                                                    | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `price`, `direction`, `type` (`PASSIVE_FILL` or `AGGRESSIVE_FILL`)                     |
| `OrderExpired`         | the IOC order expired with quantity left                                          | `origin_time`, `order_id`, `client_id`, `client_order_id`, `security_id`, `reason` (`IOC_REMAINDER_EXPIRED`)                                                     |
| `OrderCancelled`       | the cancel request was executed                                                   | `origin_time`, `order_id`, `client_id`, `client_order_id`, `client_cancellation_id`, `security_id`                                                               |
| `OrderRejected`        | the add request was rejected                                                      | `origin_time`, `security_id`, `client_id`, `client_order_id`, `reason`                                                                                           |
| `CancellationRejected` | the cancel request was rejected                                                   | `origin_time`, `client_id`, `client_order_id`, `client_cancellation_id`, `security_id`, `reason`                                                                 |
| `ReplacementAccepted`  | the old order is out of the book, the replacement got `new_order_id`              | `origin_time`, `order_id`, `new_order_id`, `client_id`, `client_order_id`, `client_replacement_id`, `security_id`, `price`, `quantity`, `direction`              |
| `ReplacementRejected`  | the replace request was rejected                                                  | `origin_time`, `client_id`, `client_order_id`, `client_replacement_id`, `security_id`, `reason`                                                                  |

All report types except `Trade` are the same structures as the engine events with the same names. `report::Trade` is a
separate structure with one client id. There is no separate report for `OrderReduced` - the client learns about a
partial fill from the `Trade` report and computes the remaining quantity itself.

## ExchangeDispatcher

`ExchangeDispatcher` converts engine events into market data events and reports and sends them to the clients.

| Engine event           | Market data events                    | Reports                                          |
| ---------------------- | ------------------------------------- | ------------------------------------------------ |
| `OrderAccepted`        |                                       | `OrderAccepted`                                  |
| `Trade`                | `Trade`                               | `Trade` for both the aggressor and the recipient |
| `OrderBookEntry`       | `BookOrderEntry`                      | `OrderBookEntry`                                 |
| `OrderFilled`          | `BookOrderDeletion` if `PASSIVE_FILL` | `OrderFilled`                                    |
| `OrderReduced`         | `BookOrderChange` with `new_quantity` |                                                  |
| `OrderExpired`         |                                       | `OrderExpired`                                   |
| `OrderCancelled`       | `BookOrderDeletion`                   | `OrderCancelled`                                 |
| `OrderRejected`        |                                       | `OrderRejected`                                  |
| `CancellationRejected` |                                       | `CancellationRejected`                           |
| `ReplacementAccepted`  | `BookOrderDeletion` of the old order  | `ReplacementAccepted`                            |
| `ReplacementRejected`  |                                       | `ReplacementRejected`                            |

Please note that:

- `OrderFilled` does not carry the filled quantity of the order, the fills are reported separately for both the
  aggressor and the recipient with `Trade`.
- A replaced order shows in market data as a cancellation of the old order, trades of the new order and possibly an
  addition of the new order.

## Book containers

`L3Book` touches the book through the `BookContainer` concept. `StlL3BookContainer` is the reference implementation:
each half is `std::map<Price, std::list<Order>, compare>` with `compare = std::greater` for the buy half and
`compare = std::less` for the sell half, while `std::unordered_map<UniqueID, typename BookLevelType::iterator>` is used
to find an order in the book. All map, list and hash nodes come from `PlaceArena` through `CustomAllocator`. The only
allocation that bypasses the arena is for the hash table buckets. `matching_engine.reserve_books_size` in the config is
passed to every book. This is a hard limit: a `PlaceArena` never grows and an exhausted arena fails with
`Arena is empty`.

Currently it's the only implemented container and it's used as a baseline for future ones.
