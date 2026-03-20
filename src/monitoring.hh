#pragma once

#include "iofs.hh"
#include <string>

// Automatically determine size based on the synthetic 'last' enum
constexpr size_t IO_OP_COUNT = static_cast<size_t>(IOOp::last);

class Monitoring {
public:
  // Singleton
  static Monitoring &instance() {
    static Monitoring inst{};
    return inst;
  }

  void record(IOOp op, uint64_t duration_ns, uint64_t units);
  void start_server(int port);

private:
  Monitoring() = default;

  std::string generate_prometheus_output() const;
  static const char *op_to_string(IOOp op);
};
