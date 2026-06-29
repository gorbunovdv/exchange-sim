#pragma once

namespace exchange_sim {

template<typename UnderlyingType>
class Counter {
public:
  UnderlyingType inc() { return ++counter_; }

private:
  UnderlyingType counter_ = 0;
};

}  // namespace exchange_sim