#pragma once

// Use if you want to enable zero copy through splicing
#define USE_ZERO_COPY

#include <fcntl.h>
#include <sys/statvfs.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#define FUSE_USE_VERSION 36
#include <dirent.h>
#include <fuse.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <filesystem>

enum class IOOp {
  getattr,
  readlink,
  mkdir,
  unlink,
  rmdir,
  symlink,
  rename,
  link,
  chmod,
  chown,
  truncate,
  open,
  read,
  write,
  statfs,
  flush,
  release,
  fsync,
  setxattr,
  getxattr,
  listxattr,
  removexattr,
  opendir,
  readdir,
  releasedir,
  access,
  create,
  utimens,
  write_buf,
  read_buf,
  flock,
  fallocate
};

class TimerGuard {
 public:
  using clock_type = std::chrono::high_resolution_clock;
  explicit TimerGuard(IOOp op, size_t init_s = 1) : m_operation{op}, m_size{init_s}, m_start{clock_type::now()} {}
  ~TimerGuard();
  void update_size(size_t s);

 private:
  IOOp m_operation;
  size_t m_size;
  clock_type::time_point m_start;
};

// See `fuse_operations` struct definition for description on the operations
class IOFS {
 public:
  explicit IOFS(std::filesystem::path root) : m_source_root{std::move(root)} {}
  int getattr(const char *path, struct stat *stbuf, fuse_file_info *fi);
  int readlink(const char *path, char *buf, size_t size);
  int mkdir(const char *path, mode_t mode);
  int unlink(const char *path);
  int rmdir(const char *path);
  int symlink(const char *from, const char *to);
  int rename(const char *from, const char *to, unsigned int flags);
  int link(const char *from, const char *to);
  int chmod(const char *path, mode_t mode, fuse_file_info *fi);
  int chown(const char *path, uid_t uid, gid_t gid, fuse_file_info *fi);
  int truncate(const char *path, off_t size, fuse_file_info *fi);
  int open(const char *path, fuse_file_info *fi);
  int read(const char *path, char *buf, size_t size, off_t offset, fuse_file_info *fi);
  int write(const char *path, const char *buf, size_t size, off_t offset, fuse_file_info *fi);
  int statfs(const char *path, struct statvfs *stbuf);
  int flush(const char *path, fuse_file_info *fi);
  int release(const char *path, fuse_file_info *fi);
  int fsync(const char *path, int isdatasync, fuse_file_info *fi);
  int setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
  int getxattr(const char *path, const char *name, char *value, size_t size);
  int listxattr(const char *path, char *list, size_t size);
  int removexattr(const char *path, const char *name);
  int opendir(const char *path, fuse_file_info *fi);
  int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info *fi,
              fuse_readdir_flags flags);
  int releasedir(const char *path, fuse_file_info *fi);
  void *init(fuse_conn_info *conn, fuse_config *cfg);
  void destroy(void *private_data);
  int access(const char *path, int mask);
  int create(const char *path, mode_t mode, fuse_file_info *fi);
  int utimens(const char *path, const timespec ts[2], fuse_file_info *fi);
#ifdef USE_ZERO_COPY
  int write_buf(const char *path, fuse_bufvec *buf, off_t offset, fuse_file_info *fi);
  int read_buf(const char *path, fuse_bufvec **bufp, size_t size, off_t offset, fuse_file_info *fi);
#endif
  int flock(const char *path, fuse_file_info *fi, int op);
  int fallocate(const char *path, int mode, off_t offset, off_t length, fuse_file_info *fi);

 private:
  std::filesystem::path m_source_root;
  std::filesystem::path resolve_path(const char *path) const;
};
