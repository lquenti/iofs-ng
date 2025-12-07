#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

struct CliArgs {
  fs::path outfile{"/tmp/iofs.out"};
  fs::path logfile{"/tmp/iofs.log"};

  int verbosity{10};
  int interval{1};
  bool detailed_logging{true};
  bool use_allow_other{false};

  std::optional<std::string> es_server;
  std::string es_port{"8086"};
  std::string es_uri{"no clue"};

  std::optional<std::string> in_server;
  std::string in_db{"moep"};
  std::string in_username{"myuser"};
  std::string in_password{"hunter2"};
  std::vector<std::string> in_tags{"cluster=hpc-1"};

  std::optional<fs::path> csv_rw_path{"/tmp/iofs_all_rw.csv"};

  // positional args
  fs::path mountpoint;
  fs::path source_dir;
};

CliArgs parse_args(int argc, char **argv);
