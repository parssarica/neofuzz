/* Pars SARICA <pars@parssarica.com> */

#include <assert.h>
#include <ctype.h>
#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sds.h"

#include "neofuzz.h"

time_t start_time;
int total_sent_requests = 0;
time_t time_diff;
time_t last_print_time = 0;
int last_is_status;
int sock_per_worker, socks_remaining;
FILE *fptr;

pthread_mutex_t worker_mutex;
pthread_mutex_t worker_file;

struct fuzz_filter
{
    sds *sc_sh_token;
    int sc_sh_count;
    int sc_hide;

    sds *mword_token;
    int mword_count;

    sds *fword_token;
    int fword_count;

    sds *msize_token;
    int msize_count;

    sds *fsize_token;
    int fsize_count;

    sds *mline_token;
    int mline_count;

    sds *fline_token;
    int fline_count;
} fuzz_filters;

static size_t write_callback(void *buffer, size_t size, size_t nmemb,
                             void *userp)
{
    t_socks *sock;
    size_t total_size = size * nmemb;
    sock = (t_socks *)userp;
    sock->content = sdscatlen(sock->content, buffer, total_size);
    if (!sock->content)
        return 0;
    return total_size;
}

static inline int send_req(t_socks *sock)
{
    curl_easy_setopt(sock->curl, CURLOPT_URL, sock->url);
    if (sdslen(co.post_data))
    {
        curl_easy_setopt(sock->curl, CURLOPT_POSTFIELDS, sock->post_data);
    }
    sock->content = sdscpylen(sock->content, "", 0);

    sock->state = SOCK_SENT;

    /*
    res = curl_easy_perform(sock->curl);
    if(res != CURLE_OK && res != CURLE_PARTIAL_FILE)
    {
        co.error_count++;
        printf("\033[F");
        printf("\033[2K");
        printf("Error processing url: %s\n", sock->url);
        return -1;
    }
    */

    return 0;
}

static inline int recv_reply(t_socks *sock)
{
    long http_code;
    curl_off_t content_length;
    int self_content_length = 0;
    int self_calculating = 0;
    char *content_type;
    /*sds *token;*/
    int passed_filter = 0;
    /*int count;*/
    int j;
    size_t s;
    int word_count = 0;
    int lines = 0;

    /*curl_easy_setopt(sock->curl, CURLOPT_VERBOSE, 1L);*/

    curl_easy_getinfo(sock->curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_getinfo(sock->curl, CURLINFO_CONTENT_TYPE, &content_type);
    curl_easy_getinfo(sock->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
                      &content_length);

    /*end of results of requests*/

    self_content_length = (int)sdslen(sock->content);
    if (self_content_length > content_length)
    {
        content_length = self_content_length;
        self_calculating = 1;
    }

    for (j = 0; j < fuzz_filters.sc_sh_count; j++)
    {
        if (fuzz_filters.sc_hide &&
            (http_code == atoi(fuzz_filters.sc_sh_token[j])))
        {
            sock->content = sdscpylen(sock->content, "", 0);
            return 0;
        }
        else if (!fuzz_filters.sc_hide &&
                 (http_code == atoi(fuzz_filters.sc_sh_token[j])))
        {
            passed_filter = 1;
            break;
        }
    }

    if (!fuzz_filters.sc_hide && !passed_filter)
    {
        sock->content = sdscpylen(sock->content, "", 0);
        return 0;
    }

    word_count = 0;
    for (s = 0; s < sdslen(sock->content); s++)
    {
        if (sock->content[s] == ' ')
            word_count++;
    }
    if (word_count == 0 && sdslen(sock->content) > 0)
        word_count++;

    passed_filter = 0;
    if (sdslen(co.words) != 0)
    {
        for (j = 0; j < fuzz_filters.mword_count; j++)
        {
            passed_filter = parse_part(fuzz_filters.mword_token[j], word_count);
            if (passed_filter)
                break;
        }
    }
    else
        passed_filter = 1;

    if (!passed_filter && sdslen(co.words))
    {
        sock->content = sdscpylen(sock->content, "", 0);
        return 0;
    }

    passed_filter = 0;
    if (sdslen(co.fwords) != 0)
    {
        for (j = 0; j < fuzz_filters.fword_count; j++)
        {
            passed_filter =
                !(parse_part(fuzz_filters.fword_token[j], word_count));
            if (!passed_filter)
                break;
        }
    }
    else
        passed_filter = 1;

    if (!passed_filter && sdslen(co.fwords))
    {
        sock->content = sdscpylen(sock->content, "", 0);
        return 0;
    }

    lines = line_count(sock->content);

    // if (!(lines >= co.min_lines &&
    //       (co.max_lines == -1 || (co.max_lines != -1 && lines <
    //       co.max_lines))))
    // {
    //     sock->content = sdscpylen(sock->content, "", 0);
    //     return 0;
    // }

    passed_filter = 0;
    if (sdslen(co.msize) != 0)
    {
        for (j = 0; j < fuzz_filters.msize_count; j++)
        {
            passed_filter =
                parse_part(fuzz_filters.msize_token[j], (int)content_length);
            if (!passed_filter)
                break;
        }
    }
    else
        passed_filter = 1;

    if (!passed_filter && sdslen(co.msize))
    {
        sock->content = sdscpylen(sock->content, "", 0);
        return 0;
    }

    passed_filter = 0;
    if (sdslen(co.fsize) != 0)
    {
        for (j = 0; j < fuzz_filters.fsize_count; j++)
        {
            passed_filter =
                !(parse_part(fuzz_filters.fsize_token[j], (int)content_length));
            if (!passed_filter)
                break;
        }
    }
    else
        passed_filter = 1;

    if (!passed_filter && sdslen(co.fsize))
    {
        sock->content = sdscpylen(sock->content, "", 0);
        return 0;
    }

    passed_filter = 0;
    if (sdslen(co.mline) != 0)
    {
        for (j = 0; j < fuzz_filters.mline_count; j++)
        {
            passed_filter = parse_part(fuzz_filters.mline_token[j], (int)lines);
            if (passed_filter)
                break;
        }
    }
    else
        passed_filter = 1;

    if (!passed_filter && sdslen(co.mline))
    {
        sock->content = sdscpylen(sock->content, "", 0);
        return 0;
    }

    passed_filter = 0;
    if (sdslen(co.fline) != 0)
    {
        for (j = 0; j < fuzz_filters.fline_count; j++)
        {
            passed_filter =
                !(parse_part(fuzz_filters.fline_token[j], (int)lines));
            if (!passed_filter)
                break;
        }
    }
    else
        passed_filter = 1;

    if (!passed_filter && sdslen(co.fline))
    {
        sock->content = sdscpylen(sock->content, "", 0);
        return 0;
    }
    // if(content_length >= min_content_length && (max_content_length == -1 ||
    // (max_content_length != -1 && content_length < max_content_length)))
    if (passed_filter)
    {
        pthread_mutex_lock(&worker_mutex);
        if (last_is_status)
        {
            printf("\033[F");
            printf("\033[2K");
        }
        printf("%s -> Status Code: %ld | Content-length: %d | Words: %d | "
               "Lines: %d",
               sock->fuzz_word, http_code, (int)content_length, word_count,
               lines);
        if (self_calculating)
        {
            printf(
                " NOTE: Content-length header doesn't match with the content's "
                "length.");
        }
        printf("\n");

        last_is_status = 0;
        pthread_mutex_unlock(&worker_mutex);
    }

    sock->content = sdscpylen(sock->content, "", 0);
    return 0;
}

void prepare_filters(void)
{
    if (sdslen(co.status_codes_show) == 0)
    {
        fuzz_filters.sc_sh_token =
            sdssplitlen(co.status_codes_hide, sdslen(co.status_codes_hide), ",",
                        1, &(fuzz_filters.sc_sh_count));
        fuzz_filters.sc_hide = 1;
    }
    else
    {
        fuzz_filters.sc_sh_token =
            sdssplitlen(co.status_codes_show, sdslen(co.status_codes_show), ",",
                        1, &(fuzz_filters.sc_sh_count));
    }

    fuzz_filters.mword_token = sdssplitlen(co.words, sdslen(co.words), ",", 1,
                                           &(fuzz_filters.mword_count));
    fuzz_filters.fword_token = sdssplitlen(co.fwords, sdslen(co.fwords), ",", 1,
                                           &(fuzz_filters.fword_count));
    fuzz_filters.msize_token = sdssplitlen(co.msize, sdslen(co.msize), ",", 1,
                                           &(fuzz_filters.msize_count));
    fuzz_filters.fsize_token = sdssplitlen(co.fsize, sdslen(co.fsize), ",", 1,
                                           &(fuzz_filters.fsize_count));
    fuzz_filters.mline_token = sdssplitlen(co.mline, sdslen(co.mline), ",", 1,
                                           &(fuzz_filters.mline_count));
    fuzz_filters.fline_token = sdssplitlen(co.fline, sdslen(co.fline), ",", 1,
                                           &(fuzz_filters.fline_count));
}

void *fuzz_worker(void *arg __attribute__((unused)))
{
    char *encoded_word;
    char *w;
    int n, nw, j;
    t_socks *sock;
    int worker_socks_count;
    void *socks_head;
    CURLM *multi_handle;
    CURLMsg *msg;
    CURLMcode mc;
    struct curl_slist *host = NULL;
    struct curl_slist *headers = NULL;
    int run_state, still_running = 1;
    sds *url_org;
    sds *post_data;
    sds *header;
    int placement_count;
    int placement_count2;
    int header_count;

    url_org = sdssplitlen(co.base_url, sdslen(co.base_url), "FUZZ", 4,
                          &placement_count);
    post_data = sdssplitlen(co.post_data, sdslen(co.post_data), "FUZZ", 4,
                            &placement_count2);

    worker_socks_count = sock_per_worker;

    pthread_mutex_lock(&worker_mutex);
    if (socks_remaining > 0)
    {
        worker_socks_count++;
        socks_remaining--;
    }
    pthread_mutex_unlock(&worker_mutex);

    socks_head = (void *)malloc(sizeof(t_socks) * worker_socks_count);
    if (socks_head == NULL)
    {
        perror("malloc");
        exit(-2);
    }

    multi_handle = curl_multi_init();
    /*curl_global_trace("all,-ssl");*/

    if (sdslen(co.headers))
    {
        header =
            sdssplitlen(co.headers, sdslen(co.headers), "\n", 1, &header_count);
        for (j = 0; j < header_count; j++)
        {
            headers = curl_slist_append(headers, header[j]);
        }
        sdsfreesplitres(header, header_count);
    }
    /*initialize socks*/
    sock = (t_socks *)socks_head;
    for (n = 0; n < worker_socks_count; n++)
    {
        sock->state = SOCK_READY;
        sock->curl = curl_easy_init();
        if (!sock->curl)
        {
            perror("curl");
            exit(-2);
        }
        /*
        sock->url = sdsempty();
        if(sock->url == NULL)
        {
            perror("sdsempty url");
            exit(-2);
        }*/
        sock->content = sdsempty();
        if (sock->content == NULL)
        {
            perror("sdsempty content");
            exit(-2);
        }

        if (sdslen(co.resolve_as))
        {
            host = curl_slist_append(host, co.resolve_as);
            curl_easy_setopt(sock->curl, CURLOPT_RESOLVE, host);
        }

        if (sdslen(co.headers))
        {
            curl_easy_setopt(sock->curl, CURLOPT_HTTPHEADER, headers);
        }
        curl_easy_setopt(sock->curl, CURLOPT_WRITEFUNCTION, write_callback);

        if (sdslen(co.proxy))
        {
            curl_easy_setopt(sock->curl, CURLOPT_PROXY, co.proxy);
        }

        if (sdslen(co.post_data))
        {
            curl_easy_setopt(sock->curl, CURLOPT_POST, 1L);
        }

        if (co.verbose)
        {
            curl_easy_setopt(sock->curl, CURLOPT_VERBOSE, 1L);
        }

        curl_easy_setopt(sock->curl, CURLOPT_USERAGENT, co.user_agent);
        curl_easy_setopt(sock->curl, CURLOPT_SSL_VERIFYPEER,
                         0); /* This is necessary for missing certificates */
        curl_easy_setopt(sock->curl, CURLOPT_TIMEOUT, (long)(co.timeout));
        if (sdslen(co.cookie))
        {
            curl_easy_setopt(sock->curl, CURLOPT_COOKIE, co.cookie);
        }
        if (sdslen(co.method))
        {
            curl_easy_setopt(sock->curl, CURLOPT_CUSTOMREQUEST, co.method);
        }
        curl_easy_setopt(sock->curl, CURLOPT_WRITEDATA, sock);

        sock++;
    }

    run_state = 1;
    while (run_state)
    {
        sock = (t_socks *)socks_head;
        for (n = 0; n < worker_socks_count; n++)
        {
            if (sock->state == SOCK_READY)
            {
                pthread_mutex_lock(&worker_file);
                w = fgets(sock->fuzz_word, sizeof(sock->fuzz_word), fptr);
                pthread_mutex_unlock(&worker_file);
                if (w == NULL)
                {
                    run_state = 2;
                    break;
                }
                nw = strlen(w);
                if (sock->fuzz_word[nw - 1] == '\n')
                    sock->fuzz_word[nw - 1] = 0;

                if (co.encode_option) /*url encode*/
                {
                    encoded_word =
                        curl_easy_escape(sock->curl, sock->fuzz_word, 0);
                    sock->url = sdsjoin(url_org, placement_count, encoded_word);
                    if (sdslen(co.post_data))
                    {
                        sock->post_data =
                            sdsjoin(post_data, placement_count2, encoded_word);
                    }

                    curl_free(encoded_word);
                }
                else
                {
                    sock->url =
                        sdsjoin(url_org, placement_count, sock->fuzz_word);

                    if (sdslen(co.post_data))
                    {
                        sock->post_data = sdsjoin(post_data, placement_count2,
                                                  sock->fuzz_word);
                    }
                }

                /*check for ready states and prepare for send req*/
                send_req(sock);

                mc = curl_multi_add_handle(multi_handle, sock->curl);
                if (mc != CURLM_OK)
                {
                    fprintf(stderr, "curl_multi_add_handle failed, code %d.\n",
                            mc);
                    exit(-1);
                }
                pthread_mutex_lock(&worker_mutex);
                total_sent_requests++;
                pthread_mutex_unlock(&worker_mutex);
            }
            sock++;
        }

        mc = curl_multi_perform(
            multi_handle, &still_running); /*after this they are really sent.*/
        if (mc == CURLM_OK)
        {
            if (run_state == 2 && !still_running)
            {
                run_state = 0;
            }

            if (co.poll_mode)
            {
                while (still_running)
                {
                    mc = curl_multi_poll(multi_handle, NULL, 0, 1000, NULL);
                    if (mc != CURLM_OK)
                    {
                        fprintf(stderr, "curl_multi_poll() failed: %s\n",
                                curl_multi_strerror(mc));
                        exit(-2);
                    }

                    mc = curl_multi_perform(multi_handle, &still_running);
                    if (mc != CURLM_OK)
                    {
                        fprintf(stderr, "curl_multi_perform() failed: %s\n",
                                curl_multi_strerror(mc));
                        exit(-2);
                    }
                }
            }
        }

        if (mc != CURLM_OK)
        {
            fprintf(stderr, "curl_multi failed, code %d.\n", mc);
            break;
        }

        while ((msg = curl_multi_info_read(multi_handle, &still_running)))
        {
            if (msg->msg == CURLMSG_DONE)
            {
                sock = (t_socks *)socks_head;
                for (n = 0; n < worker_socks_count; n++)
                {
                    if (msg->easy_handle == sock->curl)
                    {

                        if (msg->data.result != CURLE_OK &&
                            msg->data.result != CURLE_PARTIAL_FILE)
                        {
                            pthread_mutex_lock(&worker_mutex);
                            co.error_count++;
                            if (co.show_error)
                            {
                                if (last_is_status)
                                {
                                    printf("\033[F");
                                    printf("\033[2K");
                                }
                                // printf("Error processing url: %s\n",
                                // sock->url);
                                printf("[Error: %s (URL: %s)]\n",
                                       curl_multi_strerror(mc), sock->url);
                                last_is_status = 0;
                            }
                            pthread_mutex_unlock(&worker_mutex);
                        }
                        else
                        {
                            /*check for received completed states and get
                             * reply*/
                            recv_reply(sock);
                        }
                        mc = curl_multi_remove_handle(multi_handle, sock->curl);
                        if (mc != CURLM_OK)
                        {
                            fprintf(
                                stderr,
                                "curl_multi_remove_handle failed, code %d.\n",
                                mc);
                            exit(-1);
                        }
                        sdsfree(sock->url);
                        if (sdslen(co.post_data))
                        {
                            sdsfree(sock->post_data);
                        }
                        sock->state = SOCK_READY;
                        break;
                    }
                    sock++;
                }
            }
        }

        pthread_mutex_lock(&worker_mutex);
        time_diff = (time(NULL) - start_time);
        if ((time_diff % 2 == 0 && last_print_time != time_diff) ||
            !last_is_status)
        {
            if (time_diff <= 0)
                time_diff = 1;
            if (last_is_status)
            {
                printf("\033[F");
                printf("\033[2K");
            }
            printf("Requests per second: %lu   Errors: %d\n",
                   total_sent_requests / time_diff, co.error_count);
            last_is_status = 1;
            last_print_time = time_diff;
        }
        pthread_mutex_unlock(&worker_mutex);
        if (co.delay > 0 && run_state)
        {
            usleep(co.delay * 1000);
        }
    }

    sock = (t_socks *)socks_head;
    for (n = 0; n < worker_socks_count; n++)
    {
        sdsfree(sock->content);
        curl_easy_cleanup(sock->curl);
        sock++;
    }
    curl_multi_cleanup(multi_handle);
    curl_slist_free_all(host);
    free(socks_head);
    if (sdslen(co.headers))
    {
        curl_slist_free_all(headers);
    }
    sdsfreesplitres(url_org, placement_count);
    sdsfreesplitres(post_data, placement_count2);
    return NULL;
}

/* Parent */
void start_fuzz(char *wordlist_file)
{
    int i;
    pthread_t *threads;

    socks_remaining = co.socks_count % co.actual_cpu_cores;
    sock_per_worker = (co.socks_count - socks_remaining) / co.actual_cpu_cores;

    threads = malloc(co.actual_cpu_cores * sizeof(pthread_t));

    if (pthread_mutex_init(&worker_mutex, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize worker_mutex\n");
        free(threads);
        exit(-1);
    }

    if (pthread_mutex_init(&worker_file, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize worker_file\n");
        free(threads);
        exit(-1);
    }

    co.error_count = 0;

    fptr = fopen(wordlist_file, "rt");
    if (fptr == NULL)
    {
        perror("fopen");
        exit(-1);
    }

    prepare_filters();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    start_time = time(NULL);
    /* worker jobs */
    /*fuzz_worker();*/
    for (i = 0; i < co.actual_cpu_cores; i++)
    {
        if (pthread_create(&threads[i], NULL, fuzz_worker, NULL) != 0)
        {
            fprintf(stderr, "Error creating thread %d\n", i);
            free(threads);
            exit(-4);
        }
    }

    /* end of worker jobs */
    for (i = 0; i < co.actual_cpu_cores; i++)
    {
        pthread_join(threads[i], NULL);
    }

    sdsfreesplitres(fuzz_filters.sc_sh_token, fuzz_filters.sc_sh_count);
    if (sdslen(co.words) != 0)
        sdsfreesplitres(fuzz_filters.mword_token, fuzz_filters.mword_count);
    if (sdslen(co.fwords) != 0)
        sdsfreesplitres(fuzz_filters.fword_token, fuzz_filters.fword_count);
    if (sdslen(co.msize) != 0)
        sdsfreesplitres(fuzz_filters.msize_token, fuzz_filters.msize_count);
    if (sdslen(co.fsize) != 0)
        sdsfreesplitres(fuzz_filters.fsize_token, fuzz_filters.fsize_count);
    if (sdslen(co.mline) != 0)
        sdsfreesplitres(fuzz_filters.mline_token, fuzz_filters.mline_count);
    if (sdslen(co.fline) != 0)
        sdsfreesplitres(fuzz_filters.fline_token, fuzz_filters.fline_count);

    curl_global_cleanup();
    fclose(fptr);

    pthread_mutex_destroy(&worker_mutex);
    pthread_mutex_destroy(&worker_file);

    free(threads);
}
