/* Pars SARICA <pars@parssarica.com> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sds.h"

#include "neofuzz.h"

void logo(void)
{
    printf("                  __               \n");
    printf("                 / _|              \n");
    printf(" _ __   ___  ___ | |_ _   _ ________\n");
    printf("| '_ \\ / _ \\/ _ \\|  _| | | |_  /_  /\n");
    printf("| | | |  __/ (_) | | | |_| |/ / / / \n");
    printf("|_| |_|\\___|\\___/|_|  \\__,_/___/___|\n\n");
}

void help(void)
{
    version();
    printf("neofuzz <args>\n");
    printf("\nOptions:\n");
    printf("  -h/--help\t Shows this help message and exit\n");
    printf("  -v/--version\t Shows version\n");
    /* Mandatory */
    printf("\nMandatory:\n");
    printf("  -u/--url\t Specifies the url\n");
    printf("  -w/--word-list Specifies the wordlist\n");

    /* Optional */
    printf("\nOptional:\n");
    printf("  -H/--headers\t Specifies the custom headers\n");
    printf("  -b/--cookie\t Specifies the custom cookies\n");
    printf("  -x/--proxy\t Set proxy\n");
    printf("  -e/--encode\t HTTP encode url\n");

    printf("  -d/--post-data Specifies the post data\n");
    printf("  -p\t\t Specifies parallel socket count "
           "(Default: 50)\n");

    printf("\nFilters and Matches:");
    printf("\n  --mc\t Match requests with specific status "
           "codes\n");
    printf("  --fc\t Filter requests with specific status "
           "codes\n");
    printf("\n  --ms\t Match requests with content lengths "
           "greater "
           "than the specific amount\n");
    printf("  --fs\t Filter requests with content lengths "
           "less than "
           "the specific amount\n");
    printf("\n  --mw\t Match requests with word count "
           "greater than "
           "the specific amount\n");
    printf("  --fw\t Filter requests with word count "
           "greater than "
           "the specific amount\n");
    printf("\n  --ml\t Match requests with line count "
           "greater than "
           "the specific amount\n");
    printf("  --fl\t Filter requests with line count "
           "greater than "
           "the specific amount\n");

    /* More options */
    printf("\nMore options:\n");
    printf("  -X/--http-verb\t Specifies a custom method\n");
    printf("  --wait\t\t Specifies the duration to wait for after "
           "a request sent\n\t\t\t in each thread (Default: 0, Unit: "
           "Miliseconds)\n");

    printf("  --resolve-as\t\t Resolve domain's IP as a different IP "
           "address, so\n\t\t\t you can choose custom domain name or custom "
           "IP. Ex: www.new-domain.com:80:1.2.3.4\n");

    printf("  --disable-keep-alive\t Disables keep alive\n");
    printf("  --verbose\t\t Enables verbose mode\n");
    printf("  --show-error\t\t Shows errors occured during fuzzing\n");
    printf("  --poll-mode\t\t Easy on cpu, but a little bit slower on high "
           "bandwidth connection\n");
    printf("  --num-cpu-cores\t Specifies the number of CPU cores to be "
           "used (Default: All cores)\n");
    printf("  --user-agent\t\t Set user agent (Default: Windows 10 Chrome "
           "User agent etc...)\n");
    printf("  --timeout\t\t Set timeout duration (Default: 10 secs)\n");

    printf("\nExamples:\n");
    printf("FUZZ remote url with a dictionary using GET:\n");
    printf("\tneofuzz -u https://www.example.com/FUZZ -w \"/mnt/dict.txt\"\n");

    printf("FUZZ remote url with POST request and json as a content type:\n");
    printf("\tneofuzz -u https://www.example.com/cgi-bin/write.cgi -w "
           "\"/mnt/dict.txt\" -d '{\"user\": \"FUZZ\"}' -H \"Content-Type: "
           "application/json\"\n");

    printf("FUZZ remote url with POST request and but encode values:\n");
    printf("\tneofuzz -u https://www.example.com/cgi-bin/write.cgi -w "
           "\"/mnt/dict.txt\" -d \"filename=FUZZ\" -e\n");

    printf("Use 300 parallel sockets, hide/filter http codes 301 and 302:\n");
    printf("\tneofuzz -u https://www.example.com/FUZZ -w \"/mnt/dict.txt\" -p "
           "300 --fc 301,302\n");

    printf("Match replies sizes greater than 2000 and lower than 1000 like "
           "(2000-x] and [x-1000):\n");
    printf(
        "\tneofuzz -u https://www.example.com/FUZZ -w \"/mnt/dict.txt\" --ms "
        "2000-,-1000\n");

    printf("Use less cpu cores (use 1):\n");
    printf("\tneofuzz -u https://www.example.com/FUZZ -w \"/mnt/dict.txt\" "
           "--num-cpu-cores 1\n");

    printf("Use lots of sockets but use less cpu, manage them by poll:\n");
    printf("\tneofuzz -u https://www.example.com/FUZZ -w \"/mnt/dict.txt\" -p "
           "500 --poll-mode\n");
}

void version(void)
{
    logo();
    printf("Pars SARICA <pars@parssarica.com>\n\n");
    printf("%s\n", NEOFUZZ_VERSION);
}

sds sds_replace(sds original, char *find, char *replace)
{
    sds *tokens;
    int count;

    tokens =
        sdssplitlen(original, sdslen(original), find, strlen(find), &count);
    sds result = sdsjoin(tokens, count, replace);
    sdsfreesplitres(tokens, count);

    return result;
}

int line_count(sds counting)
{
    int count = 0;
    size_t i;
    for (i = 0; i < sdslen(counting); i++)
    {
        if (counting[i] == '\n')
        {
            count++;
        }
    }

    if (count > 0 ||
        (sdslen(counting) > 0 && counting[sdslen(counting) - 1] != '\n'))
    {
        count++;
    }

    return count;
}

int in_arguments(char *argv[], int argc, char *arg)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], arg) == 0)
        {
            return i;
        }
    }

    return -1;
}

int parse_part(const char *part, int wordcount)
{
    int first_num = 0;
    int second_num = 0;
    int mmin_words;
    int mmax_words;

    if (strchr(part, '-') == NULL)
    {
        mmin_words = atoi(part);
        mmax_words = -1;
        if (wordcount == mmin_words)
            return 1;
    }
    else if (part[0] == '-')
    {
        second_num = atoi(part + 1);
        mmin_words = 0;
        mmax_words = second_num;
        if (wordcount < mmax_words)
            return 1;
    }
    else if (part[strlen(part) - 1] == '-')
    {
        first_num = atoi(part);
        mmin_words = first_num;
        mmax_words = -1;
        if (wordcount > mmin_words)
            return 1;
    }
    else
    {
        sscanf(part, "%d-%d", &first_num, &second_num);
        mmin_words = first_num;
        mmax_words = second_num;
        if (wordcount > mmin_words && wordcount < mmax_words)
            return 1;
    }

    return 0;
}
