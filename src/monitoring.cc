#include "monitoring.hh"

#include <sstream>
#include <thread>
#include <print>
#include <unistd.h>

#include "config.hh"
#include "../include/httplib.hh"

Monitoring::Monitoring() {
  char buf[HOST_NAME_MAX + 1];
  if (gethostname(buf, sizeof(buf)) == 0) {
    m_hostname = buf;
  } else {
    m_hostname = "COULD NOT BE FETCHED";
  }
}

void Monitoring::load_plugins(const std::vector<std::string> &plugin_paths) {
  m_plugins.reserve(plugin_paths.size());
  for (const auto &path: plugin_paths) {
    // throws
    m_plugins.emplace_back(path);
  }
}

void Monitoring::record(IOOp op, uint64_t duration_ns, uint64_t units) {
  // Cast C++ enum to C-ABI enum
  iofs_op_t c_op{static_cast<iofs_op_t>(op)};
  for (auto& plugin : m_plugins) {
    plugin->record(c_op, duration_ns, units);
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

  // meta informations
  ss << "# HELP application_info Static information about the running binary.\n";
  ss << "# TYPE application_info gauge\n";
  ss << "application_info{toolname=\"iofs-ng\",version=\"v"
    << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH
    << "\",hostname=\"" << m_hostname << "\"} 1\n";

  // plugin info
  ss << "# HELP exporter_plugin_info Information about loaded plugins.\n";
  ss << "# TYPE exporter_plugin_info gauge\n";
  for (const auto& plugin : m_plugins) {
    ss << "exporter_plugin_info{name=\"" << plugin->get_name()
       << "\",version=\"" << plugin->get_version() << "\"} 1\n";
  }

  thread_local auto buffer{std::make_unique<char []>(PLUGIN_BUFFER_BYTES)};
  for (auto& plugin : m_plugins) {
    if (plugin.api()->poll_prometheus_metrics) {
      size_t written{plugin->poll_prometheus_metrics(buffer.get(), PLUGIN_BUFFER_BYTES)};
      if (written > 0 && written < PLUGIN_BUFFER_BYTES) {
        ss << '\n' << std::string_view(buffer.get(), written);
      }
    }
  }

  return ss.str();
}
