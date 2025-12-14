// Use if you want to enable zero copy through splicing
#define USE_ZERO_COPY

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/statvfs.h>
#define FUSE_USE_VERSION 36
#include <filesystem>
#include <fuse.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/xattr.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/file.h>
#include <print>

enum class IOOp {
  getattr, readlink, mkdir, unlink, rmdir, symlink, rename, link, chmod, chown, truncate, open, read, write,
  statfs, flush, release, fsync, setxattr, getxattr, listxattr, removexattr, opendir, readdir, releasedir,
  access, create, utimens, write_buf, read_buf, flock, fallocate
};

class TimerGuard {
  public:
    using clock_type = std::chrono::high_resolution_clock;
    explicit TimerGuard(IOOp op, size_t init_s=1) : m_operation{op}, m_size{init_s}, m_start{clock_type::now()} {}
    ~TimerGuard();
    void update_size(size_t s);
  private:
    IOOp m_operation;
    size_t m_size;
    clock_type::time_point m_start;
};

TimerGuard::~TimerGuard() {
  auto end{clock_type::now()};
  auto dur{std::chrono::duration_cast<std::chrono::nanoseconds>(end-m_start)};
  // TODO record metric here (only if it wasnt done 0 times)
}

void TimerGuard::update_size(size_t s) {
  m_size=s;
}


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

static IOFS *get_fs() {
  return static_cast<IOFS *>(fuse_get_context()->private_data);
}

struct fuse_operations iofs_oper = {
  .getattr    = [](auto ...args) { return get_fs()->getattr(args...); },
  .readlink   = [](auto ...args) { return get_fs()->readlink(args...); },
  // .mknod   = nullptr,
  .mkdir      = [](auto ...args) { return get_fs()->mkdir(args...); },
  .unlink     = [](auto ...args) { return get_fs()->unlink(args...); },
  .rmdir      = [](auto ...args) { return get_fs()->rmdir(args...); },
  .symlink    = [](auto ...args) { return get_fs()->symlink(args...); },
  .rename     = [](auto ...args) { return get_fs()->rename(args...); },
  .link       = [](auto ...args) { return get_fs()->link(args...); },
  .chmod      = [](auto ...args) { return get_fs()->chmod(args...); },
  .chown      = [](auto ...args) { return get_fs()->chown(args...); },
  .truncate   = [](auto ...args) { return get_fs()->truncate(args...); },
  .open       = [](auto ...args) { return get_fs()->open(args...); },
  .read       = [](auto ...args) { return get_fs()->read(args...); },
  .write      = [](auto ...args) { return get_fs()->write(args...); },
  .statfs     = [](auto ...args) { return get_fs()->statfs(args...); },
  .flush      = [](auto ...args) { return get_fs()->flush(args...); },
  .release    = [](auto ...args) { return get_fs()->release(args...); },
  .fsync      = [](auto ...args) { return get_fs()->fsync(args...); },
  .setxattr   = [](auto ...args) { return get_fs()->setxattr(args...); },
  .getxattr   = [](auto ...args) { return get_fs()->getxattr(args...); },
  .listxattr  = [](auto ...args) { return get_fs()->listxattr(args...); },
  .removexattr= [](auto ...args) { return get_fs()->removexattr(args...); },
  .opendir    = [](auto ...args) { return get_fs()->opendir(args...); },
  .readdir    = [](auto ...args) { return get_fs()->readdir(args...); },
  .releasedir = [](auto ...args) { return get_fs()->releasedir(args...); },
  // .fsyncdir = nullptr,
  .init       = [](auto ...args) { return get_fs()->init(args...); },
  .destroy    = [](auto ...args) { get_fs()->destroy(args...); },
  .access     = [](auto ...args) { return get_fs()->access(args...); },
  .create     = [](auto ...args) { return get_fs()->create(args...); },
  // .lock    = nullptr, /* POSIX lock, distinct from flock */
  .utimens    = [](auto ...args) { return get_fs()->utimens(args...); },
  // .bmap    = nullptr,
  // .ioctl   = nullptr,
  // .poll    = nullptr,
  .write_buf  = [](auto ...args) { return get_fs()->write_buf(args...); },
  .read_buf   = [](auto ...args) { return get_fs()->read_buf(args...); },
  .flock      = [](auto ...args) { return get_fs()->flock(args...); },
  .fallocate  = [](auto ...args) { return get_fs()->fallocate(args...); },
};

int IOFS::getattr(const char *path, struct stat *stbuf, [[maybe_unused]] fuse_file_info *fi) {
  TimerGuard timer{IOOp::getattr};
  auto full_path{resolve_path(path)};
  int res{lstat(full_path.c_str(), stbuf)};
  return (res == -1) ? -errno : 0;
}

int IOFS::readlink(const char *path, char *buf, size_t size) {
  TimerGuard timer{IOOp::readlink};
  auto full_path{resolve_path(path)};
  ssize_t res{::readlink(full_path.c_str(), buf, size-1)};
  if (res==-1) {
    return -errno;
  }
  buf[res] = '\0';
  return 0;
}

int IOFS::mkdir(const char *path, mode_t mode) {
  TimerGuard timer{IOOp::mkdir};
  auto full_path{resolve_path(path)};
  int res{::mkdir(full_path.c_str(), mode)};
  return (res == -1) ? -errno : 0;
}

int IOFS::unlink(const char *path) {
  TimerGuard timer{IOOp::unlink};
  auto full_path{resolve_path(path)};
  int res{::unlink(full_path.c_str())};
  return (res == -1) ? -errno : 0;
}

int IOFS::rmdir(const char *path) {
  TimerGuard timer{IOOp::rmdir};
  auto full_path{resolve_path(path)};
  int res{::rmdir(full_path.c_str())};
  return (res == -1) ? -errno : 0;
}

int IOFS::symlink(const char *from, const char *to) {
  TimerGuard timer{IOOp::symlink};
  auto full_path1{resolve_path(from)}, full_path2{resolve_path(to)};
  int res{::symlink(full_path1.c_str(), full_path2.c_str())};
  return (res==-1) ? -errno : 0;
}

int IOFS::rename(const char *from, const char *to, unsigned int flags) {
  TimerGuard timer{IOOp::rename};
  auto full_path1{resolve_path(from)}, full_path2{resolve_path(to)};
  // AT_FDCWD works since the paths are absolute
  int res{::renameat2(AT_FDCWD, full_path1.c_str(), AT_FDCWD, full_path2.c_str(), flags)};
  return (res==-1) ? -errno : 0;
}

int IOFS::link(const char *from, const char *to) {
  TimerGuard timer{IOOp::link};
  auto full_path1{resolve_path(from)}, full_path2{resolve_path(to)};
  int res{::link(full_path1.c_str(), full_path2.c_str())};
  return (res==-1) ? -errno : 0;
}

int IOFS::chmod(const char *path, mode_t mode, [[maybe_unused]] fuse_file_info *fi) {
  TimerGuard timer{IOOp::chmod};
  auto full_path{resolve_path(path)};
  int res{::chmod(full_path.c_str(), mode)};
  return (res == -1) ? -errno : 0;
}

int IOFS::chown(const char *path, uid_t uid, gid_t gid, [[maybe_unused]] fuse_file_info *fi) {
  TimerGuard timer{IOOp::chown};
  auto full_path{resolve_path(path)};
  int res{::lchown(full_path.c_str(), uid, gid)};
  return (res==-1) ? -errno : 0;
}

int IOFS::truncate(const char *path, off_t size, [[maybe_unused]] fuse_file_info *fi) {
  TimerGuard timer{IOOp::truncate};
  auto full_path{resolve_path(path)};
  int res{::truncate(full_path.c_str(), size)};
  return (res==-1) ? -errno : 0;
}

int IOFS::open(const char *path, fuse_file_info *fi) {
  TimerGuard timer{IOOp::open};
  auto full_path{resolve_path(path)};
  int fd{::open(full_path.c_str(), fi->flags)};
  if (fd == -1) {
    return -errno;
  }
  fi->fh = fd;
  return 0;
}

int IOFS::read([[maybe_unused]] const char *path, char *buf, size_t size, off_t offset, fuse_file_info *fi) {
  TimerGuard timer{IOOp::read, 0};
  ssize_t res{::pread(fi->fh, buf, size, offset)};
  if (res == -1) {
    return -errno;
  }
  timer.update_size(static_cast<size_t>(res));
  return static_cast<int>(res);
}

int IOFS::write([[maybe_unused]] const char *path, const char *buf, size_t size, off_t offset, fuse_file_info *fi) {
  TimerGuard timer{IOOp::write, 0};
  ssize_t res{::pwrite(fi->fh, buf, size, offset)};
  if (res == -1) {
    return -errno;
  }
  timer.update_size(static_cast<size_t>(res));
  return static_cast<int>(res);
}

int IOFS::statfs(const char *path, struct statvfs *stbuf) {
  TimerGuard timer{IOOp::statfs};
  auto full_path{resolve_path(path)};
  int res{::statvfs(path, stbuf)};
  return (res == -1) ? -errno : 0;
}

int IOFS::flush([[maybe_unused]] const char *path, fuse_file_info *fi) {
  TimerGuard timer{IOOp::flush};
  /* This is called from every close on an open file, so call the
     close on the underlying filesystem.	But since flush may be
     called multiple times for an open file, this must not really
     close the file.  This is important if used on a network
     filesystem like NFS which flush the data/metadata on close() */
  int res{::close(::dup(fi->fh))};
  return (res == -1) ? -errno : 0;
}

int IOFS::release([[maybe_unused]] const char *path, fuse_file_info *fi) {
  TimerGuard timer{IOOp::release};
  ::close(fi->fh);
  return 0;
}

int IOFS::fsync([[maybe_unused]] const char *path, int isdatasync, fuse_file_info *fi) {
  TimerGuard timer{IOOp::fsync};
  int res;
  if (isdatasync) {
    res = ::fdatasync(fi->fh);
  } else {
    res = ::fsync(fi->fh);
  }
  return (res == -1) ? -errno : 0;
}

int IOFS::setxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
  TimerGuard timer{IOOp::setxattr};
  auto full_path{resolve_path(path)};
  int res{::lsetxattr(full_path.c_str(), name, value, size, flags)};
  return (res == -1) ? -errno : 0;
}

int IOFS::getxattr(const char *path, const char *name, char *value, size_t size) {
  TimerGuard timer{IOOp::getxattr};
  auto full_path{resolve_path(path)};
  ssize_t res{::lgetxattr(full_path.c_str(), name, value, size)};
  return (res == -1) ? -errno : static_cast<int>(res);
}

int IOFS::listxattr(const char *path, char *list, size_t size) {
  TimerGuard timer{IOOp::listxattr};
  auto full_path{resolve_path(path)};
  ssize_t res{::listxattr(full_path.c_str(), list, size)};
  return (res == -1) ? -errno : static_cast<int>(res);
}

int IOFS::removexattr(const char *path, const char *name) {
  TimerGuard timer{IOOp::removexattr};
  auto full_path{resolve_path(path)};
  int res{::lremovexattr(full_path.c_str(), name)};
  return (res == -1) ? -errno : 0;
}

struct DirHandle {
    DIR *dp{nullptr};
    struct dirent *entry{nullptr};
    off_t offset{0};
    ~DirHandle() {
        if (dp) {
          closedir(dp);
        }
    }
};

static DirHandle *get_dir_handle(fuse_file_info *fi) {
    return reinterpret_cast<DirHandle*>(fi->fh);
}

int IOFS::opendir(const char *path, fuse_file_info *fi) {
  std::unique_ptr<DirHandle> d{std::make_unique<DirHandle>()};
  {
    TimerGuard timer{IOOp::opendir};
    auto full_path{resolve_path(path)};
    d->dp = ::opendir(full_path.c_str());
  } // Make the timer guard commit early
  if (!d->dp) {
    return -errno;
  }
  // Give ownership to FUSE (taking it back at releasedir)
  fi->fh = reinterpret_cast<uint64_t>(d.release());
  return 0;
}

int IOFS::readdir([[maybe_unused]] const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
    fuse_file_info *fi, fuse_readdir_flags flags) {
  TimerGuard timer{IOOp::readdir};

  // stored in opendir
  DirHandle *d{get_dir_handle(fi)};

  // Seek if fuse asks an offset where we didnt stop before
  // Note: Since std::filesystem doesnt offer a RandomIterator, it makes sense to stick with the C API here, even
  // though we aim to "modernize" it...
  if (offset != d->offset) {
    ::seekdir(d->dp, offset);
    d->entry = nullptr;
    d->offset = offset;
  }

  while (true) {
    // Read nexxt entry
    if (!d->entry) {
      d->entry = ::readdir(d->dp);
      // stop if end of directory
      if (!d->entry) {
        break;
      }
    }

    // Check if we are in Plus Mode. `enum fuse_readdir_flags` describes it as follows:
    //
	  // "Plus" mode.
    //
	  // The kernel wants to prefill the inode cache during readdir.  The
	  // filesystem may honour this by filling in the attributes and setting
	  // FUSE_FILL_DIR_FLAGS for the filler function.  The filesystem may also
	  // just ignore this flag completely.
    //
    // As I understand it, the idea is that Plus mode already pre-fetches the metadata, so that it doesnt need a
    // full getattr/stat call later...
    //
    // Furthermore, I think we could just set it everytime, as `enum fuse_fill_dir_flags` says that
    //
	  // It is okay to set FUSE_FILL_DIR_PLUS if FUSE_READDIR_PLUS is not set
	  // and vice versa.
    //
    // But, in line with Chesterton's Fence, we won't touch it until I got a feel for readdir and we have proper
    // stress/fuzz/correctness testing
    struct stat st{}; /* zero-init through value init */
    enum fuse_fill_dir_flags fill_flags{FUSE_FILL_DIR_DEFAULTS};
    if (flags & FUSE_READDIR_PLUS) {
      int res{::fstatat(dirfd(d->dp), d->entry->d_name, &st, AT_SYMLINK_NOFOLLOW)};
      if (res != -1) {
        // Telling the fs that we successfully filled it!!!
        fill_flags = static_cast<fuse_fill_dir_flags>(fill_flags | FUSE_FILL_DIR_PLUS);
      }
    }

    // If no Plus mode, or fstatat failed, we fill it with minimal mock info
    if (!(fill_flags & FUSE_FILL_DIR_PLUS)) {
      st.st_ino = d->entry->d_ino;
      st.st_mode = d->entry->d_type << 12; // ??
    }

    // get offset of *next* entry (as we processed the last one from a POSIX perspective)
    off_t nextoff = ::telldir(d->dp);

    // To quote `fuse_operations::readdir`
	  // The filesystem may choose between two modes of operation:
    // ...
	  // 2) The readdir implementation keeps track of the offsets of the
	  // directory entries.  It uses the offset parameter and always
	  // passes non-zero offset to the filler function.  When the buffer
	  // is full (or an error happens) the filler function will return
	  // '1'.
    if (filler(buf, d->entry->d_name, &st, nextoff, fill_flags)) {
      break;
    }

    // prepare for next entry
    d->entry = nullptr;
    d->offset = nextoff;
  }
  return 0;
}

int IOFS::releasedir(const char *path, fuse_file_info *fi) {
  TimerGuard timer{IOOp::releasedir};
  // re-take ownership (released in opendir) to get RAII cleanup
  std::unique_ptr<DirHandle> d{reinterpret_cast<DirHandle *>(fi->fh)};
  return 0;
}

void *IOFS::init (fuse_conn_info *conn, fuse_config *cfg) {
  // The initing of the IOFS object (i.e. the construction) already happens in main
  // (passed to FUSE via `user_data` parameter of `fuse_main`)
  // I think its cleaner to handle IOFS creation problems *before* already being in FUSE space with background logs...

  // see documentation of options in fuse.h
  // cfg->direct_io = 1;
  // cfg->kernel_cache = 1;
  cfg->auto_cache = 0;

  std::println("IOFS init");
  std::println("intr: {}", cfg->intr);
  std::println("remember: {}", cfg->remember);
  std::println("hard_remove: {}", cfg->hard_remove);
  std::println("use_ino: {}", cfg->use_ino);
  std::println("readdir_ino: {}", cfg->readdir_ino);
  std::println("direct_io: {}", cfg->direct_io);
  std::println("kernel_cache: {}", cfg->kernel_cache);
  std::println("auto_cache: {}", cfg->auto_cache);
  std::println("ac_attr_timeout_set: {}", cfg->ac_attr_timeout_set);
  std::println("nullpath_ok: {}", cfg->nullpath_ok);

  std::println("ac_attr_timeout: {}", cfg->ac_attr_timeout);
  std::println("entry_timeout: {}", cfg->entry_timeout);
  std::println("negative_timeout: {}", cfg->negative_timeout);
  std::println("attr_timeout: {}", cfg->attr_timeout);

  return this;
}

void IOFS::destroy([[maybe_unused]] void *private_data) {
  // ~IOFS is called at end of `main`...
}

int IOFS::access(const char *path, int mask) {
  TimerGuard timer{IOOp::access};
  auto full_path{resolve_path(path)};
  int res{::access(full_path.c_str(), mask)};
  return (res == -1) ? -errno : 0;
}

int IOFS::create(const char *path, mode_t mode, fuse_file_info *fi) {
  TimerGuard timer{IOOp::create};
  auto full_path{resolve_path(path)};
  int fd{::open(full_path.c_str(), fi->flags, mode)};
  if (fd == -1) {
    return -errno;
  }
  fi->fh = fd;
  return 0;
}

int IOFS::utimens(const char *path, const timespec ts[2], [[maybe_unused]] fuse_file_info *fi) {
  TimerGuard timer{IOOp::utimens};
  auto full_path{resolve_path(path)};
  /* don't use utime/utimes since they follow symlinks */
  int res{::utimensat(0, full_path.c_str(), ts, AT_SYMLINK_NOFOLLOW)};
  return (res == -1) ? -errno : 0;
}

int IOFS::write_buf([[maybe_unused]] const char *path, fuse_bufvec *buf, off_t offset, fuse_file_info *fi) {
  TimerGuard timer{IOOp::write_buf, 0};
  size_t size{fuse_buf_size(buf)};
  struct fuse_bufvec dst = FUSE_BUFVEC_INIT(size);
  dst.buf[0].flags = static_cast<fuse_buf_flags>(FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
  dst.buf[0].fd = fi->fh;
  dst.buf[0].pos = offset;
  ssize_t res{fuse_buf_copy(&dst, buf, FUSE_BUF_SPLICE_NONBLOCK)};
  if (res >= 0) {
    timer.update_size(static_cast<size_t>(res));
  }
  return static_cast<int>(res);
}

int IOFS::read_buf([[maybe_unused]] const char *path, fuse_bufvec **bufp, size_t size, off_t offset,
    fuse_file_info *fi) {
  TimerGuard timer{IOOp::read_buf, 0};
  // Use malloc, as FUSE will use free, not delete
  auto *src{static_cast<struct fuse_bufvec *>(std::malloc(sizeof(struct fuse_bufvec)))};
  if (!src) {
    return -ENOMEM;
  }
  timer.update_size(size);
  *src = FUSE_BUFVEC_INIT(size);
  src->buf[0].flags = static_cast<fuse_buf_flags>(FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
  src->buf[0].fd = fi->fh;
  src->buf[0].pos = offset;
  *bufp = src;
  return 0;
}

int IOFS::flock([[maybe_unused]] const char *path, fuse_file_info *fi, int op) {
  TimerGuard timer{IOOp::flock};
  int res{::flock(fi->fh, op)};
  return (res == -1) ? -errno : 0;
}
int IOFS::fallocate([[maybe_unused]] const char *path, int mode, off_t offset, off_t length, fuse_file_info *fi) {
  if (mode) {
    return -EOPNOTSUPP;
  }
  TimerGuard timer{IOOp::fallocate};
  int err{::posix_fallocate(fi->fh, offset, length)};
  return -err;
}

std::filesystem::path IOFS::resolve_path(const char *path) const {
  return m_source_root / std::filesystem::path(path).relative_path();
}
