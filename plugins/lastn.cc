#include "plugin.hh"
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

// Sample plugin: Records last N metrics
// - Discards everything except `read`/`write` (and optionally `read_buf`/`write_buf`).
// - Uses a `N`-sized ring buffer
// - Each slot records: was it a read or write, how many bytes, how long it took.
//
// Note: It is not fully correct, as we do not lock the ring buffer while pulling from it for prometheus.
// This is very unfortunate, but otherwise we'd have to lock a mutex for each entry, which would be an (imo)
// unproportional overhead.
// One could argue that using a double buffered ring buffer would increase correctness. BUT, it very much increases
// code complexity (for a sample plugin!) and is still not 100%; You'd have to lock both buffers when switching indices.
// And to enforce that, you'd also have to lock for each I/O, which is way more heavyweight than some atomic int.
// (An atomic int double buffer index wouldn't help either, as it could be switched between you fetching the
// atomic and you starting the write)

// CHANGE TO YOUR PREFERENCE
static constexpr size_t LAST_N = 128;

// Whether to count `read_buf`/`write_buf` as well.
// Obviously, its a noop if its not compiled in iofs-ng
//
// #define LAST_N_USE_ZERO_COPY

struct Entry {
  bool is_write;
  uint64_t size_bytes;
  uint64_t duration_ns;
};

class LastNPlugin {
  int m_id;
  std::string m_name;

  // Ring buffer: `m_head` is the NEXT to write (mod N)
  std::array<std::optional<Entry>, LAST_N> m_ring{};
  std::atomic<uint64_t> m_head{0};

public:
  LastNPlugin() {
    static std::atomic<int> counter{0};
    m_id = ++counter;
    m_name = "Instance_" + std::to_string(m_id);
    std::cout << "[LastNPlugin " << m_name << "] Constructed (N=" << LAST_N << ")" << std::endl;
  }

  ~LastNPlugin() {
    std::cout << "[LastNPlugin " << m_name << "] Destructed" << std::endl;
  }

  void record(iofs_op_t op, uint64_t duration_ns, uint64_t units) {
    bool is_write = false;

    switch (op) {
      case IOFS_OP_READ:
        is_write = false;
        break;
      case IOFS_OP_WRITE:
        is_write = true;
        break;

#ifdef LAST_N_USE_ZERO_COPY
      case IOFS_OP_READ_BUF:
        is_write = false;
        break;
      case IOFS_OP_WRITE_BUF:
        is_write = true;
        break;
#endif

      default:
        return; // not read write, we DO NOT care
    }

    // Get next slot (relaxed since we dont really care about order correctness (at least not for that price))
    uint64_t slot_idx{m_head.fetch_add(1, std::memory_order_relaxed) % LAST_N};
    m_ring[slot_idx] = Entry{is_write, units, duration_ns};
  }

  size_t poll_metrics(char *buf, size_t buf_size) {
    // Get current state
    uint64_t head{m_head.load(std::memory_order_relaxed)};
    uint64_t filled{(head < LAST_N) ? head : LAST_N};  // works since it monotonically increases
    uint64_t start{(head >= LAST_N) ? (head % LAST_N) : 0}; // oldest entry

    size_t offset{0};
    uint64_t seq_base{head - filled}; // absolute seq of the oldest entry in the window

    // Write header once
    int written = std::snprintf(buf, buf_size,
      "# HELP lastN Per-entry I/O record. i=global sequence number. size label is bytes transferred.\n"
      "# TYPE lastN gauge\n"
    );
    if (written < 0) return 0;
    offset += static_cast<size_t>(written);

    for (uint64_t i = 0; i < filled; ++i) {
      const auto &slot{m_ring[(start + i) % LAST_N]};
      if (!slot.has_value()) {
        continue;
      }

      const Entry &e = slot.value();
      written = std::snprintf(buf + offset, buf_size - offset,
        "lastN{i=\"%llu\",size=\"%llu\",op=\"%s\"} %llu\n",
        static_cast<unsigned long long>(seq_base + i),
        static_cast<unsigned long long>(e.size_bytes),
        e.is_write ? "w" : "r",
        static_cast<unsigned long long>(e.duration_ns)
      );
      if (written < 0) return 0;
      offset += static_cast<size_t>(written);
      if (offset >= buf_size) return buf_size; // truncated
    }

    return offset;
  }
};

static thread_local LastNPlugin *g_instance = nullptr;

static struct IofsPlugin plugin_api = {
  .get_name    = []() -> const char * { return "LastNPlugin"; },
  .get_version = []() -> const char * { return "0.1.0"; },

  .init    = []() -> void * { return new LastNPlugin(); },
  .bind    = [](void *ctx) { g_instance = static_cast<LastNPlugin *>(ctx); },
  .destroy = [](void *ctx) { delete static_cast<LastNPlugin *>(ctx); },

  .record                 = [](auto... args) { g_instance->record(args...); },
  .poll_prometheus_metrics = [](auto... args) { return g_instance->poll_metrics(args...); },
};

extern "C" {
  struct IofsPlugin *get_iofs_plugin(void) {
    return &plugin_api;
  }
}
