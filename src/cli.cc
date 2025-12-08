#include "cli.hh"
#include <CLI11.hh>

CliArgs parse_args(int argc, char **argv) {
  CLI::App app{"iofs-ng - A FUSE file system developed for I/O monitoring"};
  app.set_version_flag("--version", "iofs-ng 0.9");
  app.footer("Bug reports: <https://github.com/lquenti/iofs-ng>");

  CliArgs args;
  app.add_option("-a,--allow-other", args.use_allow_other,"Use allow_other, see `man mount.fuse`");

  app.add_option("-l,--logfile", args.logfile, "Location of logs");
  app.add_option("-O,--outfile", args.outfile, "Location of data");

  app.add_option("mountpoint", args.mountpoint, "FUSE mountpoint")->required()->check(CLI::ExistingDirectory);
  app.add_option("source", args.source_dir, "Source directory")->required()->check(CLI::ExistingDirectory);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    std::exit(app.exit(e));
  }

  return args;
}
