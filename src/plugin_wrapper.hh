#pragma once

#include "../plugins/plugin.hh"
#include <dlfcn.h>
#include <memory>
#include <string>

// Compiler cries about ODR with lambda instanciations, but I really like it, and may need it for future projects
// template <typename T, auto fn>
// using c_unique_ptr = std::unique_ptr<T, decltype([](T *ptr) { fn(ptr); })>;
//
// // deleters should return void
// using LibHandle = c_unique_ptr<void, [](void *handle){dlclose(handle);}>;

struct DlCloseDeleter {
  void operator()(void* handle) const {
    dlclose(handle);
  }
};

using LibHandle = std::unique_ptr<void, DlCloseDeleter>;

class PluginInstance {
public:
  explicit PluginInstance(const std::string &path);

  ~PluginInstance();

  // Move-only semantics
  PluginInstance(PluginInstance &&) = default;
  PluginInstance &operator=(PluginInstance &&) = default;
  PluginInstance(const PluginInstance &) = delete;
  PluginInstance &operator=(const PluginInstance &) = delete;

  // The Execute-Around Proxy
  class ArrowChainProxy {
    struct IofsPlugin *api;
  public:
    ArrowChainProxy(struct IofsPlugin *a, void *ctx);
    ~ArrowChainProxy();
    struct IofsPlugin *operator->();
  };

  // Overload -> to return the Proxy temporary
  ArrowChainProxy operator->() const;

  // Direct access for metadata (no context binding needed)
  struct IofsPlugin *api() const;

private:
  LibHandle m_lib;
  struct IofsPlugin *m_api{nullptr};
  void *m_ctx{nullptr};
};
