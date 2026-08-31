# Order generation

There are currently two order generators implemented: `SimpleOrderGenerator` and `ShiftOrderGenerator`. 

## SimpleOrderGenerator

`SimpleOrderGenerator` is a simple order generator that generates orders with random prices and quantities. It selects
all parameters at random: order type (add/replace/cancel), security, price, quantity and so on. Although it's simple, it's a good starting point 
and it's good for basic testing of `MatchingEngine`.

## ShiftOrderGenerator

`ShiftOrderGenerator` is an order generator that tries to resemble real market orders. The goal of this generator would be to simulate 
actual stages when prices do not change and shifts of prices during the trading. It's a state machine consisting of three stages:
`QUIET`, `REPLENISH` and `SHIFT`. The initial starting state is `SHIFT` with an empty order book. Each security has its own current state and book.

### SHIFT

`SHIFT` stage represents a stage when prices are shifted. In the beginning of this stage, the generator generates a random price shift and 
therefore selects new `middle_price`. Then it generates beats in the direction of the price shift until there are no orders with
`Direction::BUY` and `price > middle_price` and no orders with `Direction::SELL` and `price <= middle_price`. After that it transitions to
`REPLENISH` stage.

### REPLENISH

`REPLENISH` stage represents a stage when there are too few orders in the book. It generates new limit orders until `active_orders_count >= min_active_orders_count`.
New orders are generated with random prices and quantities. After that it transitions to `QUIET` stage.

### QUIET

`QUIET` stage represents a stage when there are enough orders in the book. It generates all types of event: add order, replace order, cancel order and beats.
Beats are specifically generated small: no more than one order could be fully filled by beat.

Types of generated events are limited by the current state: add order could not be generated if `active_orders_count == max_active_orders_count`, 
cancel order could not be generated if `active_orders_count == min_active_orders_count` and so on.

`QUIET` stage ends once it sends `max_quiet_state_sent_orders_count` orders.

### Beats

Beat is generated using an existing order book. It selects direction randomly, selects the number of orders to be filled and generates 
 the amount so that these orders could be fully filled by beat. Although future beats are planned to be using both replace and add orders,
currently only add orders are supported because of the design of OrderStorage.

