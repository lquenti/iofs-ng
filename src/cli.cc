#include "cli.hh"
#include <CLI11.hh>

CliArgs parse_args(int argc, char **argv) {
  CLI::App app{"iofs-ng - A FUSE file system developed for I/O monitoring"};
  app.set_version_flag("--version", "iofs-ng 0.9");
  app.footer("Bug reports: <https://github.com/lquenti/iofs-ng>");

  CliArgs args;
  app.add_option("-v,--verbosity", args.verbosity, "Produce verbose output");
  app.add_option("-i,--interval", args.interval, "Output interval in seconds");
  app.add_option("-a,--allow-other", args.use_allow_other,"Use allow_other, see `man mount.fuse`");

  app.add_option("-l,--logfile", args.logfile, "Location of logs");
  app.add_option("-O,--outfile", args.outfile, "Location of data");

  auto* es = app.add_option_group("Elasticsearch", "Elasticsearch Configuration");
  es->add_option("--es-server", args.es_server, "Location of the Elasticsearch server")
    ->default_str("http://localhost");
  es->add_option("--es-port", args.es_port, "Elasticsearch Port");
  es->add_option("--es-uri", args.es_uri, "ES URI");

  auto* inf = app.add_option_group("InfluxDB", "InfluxDB Configuration");
  inf->add_option("--in-server", args.in_server, "Location of the influxdb server")
    ->default_str("http://localhost:8086");
  inf->add_option("--in-db", args.in_db, "Database name");
  inf->add_option("--in-username", args.in_username, "Username");
  inf->add_option("--in-password", args.in_password, "Password");

  inf->add_option("-t,--in-tags", args.in_tags, "Custom tags for InfluxDB");

  app.add_option("--csv-rw-path", args.csv_rw_path, "Path to write out *all* unaggregated r/w I/O calls");

  app.add_option("mountpoint", args.mountpoint, "FUSE mountpoint")->required()->check(CLI::ExistingDirectory);
  app.add_option("source", args.source_dir, "Source directory")->required()->check(CLI::ExistingDirectory);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    std::exit(app.exit(e));
  }

  return args;
}
