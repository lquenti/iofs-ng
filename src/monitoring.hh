#pragma once

#include "iofs.hh"
#include <array>
#include <atomic>
#include <string>

// Automatically determine size based on the synthetic 'last' enum
constexpr size_t IO_OP_COUNT = static_cast<size_t>(IOOp::last);

struct MetricEntry {
  std::atomic<uint64_t> count{0};
  std::atomic<uint64_t> total_units{0};
  std::atomic<uint64_t> total_duration_ns{0};
};

class Monitoring {
public:
  // Singleton
  static Monitoring &instance() {
    static Monitoring inst{};
    return inst;
  }

  void record(IOOp op, uint64_t duration_ns, uint64_t units);
  void start_server(int port);
  std::string generate_prometheus_output() const;

private:
  Monitoring() = default;
  std::array<MetricEntry, IO_OP_COUNT> m_metrics;

  static const char *op_to_string(IOOp op);
};
