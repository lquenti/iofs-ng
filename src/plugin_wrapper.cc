#include "plugin_wrapper.hh"
#include <print>

PluginInstance::PluginInstance(const std::string &path) {
  void *handle{dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL)};
  if (!handle) {
    throw std::runtime_error("Failed to load plugin " + path + ": " + dlerror());
  }
  m_lib.reset(handle);

  auto get_plugin_fn{reinterpret_cast<struct IofsPlugin *(*)()>(dlsym(handle, "get_iofs_plugin"))};
  if (!get_plugin_fn) {
    throw std::runtime_error("Missing symbol 'get_iofs_plugin' in " + path);
  }

  m_api = get_plugin_fn();
  if (!validate_iofs_plugin(m_api)) {
    throw std::runtime_error("Plugin API validation failed (or version mismatch) for " + path);
  }

  // Initialize the plugin and store its context
  m_ctx = m_api->init();

  std::println("Loaded plugin: {} (v{}) from {}", m_api->get_name(), m_api->get_version(), path);
}

PluginInstance::~PluginInstance() {
  if (m_api && m_api->destroy) {
    m_api->destroy(m_ctx);
  }
}

PluginInstance::ArrowChainProxy::ArrowChainProxy(struct IofsPlugin *a, void *ctx) : api{a} {
  api->bind(ctx);
}

PluginInstance::ArrowChainProxy::~ArrowChainProxy() {
  api->bind(nullptr);
}

struct IofsPlugin *PluginInstance::ArrowChainProxy::operator->() {
  return api;
}

PluginInstance::ArrowChainProxy PluginInstance::operator->() const {
  return ArrowChainProxy(m_api, m_ctx);
}

struct IofsPlugin *PluginInstance::api() const {
  return m_api;
}
