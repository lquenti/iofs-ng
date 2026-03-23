#include "plugin.hh"
#include <iostream>
#include <string>
#include <atomic>
#include <cstdio>

// Exmaple plugin.
// - Takes all read/write operations, and accumulates them.

class SamplePlugin {
  int id;
  std::string name;

  std::atomic<uint64_t> total_ops{0};
  std::atomic<uint64_t> total_duration_ns{0};
  std::atomic<uint64_t> total_bytes_transferred{0};

public:
  SamplePlugin() {
    static std::atomic<int> counter{0};
    id = ++counter;
    name = "Instance_" + std::to_string(id);
    std::cout << "[SamplePlugin " << name << "] Constructed\n";
  }

  ~SamplePlugin() {
    std::cout << "[SamplePlugin " << name << "] Destructed\n";
    std::cout << "[SamplePlugin " << name << "] Final Stats - Ops: " << total_ops.load()
              << " | Bytes: " << total_bytes_transferred.load() << "\n";
  }

  void record(iofs_op_t op, uint64_t duration_ns, uint64_t units) {
    // relaxed since I dont care about the order (only looking at then end) (and its commutative/associative)
    total_ops.fetch_add(1, std::memory_order_relaxed);
    total_duration_ns.fetch_add(duration_ns, std::memory_order_relaxed);

    if (op == IOFS_OP_READ || op == IOFS_OP_READ_BUF || op == IOFS_OP_WRITE || op == IOFS_OP_WRITE_BUF) {
      total_bytes_transferred.fetch_add(units, std::memory_order_relaxed);
    }
  }

  size_t poll_metrics(char *buf, size_t buf_size) {
    int written = std::snprintf(buf, buf_size,
      "# HELP dummy_total_ops Total number of I/O operations recorded.\n"
      "# TYPE dummy_total_ops counter\n"
      "dummy_total_ops %llu\n"
      "# HELP dummy_total_bytes Total bytes read/written.\n"
      "# TYPE dummy_total_bytes counter\n"
      "dummy_total_bytes %llu\n",
      static_cast<unsigned long long>(total_ops.load(std::memory_order_relaxed)),
      static_cast<unsigned long long>(total_bytes_transferred.load(std::memory_order_relaxed))
    );

    if (written < 0) {
      return 0;
    }
    return static_cast<size_t>(written);
  }
};

static thread_local SamplePlugin *g_instance = nullptr;

static struct IofsPlugin plugin_api = {
  .get_name = []() -> const char* { return "SamplePlugin"; },
  .get_version = []() -> const char* { return "0.1.0"; },

  .init = []() -> void* { return new SamplePlugin(); },
  .bind = [](void* ctx) { g_instance = static_cast<SamplePlugin*>(ctx); },
  .destroy = [](void* ctx) { delete static_cast<SamplePlugin*>(ctx); },

  .record = [](auto... args) { g_instance->record(args...); },
  .poll_prometheus_metrics = [](auto... args) { return g_instance->poll_metrics(args...); }

};

extern "C" {
  struct IofsPlugin *get_iofs_plugin(void) {
    return &plugin_api;
  }
}
