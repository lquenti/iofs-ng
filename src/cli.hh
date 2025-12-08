#pragma once

#include <filesystem>

namespace fs = std::filesystem;

struct CliArgs {
  fs::path outfile{"/tmp/iofs.out"};
  fs::path logfile{"/tmp/iofs.log"};

  int verbosity{10};
  int interval{1};
  bool detailed_logging{true};
  bool use_allow_other{false};

  // positional args
  fs::path mountpoint;
  fs::path source_dir;
};

CliArgs parse_args(int argc, char **argv);
