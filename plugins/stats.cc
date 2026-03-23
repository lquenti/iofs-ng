#include "plugin.hh"
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <iterator>
#include <utility>

// **Minimal Mode**: Only the relevant subsets. What "relevant" is can be configured below
//
// #define STATS_MINIMAL

// The histogram <=x buckets. Edit as you want, size is autocomputed at compile time
constexpr size_t STATS_HIST_BOUNDS[] = {
  4096,
  4096 * 2,
  4096 * 4,
  65536,
  65536 * 2,
  65536 * 4,
};
constexpr size_t STATS_HIST_EXPLICIT_BUCKETS = std::size(STATS_HIST_BOUNDS);
constexpr size_t STATS_HIST_TOTAL_BUCKETS = STATS_HIST_EXPLICIT_BUCKETS + 1; // +Inf

#ifdef STATS_MINIMAL
  // Here you can define what minimal means
  #define STATS_OP_READ
  #define STATS_OP_WRITE
  #define STATS_OP_READ_BUF
  #define STATS_OP_WRITE_BUF
  #define STATS_OP_OPEN
  #define STATS_OP_CREATE
  #define STATS_OP_RELEASE
  #define STATS_OP_GETATTR
  #define STATS_OP_READDIR
#else
  // If you want to disable stuff even in verbose mode if you don't care
  #define STATS_OP_GETATTR
  #define STATS_OP_READLINK
  #define STATS_OP_MKDIR
  #define STATS_OP_UNLINK
  #define STATS_OP_RMDIR
  #define STATS_OP_SYMLINK
  #define STATS_OP_RENAME
  #define STATS_OP_LINK
  #define STATS_OP_CHMOD
  #define STATS_OP_CHOWN
  #define STATS_OP_TRUNCATE
  #define STATS_OP_OPEN
  #define STATS_OP_READ
  #define STATS_OP_WRITE
  #define STATS_OP_STATFS
  #define STATS_OP_FLUSH
  #define STATS_OP_RELEASE
  #define STATS_OP_FSYNC
  #define STATS_OP_SETXATTR
  #define STATS_OP_GETXATTR
  #define STATS_OP_LISTXATTR
  #define STATS_OP_REMOVEXATTR
  #define STATS_OP_OPENDIR
  #define STATS_OP_READDIR
  #define STATS_OP_RELEASEDIR
  #define STATS_OP_ACCESS
  #define STATS_OP_CREATE
  #define STATS_OP_UTIMENS
  #define STATS_OP_WRITE_BUF
  #define STATS_OP_READ_BUF
  #define STATS_OP_FLOCK
  #define STATS_OP_FALLOCATE
#endif

// To set whats enabled on compile time so that we can make sure its as little overhead as possible
static constexpr bool OP_ENABLED[IOFS_OP_COUNT] = {
  /* IOFS_OP_GETATTR      */ (bool)
#ifdef STATS_OP_GETATTR
  true,
#else
  false,
#endif
  /* IOFS_OP_READLINK     */
#ifdef STATS_OP_READLINK
  true,
#else
  false,
#endif
  /* IOFS_OP_MKDIR        */
#ifdef STATS_OP_MKDIR
  true,
#else
  false,
#endif
  /* IOFS_OP_UNLINK       */
#ifdef STATS_OP_UNLINK
  true,
#else
  false,
#endif
  /* IOFS_OP_RMDIR        */
#ifdef STATS_OP_RMDIR
  true,
#else
  false,
#endif
  /* IOFS_OP_SYMLINK      */
#ifdef STATS_OP_SYMLINK
  true,
#else
  false,
#endif
  /* IOFS_OP_RENAME       */
#ifdef STATS_OP_RENAME
  true,
#else
  false,
#endif
  /* IOFS_OP_LINK         */
#ifdef STATS_OP_LINK
  true,
#else
  false,
#endif
  /* IOFS_OP_CHMOD        */
#ifdef STATS_OP_CHMOD
  true,
#else
  false,
#endif
  /* IOFS_OP_CHOWN        */
#ifdef STATS_OP_CHOWN
  true,
#else
  false,
#endif
  /* IOFS_OP_TRUNCATE     */
#ifdef STATS_OP_TRUNCATE
  true,
#else
  false,
#endif
  /* IOFS_OP_OPEN         */
#ifdef STATS_OP_OPEN
  true,
#else
  false,
#endif
  /* IOFS_OP_READ         */
#ifdef STATS_OP_READ
  true,
#else
  false,
#endif
  /* IOFS_OP_WRITE        */
#ifdef STATS_OP_WRITE
  true,
#else
  false,
#endif
  /* IOFS_OP_STATFS       */
#ifdef STATS_OP_STATFS
  true,
#else
  false,
#endif
  /* IOFS_OP_FLUSH        */
#ifdef STATS_OP_FLUSH
  true,
#else
  false,
#endif
  /* IOFS_OP_RELEASE      */
#ifdef STATS_OP_RELEASE
  true,
#else
  false,
#endif
  /* IOFS_OP_FSYNC        */
#ifdef STATS_OP_FSYNC
  true,
#else
  false,
#endif
  /* IOFS_OP_SETXATTR     */
#ifdef STATS_OP_SETXATTR
  true,
#else
  false,
#endif
  /* IOFS_OP_GETXATTR     */
#ifdef STATS_OP_GETXATTR
  true,
#else
  false,
#endif
  /* IOFS_OP_LISTXATTR    */
#ifdef STATS_OP_LISTXATTR
  true,
#else
  false,
#endif
  /* IOFS_OP_REMOVEXATTR  */
#ifdef STATS_OP_REMOVEXATTR
  true,
#else
  false,
#endif
  /* IOFS_OP_OPENDIR      */
#ifdef STATS_OP_OPENDIR
  true,
#else
  false,
#endif
  /* IOFS_OP_READDIR      */
#ifdef STATS_OP_READDIR
  true,
#else
  false,
#endif
  /* IOFS_OP_RELEASEDIR   */
#ifdef STATS_OP_RELEASEDIR
  true,
#else
  false,
#endif
  /* IOFS_OP_ACCESS       */
#ifdef STATS_OP_ACCESS
  true,
#else
  false,
#endif
  /* IOFS_OP_CREATE       */
#ifdef STATS_OP_CREATE
  true,
#else
  false,
#endif
  /* IOFS_OP_UTIMENS      */
#ifdef STATS_OP_UTIMENS
  true,
#else
  false,
#endif
  /* IOFS_OP_WRITE_BUF    */
#ifdef STATS_OP_WRITE_BUF
  true,
#else
  false,
#endif
  /* IOFS_OP_READ_BUF     */
#ifdef STATS_OP_READ_BUF
  true,
#else
  false,
#endif
  /* IOFS_OP_FLOCK        */
#ifdef STATS_OP_FLOCK
  true,
#else
  false,
#endif
  /* IOFS_OP_FALLOCATE    */
#ifdef STATS_OP_FALLOCATE
  true,
#else
  false,
#endif
};
static_assert(std::size(OP_ENABLED) == IOFS_OP_COUNT, "OP_ENABLED out of sync with iofs_op_t");

// TODO REDUNDANCY REMOVE EITHER ME OR THE PLUGIN THINGY
static constexpr const char *OP_NAMES[] = {
  "getattr", "readlink", "mkdir", "unlink", "rmdir", "symlink", "rename",
  "link", "chmod", "chown", "truncate", "open", "read", "write", "statfs",
  "flush", "release", "fsync", "setxattr", "getxattr", "listxattr",
  "removexattr", "opendir", "readdir", "releasedir", "access", "create",
  "utimens", "write_buf", "read_buf", "flock", "fallocate",
};
static_assert(std::size(OP_NAMES) == IOFS_OP_COUNT, "OP_NAMES out of sync with iofs_op_t");

class StatsPlugin {
  std::atomic<uint64_t> m_ops_total[IOFS_OP_COUNT]{};
  std::atomic<uint64_t> m_duration_ns[IOFS_OP_COUNT]{};

  // Histogram is only defined for `r` `w` (idx can be seen as `isWrite`, i.e. `1==write`)
  std::atomic<uint64_t> m_hist_bucket[2][STATS_HIST_TOTAL_BUCKETS]{};
  std::atomic<uint64_t> m_hist_count[2]{};
  std::atomic<uint64_t> m_hist_sum[2]{};

  // Returns {tracked, is_write}. tracked=false means op is not histogrammed.
  static std::pair<bool, bool> hist_rw(iofs_op_t op) {
    if (op == IOFS_OP_READ || op == IOFS_OP_READ_BUF) {
      return {true, false};
    }
    if (op == IOFS_OP_WRITE || op == IOFS_OP_WRITE_BUF) {
      return {true, true};
    }
    return {false, false};
  }

  // Find the bucket index for a given byte count.
  static size_t bucket_for(uint64_t bytes) {
    for (size_t i = 0; i < STATS_HIST_EXPLICIT_BUCKETS; ++i) {
      if (bytes <= STATS_HIST_BOUNDS[i]) {
        return i;
      }
    }
    return STATS_HIST_EXPLICIT_BUCKETS; // +Inf
  }

  // Type-safe snprintf wrapper that advances offset
  template <typename... Args>
  static bool emit(char *buf, size_t buf_size, size_t &offset, const char *fmt, Args&&... args) {
    if (offset >= buf_size) {
      return false;
    }
    int written = std::snprintf(buf + offset, buf_size - offset, fmt, std::forward<Args>(args)...);
    if (written < 0) {
      return false;
    }
    offset += static_cast<size_t>(written);
    return offset < buf_size;
  }

public:
  void record(iofs_op_t op, uint64_t duration_ns, uint64_t units) {
    if (static_cast<size_t>(op) >= IOFS_OP_COUNT) {
      return;
    }

    // Is it part of our current mode
    if (OP_ENABLED[op]) {
      m_ops_total[op].fetch_add(1, std::memory_order_relaxed);
      m_duration_ns[op].fetch_add(duration_ns, std::memory_order_relaxed);
    }

    // If read/write we always track
    auto [tracked, is_write] = hist_rw(op);
    if (tracked) {
      size_t bucket = bucket_for(units);
      // Increment all buckets from the matching one upward to maintain the
      // Prometheus cumulative invariant: bucket[le] = count of observations <= le.
      for (size_t b = bucket; b < STATS_HIST_TOTAL_BUCKETS; ++b) {
        m_hist_bucket[is_write][b].fetch_add(1, std::memory_order_relaxed);
      }
      m_hist_count[is_write].fetch_add(1, std::memory_order_relaxed);
      m_hist_sum[is_write].fetch_add(units, std::memory_order_relaxed);
    }
  }

  size_t poll_metrics(char *buf, size_t buf_size) {
    size_t offset = 0;

    emit(buf, buf_size, offset,
      "# HELP iofs_ops_total Cumulative number of times each FUSE op was called.\n"
      "# TYPE iofs_ops_total counter\n");
    for (size_t i = 0; i < IOFS_OP_COUNT; ++i) {
      if (!OP_ENABLED[i]) {
        continue;
      }
      emit(buf, buf_size, offset,
        "iofs_ops_total{op=\"%s\"} %llu\n",
        OP_NAMES[i],
        static_cast<unsigned long long>(m_ops_total[i].load(std::memory_order_relaxed)));
    }

    emit(buf, buf_size, offset,
      "# HELP iofs_duration_ns_total Cumulative nanoseconds spent in each FUSE op.\n"
      "# TYPE iofs_duration_ns_total counter\n");
    for (size_t i = 0; i < IOFS_OP_COUNT; ++i) {
      if (!OP_ENABLED[i]) {
        continue;
      }
      emit(buf, buf_size, offset,
        "iofs_duration_ns_total{op=\"%s\"} %llu\n",
        OP_NAMES[i],
        static_cast<unsigned long long>(m_duration_ns[i].load(std::memory_order_relaxed)));
    }

    static constexpr const char *RW_NAMES[2] = {"read", "write"};
    emit(buf, buf_size, offset,
      "# HELP iofs_io_bytes Histogram of bytes transferred per read/write call.\n"
      "# TYPE iofs_io_bytes histogram\n");
    for (int w = 0; w < 2; ++w) {
      for (size_t b = 0; b < STATS_HIST_EXPLICIT_BUCKETS; ++b) {
        emit(buf, buf_size, offset,
          "iofs_io_bytes_bucket{op=\"%s\",le=\"%zu\"} %llu\n",
          RW_NAMES[w],
          STATS_HIST_BOUNDS[b],
          static_cast<unsigned long long>(m_hist_bucket[w][b].load(std::memory_order_relaxed)));
      }
      emit(buf, buf_size, offset,
        "iofs_io_bytes_bucket{op=\"%s\",le=\"+Inf\"} %llu\n",
        RW_NAMES[w],
        static_cast<unsigned long long>(m_hist_bucket[w][STATS_HIST_EXPLICIT_BUCKETS].load(std::memory_order_relaxed)));
      emit(buf, buf_size, offset,
        "iofs_io_bytes_count{op=\"%s\"} %llu\n",
        RW_NAMES[w],
        static_cast<unsigned long long>(m_hist_count[w].load(std::memory_order_relaxed)));
      emit(buf, buf_size, offset,
        "iofs_io_bytes_sum{op=\"%s\"} %llu\n",
        RW_NAMES[w],
        static_cast<unsigned long long>(m_hist_sum[w].load(std::memory_order_relaxed)));
    }

    return offset;
  }
};

static thread_local StatsPlugin *g_instance = nullptr;

static struct IofsPlugin plugin_api = {
  .get_name = []() -> const char * { return "StatsPlugin"; },
  .get_version = []() -> const char * { return "0.1.0"; },

  .init = []() -> void * { return new StatsPlugin(); },
  .bind = [](void *ctx) { g_instance = static_cast<StatsPlugin *>(ctx); },
  .destroy = [](void *ctx) { delete static_cast<StatsPlugin *>(ctx); },

  .record = [](auto... args) { g_instance->record(args...); },
  .poll_prometheus_metrics = [](auto... args) { return g_instance->poll_metrics(args...); },
};

extern "C" {
  struct IofsPlugin *get_iofs_plugin(void) {
    return &plugin_api;
  }
}
