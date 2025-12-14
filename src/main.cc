#include <filesystem>
#include <CLI11.hh>
#include <sys/stat.h>

#define FUSE_USE_VERSION 36
#include <fuse.h>

#include "iofs.hh"

namespace fs = std::filesystem;

struct CliArgs {
  bool use_allow_other{false};
  bool use_foreground{false};
  bool use_debug{false};

  // positional args
  fs::path mountpoint;
  fs::path source_dir;
};

CliArgs parse_args(int argc, char **argv) {
  CLI::App app{"iofs-ng - A FUSE file system developed for I/O monitoring"};
  app.set_version_flag("--version", "iofs-ng 0.9");
  app.footer("Bug reports: <https://github.com/lquenti/iofs-ng>");

  CliArgs args;
  app.add_flag("-a,--allow-other", args.use_allow_other, "Use allow_other, see `man mount.fuse`");
  app.add_flag("-f,--foreground", args.use_foreground, "Stay in foreground");
  app.add_flag("-d,--debug", args.use_debug, "Show FUSE debug logs");

  app.add_option("mountpoint", args.mountpoint, "FUSE mountpoint")->required()->check(CLI::ExistingDirectory);
  app.add_option("source", args.source_dir, "Source directory")->required()->check(CLI::ExistingDirectory);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    std::exit(app.exit(e));
  }

  return args;
}


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
#ifdef USE_ZERO_COPY
  .write_buf  = [](auto ...args) { return get_fs()->write_buf(args...); },
  .read_buf   = [](auto ...args) { return get_fs()->read_buf(args...); },
#endif
  .flock      = [](auto ...args) { return get_fs()->flock(args...); },
  .fallocate  = [](auto ...args) { return get_fs()->fallocate(args...); },
};



int main(int argc, char **argv) {
  CliArgs arguments{parse_args(argc, argv)};

  IOFS fs_instance{arguments.source_dir};

  umask(0);

  std::string mountpoint_str = arguments.mountpoint.string();

  // They are stack-allocated as FUSE doesn't accept const char*
  char arg_fg[] = "-f";
  char arg_dbg[] = "-d";
  char arg_opt_kern[] = "-o";
  char arg_opt_allow[] = "allow_other";

  std::vector<char*> fuse_args;
  fuse_args.push_back(argv[0]);                  // Program name
  fuse_args.push_back(mountpoint_str.data());    // Mountpoint
  if (arguments.use_foreground) {
    fuse_args.push_back(arg_fg);
  }
  if (arguments.use_debug) {
    fuse_args.push_back(arg_dbg);
  }
  if (arguments.use_allow_other) {
      fuse_args.push_back(arg_opt_kern);
      fuse_args.push_back(arg_opt_allow);
  }
  int ret = fuse_main(static_cast<int>(fuse_args.size()), fuse_args.data(), &iofs_oper, &fs_instance);
  return ret;
}
