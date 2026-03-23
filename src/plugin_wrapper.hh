#pragma once

#include "../plugins/plugin.hh"
#include <dlfcn.h>
#include <memory>
#include <string>
#include <stdexcept>
#include <print>

template <typename T, auto fn>
using c_unique_ptr = std::unique_ptr<T, decltype([](T *ptr) { fn(ptr); })>;

// deleters should return void
using LibHandle = c_unique_ptr<void, [](void *handle){dlclose(handle);}>;

class PluginInstance {
public:
  explicit PluginInstance(const std::string &path) {
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

  ~PluginInstance() {
    if (m_api && m_api->destroy) {
      m_api->destroy(m_ctx);
    }
  }

  // Move-only semantics
  PluginInstance(PluginInstance &&) = default;
  PluginInstance &operator=(PluginInstance &&) = default;
  PluginInstance(const PluginInstance &) = delete;
  PluginInstance &operator=(const PluginInstance &) = delete;

  // The Execute-Around Proxy
  class ArrowChainProxy {
    struct IofsPlugin *api;
    void *ctx;
  public:
    ArrowChainProxy(struct IofsPlugin *a, void *c) : api{a}, ctx{c} {
      api->bind(ctx);
    }
    ~ArrowChainProxy() {
      api->bind(nullptr);
    }
    struct IofsPlugin *operator->() { return api; }
  };

  // Overload -> to return the Proxy temporary
  ArrowChainProxy operator->() const {
    return ArrowChainProxy(m_api, m_ctx);
  }

  // Direct access for metadata (no context binding needed)
  struct IofsPlugin *api() const { return m_api; }

private:
  LibHandle m_lib;
  struct IofsPlugin *m_api{nullptr};
  void *m_ctx{nullptr};
};
