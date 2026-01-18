#include "monitoring.hh"

#include <sstream>
#include <thread>
#include <print>

#include <httplib.hh>

void Monitoring::record(IOOp op, uint64_t duration_ns, uint64_t units) {
  auto idx = static_cast<size_t>(op);
  if (idx < m_metrics.size()) { // This could be an assert in the future I think?
    // Relaxed memory order is okay since time doesnt matter in acc
    m_metrics[idx].count.fetch_add(1, std::memory_order_relaxed);
    m_metrics[idx].total_units.fetch_add(units, std::memory_order_relaxed);
    m_metrics[idx].total_duration_ns.fetch_add(duration_ns, std::memory_order_relaxed);
  }
}

void Monitoring::start_server(int port) {
  std::thread([port]() {
    httplib::Server svr;

    svr.Get("/metrics", [](const httplib::Request &, httplib::Response &res) {
      res.set_content(Monitoring::instance().generate_prometheus_output(), "text/plain");
    });

    std::println("Starting Prometheus metrics server on port {}", port);
    if (!svr.listen("0.0.0.0", port)) {
      std::println(stderr, "Failed to start metrics server on port {}", port);
    }
  }).detach();
}

std::string Monitoring::generate_prometheus_output() const {
  std::stringstream ss;

  // 1. Operation Counts
  ss << "# HELP iofs_ops_total Total number of IO operations\n";
  ss << "# TYPE iofs_ops_total counter\n";
  for (size_t i = 0; i < m_metrics.size(); ++i) {
    auto val = m_metrics[i].count.load(std::memory_order_relaxed);
    if (val > 0) {
      ss << "iofs_ops_total{op=\"" << op_to_string(static_cast<IOOp>(i)) << "\"} " << val << "\n";
    }
  }

  // 2. Duration (converted to seconds)
  ss << "\n# HELP iofs_duration_seconds_total Total time spent in operations\n";
  ss << "# TYPE iofs_duration_seconds_total counter\n";
  for (size_t i = 0; i < m_metrics.size(); ++i) {
    auto ns = m_metrics[i].total_duration_ns.load(std::memory_order_relaxed);
    if (ns > 0) {
      double seconds = static_cast<double>(ns) / 1.0e9;
      ss << "iofs_duration_seconds_total{op=\"" << op_to_string(static_cast<IOOp>(i)) << "\"} " << seconds << "\n";
    }
  }

  // 3. Units (Bytes/Calls)
  ss << "\n# HELP iofs_units_total Total units (bytes/entries) processed\n";
  ss << "# TYPE iofs_units_total counter\n";
  for (size_t i = 0; i < m_metrics.size(); ++i) {
    auto val = m_metrics[i].total_units.load(std::memory_order_relaxed);
    if (val > 0) {
      ss << "iofs_units_total{op=\"" << op_to_string(static_cast<IOOp>(i)) << "\"} " << val << "\n";
    }
  }

  return ss.str();
}

const char *Monitoring::op_to_string(IOOp op) {
  switch (op) {
    case IOOp::getattr: return "getattr";
    case IOOp::readlink: return "readlink";
    case IOOp::mkdir: return "mkdir";
    case IOOp::unlink: return "unlink";
    case IOOp::rmdir: return "rmdir";
    case IOOp::symlink: return "symlink";
    case IOOp::rename: return "rename";
    case IOOp::link: return "link";
    case IOOp::chmod: return "chmod";
    case IOOp::chown: return "chown";
    case IOOp::truncate: return "truncate";
    case IOOp::open: return "open";
    case IOOp::read: return "read";
    case IOOp::write: return "write";
    case IOOp::statfs: return "statfs";
    case IOOp::flush: return "flush";
    case IOOp::release: return "release";
    case IOOp::fsync: return "fsync";
    case IOOp::setxattr: return "setxattr";
    case IOOp::getxattr: return "getxattr";
    case IOOp::listxattr: return "listxattr";
    case IOOp::removexattr: return "removexattr";
    case IOOp::opendir: return "opendir";
    case IOOp::readdir: return "readdir";
    case IOOp::releasedir: return "releasedir";
    case IOOp::access: return "access";
    case IOOp::create: return "create";
    case IOOp::utimens: return "utimens";
    case IOOp::write_buf: return "write_buf";
    case IOOp::read_buf: return "read_buf";
    case IOOp::flock: return "flock";
    case IOOp::fallocate: return "fallocate";
    default: return "unknown";
  }
}
