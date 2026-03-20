#include "monitoring.hh"

#include <sstream>
#include <thread>
#include <print>
#include <unistd.h>

#include "config.hh"
#include <httplib.hh>

Monitoring::Monitoring() {
  char buf[HOST_NAME_MAX + 1];
  if (gethostname(buf, sizeof(buf)) == 0) {
    m_hostname = buf;
  } else {
    m_hostname = "COULD NOT BE FETCHED";
  }
}

void Monitoring::record(IOOp op, uint64_t duration_ns, uint64_t units) {
  // TODO
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

  // meta informations
  ss << "# HELP application_info Static information about the running binary.\n";
  ss << "# TYPE application_info gauge\n";
  ss << "application_info{toolname=\"iofs-ng\",version=\"v"
    << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH
    << "\",hostname=\"" << m_hostname << "\"} 1\n";

  // plugins loaded TODO
  // # HELP exporter_plugin_info Information about loaded plugins.
  // # TYPE exporter_plugin_info gauge
  // exporter_plugin_info{name="plugin1"} 1
  // exporter_plugin_info{name="plugin2"} 1
  // exporter_plugin_info{name="plugin3"} 1

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
