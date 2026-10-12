#pragma once
#include <limits>

namespace exchange_sim {

template<typename StrongType>
class Counter {
public:
  StrongType inc() {
    CHECK(counter_ < std::numeric_limits<typename StrongType::UnderlyingType>::max(), "Counter overflow");
    return StrongType{++counter_};
  }

private:
  StrongType::UnderlyingType counter_ = 0;
};

}  // namespace exchange_sim
