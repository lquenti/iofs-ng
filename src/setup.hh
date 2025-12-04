/* parsing the command line options for iofs.c
 * might later include some config file handling
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#define ES_SERVER 0x100
#define ES_PORT 0x101
#define ES_URI 0x102
#define IN_SERVER 0x103
#define IN_DB 0x104
#define CSV_RW_PATH 0x105
#define IN_USERNAME 0x106
#define IN_PASSWORD 0x107

#define BUF_LEN 256

static error_t parse_opt(int , char *, struct argp_state *);

const char *argp_program_version = "iofs 0.8";

const char *argp_program_bug_address = "<hpc-support@gwdg.de>";

/* This structure is used by main to communicate with parse_opt. */
typedef struct options_t
{
  char *args[2];            /* ARG1 and ARG2 */
  char outfile[BUF_LEN];            /* Argument for -o */
  char logfile[BUF_LEN];            /* Argument for -o */
  char es_server[BUF_LEN], es_server_port[6], es_uri[BUF_LEN];
  char in_server[BUF_LEN], in_db[BUF_LEN], in_username[BUF_LEN], in_password[BUF_LEN], in_tags[BUF_LEN];
  int use_allow_other; /* actually a bool */
  int verbosity;
  int detailed_logging;
  int interval;
  char csv_rw_path[BUF_LEN];
} options_t;

static int append_tags(options_t *, char *);

// TODO think about whether I might have to default-initialize all of them
static options_t arguments = {
  .outfile = "/tmp/iofs.out",
  .logfile = "/tmp/iofs.log",
  .es_server = "",
  .in_server = "",
  .in_username = "",
  .use_allow_other = 0,
  .verbosity = 10,
  .detailed_logging = 1,
  .interval = 1,
  .csv_rw_path = "" // empty path means no sending out
};

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option arg_options[] = {
  {"verbosity", 'v', "10", 0, "Produce verbose output"},
  {"interval", 'i', "1", 0, "output interval in seconds"},
  {"logfile",  'l', "/tmp/iofs.log", 0, "location of logs"},
  {"outfile",  'O', "/tmp/iofs.out", 0, "location of data"},
  {"es-server", ES_SERVER, "http://localhost", 0, "Location of the elasticsearch server"},
  {"es-port", ES_PORT, "8086", 0, "Elasticsearch Port"},
  {"es-uri", ES_URI, "no clue", 0, "something"},
  {"in-server", IN_SERVER, "http://localhost:8086", 0, "Location of the influxdb server with port"},
  {"in-db", IN_DB, "moep", 0, "database name"},
  {"in-tags", 't', "cluster=hpc-1", 0, "Custom tags for InfluxDB"},
  {"in-username", IN_USERNAME, "myuser", 0, "Username for the influxdb"},
  {"in-password", IN_PASSWORD, "hunter2", 0, "Password for the influxdb"},
  {"allow-other", 'a', 0, 0, "Use allow_other, see man mount.fuse"},
  {"csv-rw-path", CSV_RW_PATH, "/tmp/iofs_all_rw.csv", 0, "Path to write out *all* unaggregated r/w I/O calls"},
  {0}
};


static char args_doc[] = "fuse-mountpont source-directory";
static char doc[] =
"IOFS -- The I/O file system - A FUSE file system developed for I/O monitoring";
static struct argp argp = {arg_options, parse_opt, args_doc, doc};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  options_t *arguments = static_cast<options_t *>(state->input);

  switch (key)
    {
    case 'v':
      arguments->verbosity = atoi(arg);
      break;
    case 'i':
      arguments->interval = atoi(arg);
      break;
    case 'a':
      arguments->use_allow_other = 1;
    case 'l':
      if (snprintf(arguments->logfile, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case 'O':
      if (snprintf(arguments->outfile, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case ES_SERVER:
      if (snprintf(arguments->es_server, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case ES_PORT:
      if (snprintf(arguments->es_server_port, 6, "%s", arg) > 6) {
        printf("Input argument %s bigger then %d. Aborting", arg, 6);
        return 1;
      }
      break;
    case ES_URI:
      if (snprintf(arguments->es_uri, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case IN_SERVER:
      if (snprintf(arguments->in_server, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case IN_DB:
      if (snprintf(arguments->in_db, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case IN_USERNAME:
      if (snprintf(arguments->in_username, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case IN_PASSWORD:
      if (snprintf(arguments->in_password, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case CSV_RW_PATH:
      if (snprintf(arguments->csv_rw_path, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", arg, BUF_LEN);
        return 1;
      }
      break;
    case 't': {
      //TODO: Do something if this fails
      int ret = append_tags(arguments, arg);
      assert(ret==0);
      break;
    }
    case ARGP_KEY_ARG: {
      if (state->arg_num >= 2)
        /* Too many arguments. */
        argp_usage (state);

      arguments->args[state->arg_num] = arg;
      break;
    }
    case ARGP_KEY_END: {
      if (state->arg_num < 2)
        /* Not enough arguments. */
        argp_usage (state);
      break;
    }
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static int append_tags(options_t *arguments, char *tags) {
  if (snprintf(
        arguments->in_tags + strlen(arguments->in_tags),
        BUF_LEN + strlen(arguments->in_tags), ",%s", tags)
      > BUF_LEN) {
    printf("Could not add tag %s, too many tags", tags);
    return 5;
  }
  return 0;
}
