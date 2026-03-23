#ifndef IOFS_PLUGIN_API_H
#define IOFS_PLUGIN_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  IOFS_OP_GETATTR = 0,
  IOFS_OP_READLINK,
  IOFS_OP_MKDIR,
  IOFS_OP_UNLINK,
  IOFS_OP_RMDIR,
  IOFS_OP_SYMLINK,
  IOFS_OP_RENAME,
  IOFS_OP_LINK,
  IOFS_OP_CHMOD,
  IOFS_OP_CHOWN,
  IOFS_OP_TRUNCATE,
  IOFS_OP_OPEN,
  IOFS_OP_READ,
  IOFS_OP_WRITE,
  IOFS_OP_STATFS,
  IOFS_OP_FLUSH,
  IOFS_OP_RELEASE,
  IOFS_OP_FSYNC,
  IOFS_OP_SETXATTR,
  IOFS_OP_GETXATTR,
  IOFS_OP_LISTXATTR,
  IOFS_OP_REMOVEXATTR,
  IOFS_OP_OPENDIR,
  IOFS_OP_READDIR,
  IOFS_OP_RELEASEDIR,
  IOFS_OP_ACCESS,
  IOFS_OP_CREATE,
  IOFS_OP_UTIMENS,
  IOFS_OP_WRITE_BUF,
  IOFS_OP_READ_BUF,
  IOFS_OP_FLOCK,
  IOFS_OP_FALLOCATE,
  IOFS_OP_COUNT
} iofs_op_t;

struct IofsPlugin {
  const char *(*get_name)(void);
  const char *(*get_version)(void);

  void *(*init)(void);
  void (*bind)(void *ctx);
  void (*destroy)(void *ctx);

  void (*record)(iofs_op_t op, uint64_t duration_ns, uint64_t units);
  size_t (*poll_prometheus_metrics)(char *buf, size_t buf_size);
};

struct IofsPlugin *get_iofs_plugin(void);

static inline int validate_iofs_plugin(const struct IofsPlugin *p) {
  if (!p || !p->init || !p->bind || !p->destroy || !p->record) return 0;
  return 1;
}

#ifdef __cplusplus
}
#endif

#endif
