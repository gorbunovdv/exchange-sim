#pragma once
#include "base/sink.h"

namespace exchange_sim {

template<typename Object, SinkOf<Object> SinkType>
class ForwardSink {
public:
  explicit ForwardSink(SinkType* sink = nullptr): sink_(sink) {}

  void set_sink(SinkType* sink) { sink_ = sink; }

  void push(Object&& object) { sink_->push(std::forward<Object>(object)); }

private:
  SinkType* sink_;
};

}  // namespace exchange_sim