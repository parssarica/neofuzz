/* Pars SARICA <pars@parssarica.com> */

#pragma once

#include <curl/curl.h>

#define NEOFUZZ_VERSION "Version 0.9.8 Neofuzz"

typedef struct
{
    sds base_url;
    sds status_codes_show;
    sds status_codes_hide;
    sds proxy;
    sds words;
    sds fwords;
    sds msize;
    sds fsize;
    sds mline;
    sds fline;
    sds user_agent;
    sds post_data;
    sds resolve_as;
    sds headers;
    sds cookie;
    sds method;
    int max_content_length;
    int max_lines;
    int timeout;
    int verbose;
    int socks_count;
    int error_count;
    int encode_option;
    int poll_mode;
    int show_error;
    int cpu_cores;
    int actual_cpu_cores;
    int disable_keep_alive;
    int delay;
} current_options;

extern current_options co;

typedef struct
{
    CURL *curl;
    sds url;
    sds content;
    sds post_data;
    char fuzz_word[1024];
    int state;
} t_socks;

#define SOCK_READY 0
#define SOCK_SENT 1
#define SOCK_RECV 2
#define SOCK_FILTER 3
#define SOCK_FINISHED 4

extern uint8_t *socks_pool;
extern int socks_pool_size;

void logo(void);
void help(void);
void version(void);
sds sds_replace(sds, char *, char *);
int line_count(sds);
int in_arguments(char *argv[], int argc, char *arg);
int parse_part(const char *part, int wordcount);
void start_fuzz(char *wordlist_file);
int print_co_stats(char *);
