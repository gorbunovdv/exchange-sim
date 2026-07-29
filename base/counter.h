#pragma once

namespace exchange_sim {

template<typename StrongType>
class Counter {
public:
  StrongType inc() { return StrongType{++counter_}; }

private:
  StrongType::UnderlyingType counter_ = 0;
};

}  // namespace exchange_sim
