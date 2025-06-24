/* Pars SARICA <pars@parssarica.com> */

#include <assert.h>
#include <ctype.h>
#include <curl/curl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sds.h"

#include "neofuzz.h"

current_options co;

int print_co_stats(char *wordlist_file)
{
    logo();
    printf("────────────────────────────────────────────\n");
    printf("\nTarget URL: %s\n", co.base_url);
    if (strcmp(wordlist_file, "") != 0)
        printf("Word list: %s\n", wordlist_file);
    if (strcmp(co.post_data, "") != 0)
        printf("Action: POST\n");
    else if (strcmp(co.method, "") != 0)
        printf("Action: %s\n", co.method);
    else
        printf("Action: GET\n");
    if (strcmp(co.status_codes_hide, "") == 0 &&
        strcmp(co.status_codes_show, "") != 0)
        printf("Status codes to show: %s\n", co.status_codes_show);
    if (strcmp(co.status_codes_show, "") == 0 &&
        strcmp(co.status_codes_hide, "") != 0)
        printf("Status codes to hide: %s\n", co.status_codes_hide);
    if (sdslen(co.words) != 0)
        printf("Word counts to match: %s\n", co.words);
    if (sdslen(co.fwords) != 0)
        printf("Word counts to filter: %s\n", co.fwords);
    if (sdslen(co.msize) != 0)
        printf("Sizes to match: %s\n", co.msize);
    if (sdslen(co.fsize) != 0)
        printf("Sizes to filter: %s\n", co.fsize);
    if (sdslen(co.mline) != 0)
        printf("Line counts to match: %s\n", co.mline);
    if (sdslen(co.fline) != 0)
        printf("Line counts to filter: %s\n", co.fline);

    printf("Parallel socket count: %d\n", co.socks_count);
    printf("CPU cores to use / Thread count: %d\n", co.actual_cpu_cores);
    printf("User agent: %s\n", co.user_agent);

    if (co.verbose)
        printf("Verbose mode: Enabled\n");
    if (co.poll_mode)
        printf("Poll Mode: Enabled\n");
    if (co.disable_keep_alive)
        printf("Keep TCP connection alive: Disabled\n");
    if (co.encode_option)
        printf("Encoding urls: Enabled\n");

    printf("\n────────────────────────────────────────────\n\n");

    return 0;
}

int main(int argc, char *argv[])
{
    int opt, option_index, mc_used = 0, fc_used = 0;
    sds wordlist_file = sdsempty();
    long num_cores;

    co.base_url = sdsempty();
    // co.status_codes_show = sdsnew("200,204,301,302,307,401,403");
    co.status_codes_show = sdsempty();
    co.status_codes_hide = sdsempty();
    co.proxy = sdsempty();
    co.words = sdsempty();
    co.fwords = sdsempty();
    co.msize = sdsempty();
    co.fsize = sdsempty();
    co.mline = sdsempty();
    co.fline = sdsempty();
    co.post_data = sdsempty();
    co.resolve_as = sdsempty();
    co.headers = sdsempty();
    co.cookie = sdsempty();
    co.method = sdsempty();
    co.user_agent =
        sdsnew("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
               "(KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.3");
    co.max_content_length = -1;
    co.timeout = 10;
    co.verbose = 0;
    co.poll_mode = 0;
    co.encode_option = 0;
    co.cpu_cores = 0;
    co.socks_count = 50;
    co.delay = 0;

    if (argc == 1)
    {
        help();
        goto exit;
    }

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {"url", required_argument, NULL, 'u'},
        {"word-list", required_argument, NULL, 'w'},
        {"verbose", no_argument, NULL, 'v'},
        {"encode", no_argument, NULL, 'e'},
        {"poll-mode", no_argument, NULL, 0},
        {"user-agent", required_argument, NULL, 0},
        {"num-cpu-cores", required_argument, NULL, 0},
        {"proxy", required_argument, NULL, 'x'},
        {"timeout", required_argument, NULL, 0},
        {"mc", required_argument, NULL, 0},
        {"fc", required_argument, NULL, 0},
        {"mw", required_argument, NULL, 0},
        {"fw", required_argument, NULL, 0},
        {"ms", required_argument, NULL, 0},
        {"fs", required_argument, NULL, 0},
        {"ml", required_argument, NULL, 0},
        {"fl", required_argument, NULL, 0},
        {"post-data", required_argument, NULL, 'd'},
        {"show-error", no_argument, NULL, 0},
        {"resolve-as", required_argument, NULL, 0},
        {"disable-keep-alive", no_argument, NULL, 0},
        {"headers", required_argument, NULL, 'H'},
        {"cookie", required_argument, NULL, 'b'},
        {"http-verb", required_argument, NULL, 'X'},
        {"wait", required_argument, NULL, 0},
        {NULL, 0, NULL, 0}};

    while ((opt = getopt_long(argc, argv, "hVu:w:vep:d:x:H:b:X:", long_options,
                              &option_index)) != -1)
    {
        switch (opt)
        {
        case 'h':
            help();
            goto exit;
            break;
        case 'V':
            version();
            goto exit;
            break;
        case 'u':
            co.base_url = sdscpylen(co.base_url, optarg, strlen(optarg));
            break;
        case 'w':
            wordlist_file = sdscpylen(wordlist_file, optarg, strlen(optarg));
            break;
        case 'x':
            co.proxy = sdscpylen(co.proxy, optarg, strlen(optarg));
            break;
        case 'p':
            co.socks_count = atoi(optarg);
            if (co.socks_count <= 0)
            {
                printf("Parallel sockets must be an integer greater than 0.\n");
                goto exit;
            }
            break;
        case 'd':
            co.post_data = sdscpylen(co.post_data, optarg, strlen(optarg));
            break;
        case 'e':
            co.encode_option = 1;
            break;
        case 'v':
            co.verbose = 1;
            break;
        case 'H':
            co.headers = sdscatlen(co.headers, optarg, strlen(optarg));
            co.headers = sdscatlen(co.headers, "\n", 1);
            break;
        case 'b':
            co.cookie = sdscpylen(co.cookie, optarg, strlen(optarg));
            break;
        case 'X':
            co.method = sdscpylen(co.method, optarg, strlen(optarg));
            break;
        case 0: // Long options
            if (strcmp(long_options[option_index].name, "user-agent") == 0)
            {
                co.user_agent =
                    sdscpylen(co.user_agent, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "num-cpu-cores") ==
                     0)
            {
                co.cpu_cores = atoi(optarg);
            }
            else if (strcmp(long_options[option_index].name, "url") == 0)
            {
                co.base_url = sdscpylen(co.base_url, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "word-list") == 0)
            {
                wordlist_file =
                    sdscpylen(wordlist_file, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "proxy") == 0)
            {
                co.proxy = sdscpylen(co.proxy, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "timeout") == 0)
            {
                if (atoi(optarg) > 0 && atoi(optarg) < 3000000)
                {
                    co.timeout = atoi(optarg);
                }
                else
                {
                    printf("Timeout must be greater than 0 and smaller than "
                           "3.000.000.\n");
                    goto exit;
                }
            }
            else if (strcmp(long_options[option_index].name, "show-error") == 0)
            {
                co.show_error = 1;
            }
            else if (strcmp(long_options[option_index].name, "ml") == 0)
            {
                co.mline = sdscpylen(co.mline, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "fl") == 0)
            {
                co.fline = sdscpylen(co.fline, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "post-data") == 0)
            {
                co.post_data = sdscpylen(co.post_data, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "poll-mode") == 0)
            {
                co.poll_mode = 1;
            }
            else if (strcmp(long_options[option_index].name,
                            "disable-keep-alive") == 0)
            {
                co.headers = sdscatlen(co.headers, "Connection: close\n", 18);
                co.disable_keep_alive = 1;
            }
            else if (strcmp(long_options[option_index].name, "resolve-as") == 0)
            {
                co.resolve_as =
                    sdscpylen(co.resolve_as, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "http-verb") == 0)
            {
                co.method = sdscpylen(co.method, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "mc") == 0)
            {
                co.status_codes_show =
                    sdscpylen(co.status_codes_show, optarg, strlen(optarg));
                mc_used = 1;
            }
            else if (strcmp(long_options[option_index].name, "fc") == 0)
            {
                co.status_codes_hide =
                    sdscpylen(co.status_codes_hide, optarg, strlen(optarg));
                fc_used = 1;
            }
            else if (strcmp(long_options[option_index].name, "mw") == 0)
            {
                co.words = sdscpylen(co.words, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "fw") == 0)
            {
                co.fwords = sdscpylen(co.fwords, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "ms") == 0)
            {
                co.msize = sdscpylen(co.msize, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "fs") == 0)
            {
                co.fsize = sdscpylen(co.fsize, optarg, strlen(optarg));
            }
            else if (strcmp(long_options[option_index].name, "wait") == 0)
            {
                co.delay = atoi(optarg);
            }
            else if (strcmp(long_options[option_index].name, "encode") == 0)
            {
                co.encode_option = 1;
            }
            else if (strcmp(long_options[option_index].name, "header") == 0)
            {
                co.headers = sdscatlen(co.headers, optarg, strlen(optarg));
                co.headers = sdscatlen(co.headers, "\n", 1);
            }
            break;
        case '?':
            // fprintf(stderr, "Unknown option: %s\n", argv[option_index]);
            // goto exit;
            break;
        default:
            help();
            goto exit;
        }
    }

    // Check for mandatory arguments
    if (!sdslen(co.base_url))
    {
        fprintf(stderr, "Error: -u/--url is a mandatory argument.\n");
        goto exit;
    }
    if (!sdslen(wordlist_file))
    {
        fprintf(stderr, "Error: -w/--word-list is a mandatory argument.\n");
        goto exit;
    }

    if (sdslen(co.status_codes_show) && sdslen(co.status_codes_hide))
    {
        printf("Error: --mc and --fc cannot be specified at the same time.\n");
        goto exit;
    }

    if (!mc_used && !fc_used)
    {
        co.status_codes_show =
            sdscpylen(co.status_codes_show, "200,204,301,302,307,401,403", 27);
    }

    if (sdslen(co.msize) && sdslen(co.fsize))
    {
        printf("Error: --ms and --fs cannot be specified at the same time.\n");
        goto exit;
    }

    if (sdslen(co.words) && sdslen(co.fwords))
    {
        printf("Error: --mw and --fw cannot be specified at the same time.\n");
        goto exit;
    }

    if (sdslen(co.mline) && sdslen(co.fline))
    {
        printf("Error: --ml and --fl cannot be specified at the same time.\n");
        goto exit;
    }

    num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores == -1)
    {
        perror("sysconf");
        goto exit;
    }

    co.actual_cpu_cores = (co.cpu_cores > 0) ? co.cpu_cores : num_cores;

    if (co.actual_cpu_cores > co.socks_count)
    {
        co.actual_cpu_cores = co.socks_count;
    }

    if (!strstr(co.base_url, "FUZZ"))
    {
        printf("FUZZ placeholder missing in the URL.\n");
        goto exit;
    }

    print_co_stats(wordlist_file);
    start_fuzz((char *)wordlist_file);

exit:
    sdsfree(co.base_url);
    sdsfree(co.status_codes_show);
    sdsfree(co.status_codes_hide);
    sdsfree(co.proxy);
    sdsfree(co.words);
    sdsfree(co.fwords);
    sdsfree(co.msize);
    sdsfree(co.fsize);
    sdsfree(co.mline);
    sdsfree(co.fline);
    sdsfree(co.user_agent);
    sdsfree(co.post_data);
    sdsfree(co.resolve_as);
    sdsfree(co.headers);
    sdsfree(co.cookie);
    sdsfree(co.method);
    sdsfree(wordlist_file);

    return 0;
}
