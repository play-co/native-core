/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License v. 2.0 as published by Mozilla.

 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License v. 2.0 for more details.

 * You should have received a copy of the Mozilla Public License v. 2.0
 * along with the Game Closure SDK.  If not, see <http://mozilla.org/MPL/2.0/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>

#ifdef __ANDROID__
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#include <errno.h>

#include "curl/curl.h"

#include "../include/murmur.h"
#include "../include/image_cache.h"


//#define IMGCACHE_VERBOSE

// Define IMGCACHE_VERBOSE to enable extra debug logs
#ifdef IMGCACHE_VERBOSE
#define DLOG(...) LOG(__VA_ARGS__)
#else
#define DLOG(...)
#endif

// For stand-alone version, use printf instead of the core logger
#ifdef IMGCACHE_STANDALONE
#define LOG(str, ...) printf(str "\n", ## __VA_ARGS__)

// Inlined from my core implementation:
#define ThreadsThread pthread_t
typedef void (*ThreadsThreadProc)(void *);
pthread_t threads_create_thread(ThreadsThreadProc proc, void *arg) {
    pthread_t thread = {0};
    pthread_create(&thread, 0, proc, arg);
    return thread;
}
void threads_join_thread(ThreadsThread *thread) {
    pthread_join(*thread, 0);
}
#else
#include "core/log.h"
#include "core/platform/threads.h"
#endif // IMGCACHE_STANDALONE

#define MAX_REQUESTS 4 /* max parallel requests */
#define CACHE_MAX_SIZE 300 /* max image cache files to keep */
#define DB_MAX_SIZE (CACHE_MAX_SIZE*2) /* max etag entries in database */
#define CACHE_MAX_TIME (60 * 60 * 24 * 2) /* 2 days in seconds */

// If these change, clean_cache() needs to be rewritten
#define FILENAME_SEED 0
#define FILENAME_PREFIX_BYTES 2 /* = I$ */
#define FILENAME_PREFIX "I$"
#define FILENAME_HASH_BYTES 16 /* = 128 bits */
#define FILENAME_LENGTH (FILENAME_PREFIX_BYTES + FILENAME_HASH_BYTES*2)


//// Module Internal

struct data {
    char *bytes;
    size_t size;
    bool is_image;
};

struct load_item {
    char *url;
    volatile struct load_item *next;
};

struct request {
    CURL *handle;
    char *etag;
    struct data image;
    struct data header;
    volatile struct load_item *load_item;
};

struct work_item {
    struct image_data image;
    bool tried_server;
    bool request_failed;
    volatile struct work_item *next;
};

static void (*m_image_load_callback)(struct image_data *);

// Request thread variables
static ThreadsThread m_request_thread;
static pthread_mutex_t m_request_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t m_request_cond = PTHREAD_COND_INITIALIZER;
// To modify these variables you must hold the request_mutex lock
static volatile bool m_request_thread_running = true;
static volatile struct load_item *m_load_items = 0;

// Worker thread variables
static ThreadsThread m_worker_thread;
static pthread_mutex_t m_worker_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t m_worker_cond = PTHREAD_COND_INITIALIZER;
// To modify these variables you must hold the worker_mutex lock
static volatile bool m_worker_thread_running = true;
static volatile struct work_item *m_work_items = 0;

// Local function declarations
static pthread_mutex_t m_file_io_mutex = PTHREAD_MUTEX_INITIALIZER;
static void image_cache_run(void* args);
static void worker_run(void *args);

// Simple macros for manipulating the work/request lists
// ex. load_items is the head of the load items list and load_items_tail is the end of the list

#define LIST_PUSH(head, item) item->next = head; head = item;
#define LIST_POP(head, item) item = head; if (head) { head = item->next; }

static pthread_mutex_t m_etag_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct etag_data *m_etag_cache = 0;
static const char *ETAG_FILE = ".etags";
static char *m_file_cache_path;

// Expand file name into file path
static char *get_full_path(const char *filename) {
    size_t length = strlen(filename) + 1 + strlen(m_file_cache_path) + 1;
    char *file_path = (char *)malloc(length);

    snprintf(file_path, length, "%s/%s", m_file_cache_path, filename);

    return file_path;
}

static volatile struct work_item *alloc_work_item(const char *url, char *bytes, size_t size, bool request_failed, bool tried_server) {
    struct work_item *item = (struct work_item *)malloc(sizeof(struct work_item));

    item->image.url = strdup(url);
    item->image.bytes = bytes;
    item->image.size = size;
    item->request_failed = request_failed;
    item->tried_server = tried_server;

    return item;
}

static void free_work_item(volatile struct work_item *item) {
    if (item) {
        free(item->image.url);
        free(item->image.bytes);
        free((void*)item);
    }
}

static void queue_work_item(const char *url, char *bytes, size_t size, bool request_failed, bool tried_server) {
    volatile struct work_item *item = alloc_work_item(url, bytes, size, request_failed, tried_server);

    // add the work item to the work list
    pthread_mutex_lock(&m_worker_mutex);
    LIST_PUSH(m_work_items, item);
    pthread_cond_signal(&m_worker_cond);
    pthread_mutex_unlock(&m_worker_mutex);
}


// Safely walk string looking for a token, without reading off end of buffer
static int safe_strtoklen(const char *f, char token, const char *end) {
    int count = 0;

    while (f < end) {
        char ch = *f;

        // If we found the token,
        if (ch == token) {
            break;
        }

        ++count;
        ++f;
    }

    return count;
}

// Safely duplicates a string without a terminating nul character at end
static char *safe_strdup(const char *f, int len) {
    char *s = malloc(len + 1);

    memcpy(s, f, len);
    s[len] = '\0';

    return s;
}

// Precondition: url and etag have been strdup()
// Call with m_etag_mutex held
static struct etag_data *etag_add(char *url, char *etag) {
    struct etag_data *data = (struct etag_data *)malloc(sizeof(struct etag_data));

    data->url = url;
    data->etag = etag;

    HASH_ADD_KEYPTR(hh, m_etag_cache, url, strlen(url), data);

    return data;
}

// Parse etag data from in-memory image of the file without modifying it
static void parse_etag_file_data(const char *f, int len) {
    const char *end = f + len;
    int etag_count = 0;

    LOG("{image-cache} Parsing etags database of len=%d", len);

    pthread_mutex_lock(&m_etag_mutex);

    // While there is data to read,
    while (f < end) {
        // Find URL length
        const char *url = f;
        int url_len = safe_strtoklen(url, ' ', end);

        // If URL is missing,
        if (url_len <= 0) {
            DLOG("{image-cache} Stopping early because url was missing");
            break;
        }

        // Find ETAG length
        const char *etag = url + url_len + 1;
        int etag_len = safe_strtoklen(etag, '\n', end);

        // If ETAG is missing,
        if (etag_len <= 0) {
            DLOG("{image-cache} Stopping early because etag was missing");
            break;
        }

        // Duplicate string without a terminating nul
        char *url_cstr = safe_strdup(url, url_len);
        char *etag_cstr = safe_strdup(etag, etag_len);

        DLOG("{image-cache} Adding etag url='%s' : etag='%s'", url_cstr, etag_cstr);

        etag_add(url_cstr, etag_cstr);
        ++etag_count;

        // Stop processing after a reasonable amount of etags because this takes
        // a wild amount of time after a long game session
        if (etag_count > DB_MAX_SIZE) {
            DLOG("{image-cache} Stopping early because etag count exceeded maximum database size");
            break;
        }

        // Set read pointer to next etag
        f = etag + etag_len + 1;
    }

    pthread_mutex_unlock(&m_etag_mutex);

    DLOG("{image-cache} Parsed %d etags from database", etag_count);
}

/*
 * url -> etag mappings are stored in a file.  Each line looks like
 * 	http://example.com/foo.png 383761229c544a77af3df6dd1cc5c01d
 */
static void read_etags_from_cache() {
    char *path = get_full_path(ETAG_FILE);

    // Open the file
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        DLOG("{image-cache} WARNING: read_etags_from_cache open failed errno=%d for %s", errno, path);
    } else {
        off_t len = lseek(fd, 0, SEEK_END);

#ifndef __ANDROID__
        // These are BSD/iOS-only
        fcntl(fd, F_NOCACHE, 1);
        fcntl(fd, F_RDAHEAD, 1);
#endif

        if (len > 0) {
            void *raw = mmap(0, (size_t)len, PROT_READ, MAP_PRIVATE, fd, 0);

            if (raw == MAP_FAILED) {
                LOG("{image-cache} WARNING: read_etags_from_cache mmap failed errno=%d for %s len=%d", errno, path, (int)len);
            } else {
                parse_etag_file_data(raw, (int)len);

                munmap(raw, (size_t)len);
            }
        } else {
            DLOG("{image-cache} WARNING: read_etags_from_cache file length 0 errno=%d for %s len=%d", errno, path, (int)len);
        }

        close(fd);
    }

    free(path);
}

static void write_etags_to_cache() {
    char *path = get_full_path(ETAG_FILE);

    DLOG("{image-cache} Writing etag cache");

    FILE *f = fopen(path, "w");
    int etag_count = 0;

    if (!f) {
        LOG("{image-cache} ERROR: Unable to open-write etags cache file %s errno=%d", path, errno);
    } else {
        struct etag_data *data = 0;
        struct etag_data *tmp = 0;

        pthread_mutex_lock(&m_etag_mutex);
        HASH_ITER(hh, m_etag_cache, data, tmp) {
            if (data->etag && data->url) {
                //DLOG("{image-cache} Wrote etag='%s' for url='%s'", data->etag, data->url);

                fwrite(data->url, 1, strlen(data->url), f);
                fwrite(" ", 1, 1, f);
                fwrite(data->etag, 1, strlen(data->etag), f);
                fwrite("\n", 1, 1, f);

                // If etag count exceeds the database maximum,
                if (++etag_count > DB_MAX_SIZE) {
                    // Stop here
                    break;
                }
            } else {
                DLOG("{image-cache} Skipped writing etag='%s' for url='%s'", data->etag ? data->etag : "(null)", data->url ? data->url : "(null)");
            }
        }
        pthread_mutex_unlock(&m_etag_mutex);

        fclose(f);
    }

    free(path);
}

static void free_etag_data(struct etag_data *data) {
    free(data->url);
    free(data->etag);
    free(data);
}

static void clear_cache() {
    struct etag_data *data = 0;
    struct etag_data *tmp = 0;

    pthread_mutex_lock(&m_etag_mutex);
    HASH_ITER(hh, m_etag_cache, data, tmp) {
        free_etag_data(data);
    }
    pthread_mutex_unlock(&m_etag_mutex);
}

static void clear_work_items() {
    volatile struct work_item *item = m_work_items;
    volatile struct work_item *prev;

    while (item) {
        prev = item;
        item = item->next;

        free_work_item(item);
    }
}

static const char *HEX_CONV = "0123456789ABCDEF";

static char *get_filename_from_url(const char *url) {
    unsigned char result[FILENAME_HASH_BYTES];

    MurmurHash3_x86_128(url, (int)strlen(url), 0, result);

    char *filename = malloc(FILENAME_PREFIX_BYTES + FILENAME_HASH_BYTES*2+1);

    memcpy(filename, FILENAME_PREFIX, FILENAME_PREFIX_BYTES);

    int i;
    for (i = 0; i < FILENAME_HASH_BYTES; ++i) {
        filename[FILENAME_PREFIX_BYTES+i*2+0] = HEX_CONV[result[i] & 15];
        filename[FILENAME_PREFIX_BYTES+i*2+1] = HEX_CONV[result[i] >> 4];
    }
    filename[FILENAME_LENGTH] = '\0';

    return filename;
}

#define DC 0
static const unsigned char FROM_HEX[256] = {
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 0-15
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 16-31
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 32-47
    0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , DC, DC, DC, DC, DC, DC, // 48-63
    DC, 10, 11, 12, 13, 14, 15, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 64-79
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 128-
    DC, 10, 11, 12, 13, 14, 15, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 64-79
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 128-
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 128-
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // Extended
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // ASCII
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // Extended
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // ASCII
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // Extended
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // ASCII
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC
};
#undef DC

// NOTE: etag cache file needs to be written after this
void kill_etag_for_url_hash(const char *url_hash_str) {
    // This is fairly slow.  We need to trial-hash all of the etag database URLs
    // and see which ones match the given URL hash string.

    if (strlen(url_hash_str) != FILENAME_HASH_BYTES*2) {
        LOG("{image-cache} ERROR: Internal consistency failure");
        return;
    }

    unsigned char url_hash[FILENAME_HASH_BYTES];

    // Reverse encoding:

    int ii;
    for (ii = 0; ii < FILENAME_HASH_BYTES; ++ii) {
        unsigned char lo = FROM_HEX[(unsigned char)url_hash_str[0]];
        unsigned char hi = FROM_HEX[(unsigned char)url_hash_str[1]];

        url_hash[ii] = (hi << 4) | lo;
        url_hash_str += 2;
    }

    unsigned char data_hash[FILENAME_HASH_BYTES];

    struct etag_data *data = 0;
    struct etag_data *tmp = 0;

    pthread_mutex_lock(&m_etag_mutex);
    HASH_ITER(hh, m_etag_cache, data, tmp) {
        if (data && data->url) {
            MurmurHash3_x86_128(data->url, (int)strlen(data->url), 0, data_hash);

            // If they match,
            if (0 == memcmp(data_hash, url_hash, FILENAME_HASH_BYTES)) {
                DLOG("{image-cache} kill_etag_for_url_hash found matching URL to kill: %s", data->url);
                HASH_DEL(m_etag_cache, data);
                free_etag_data(data);
                break;
            }
        }
    }
    pthread_mutex_unlock(&m_etag_mutex);
}

// NOTE: etag cache file needs to be written after this
void kill_etag_for_url(const char *url) {
    struct etag_data *data = 0;

    pthread_mutex_lock(&m_etag_mutex);
    HASH_FIND_STR(m_etag_cache, url, data);

    if (data) {
        DLOG("{image-cache} Found image etag in cache to kill: %s", url);

        free(data->etag);
        data->etag = 0;
    } else {
        DLOG("{image-cache} Did not find image etag in cache to kill: %s", url);
    }

    pthread_mutex_unlock(&m_etag_mutex);
}

// Returns strdup version, be sure to free!
char *get_etag_for_url(const char *url) {
    char *etag = 0;
    struct etag_data *data = 0;

    pthread_mutex_lock(&m_etag_mutex);
    HASH_FIND_STR(m_etag_cache, url, data);

    if (data) {
        DLOG("{image-cache} Found image etag in cache: %s", url);
        etag = data->etag;
    } else {
        DLOG("{image-cache} Did not find image etag in cache: %s", url);
    }
    etag = etag ? strdup(etag) : 0;
    pthread_mutex_unlock(&m_etag_mutex);

    return etag;
}

static bool image_exists_in_cache(const char *url) {
    char *filename = get_filename_from_url(url);
    char *path = get_full_path(filename);

    bool exists = access(path, F_OK) != -1;

    free(filename);
    free(path);

    return exists;
}

static char *code_from_header_line(char *line) {
    char *code = 0;
    char *tok = strtok(line, "\"");
    if (tok) {
        code = strtok(0, "\"");
    }
    return code;
}

static char *parse_response_headers(char *headers) {
    char *tok = strtok(headers, "\n");

    while (tok != 0) {
        if (!strncmp("ETag", tok, 4)) {
            return code_from_header_line(tok);
        }

        tok = strtok(0, "\n");
    }

    return 0;
}


//// Image Cache Thread

static size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct data *d = (struct data*)userp;
    size_t new_size = d->size + real_size + (d->is_image ? 0 : 1);
    d->bytes = (char*)realloc(d->bytes, new_size);
    if (!d->bytes) {
        //ran out of memory!
        LOG("{image-cache} WARNING: write_data Out of memory");
        real_size = 0;
    } else {
        memcpy(&(d->bytes[d->size]), contents, real_size);
        d->size += real_size;
        if (!d->is_image) {
            d->bytes[d->size] = 0;
        }
    }
    return real_size;
}

static void image_cache_run(void *args) {
    struct request *request_pool[MAX_REQUESTS];
    struct timeval timeout;
    int i;

    // number of requests currently being processed
    int request_count = 0;

    CURLM *multi_handle = curl_multi_init();

    // Enable pipelining for speed
    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, 1L);

    // number of multi requests still running
    int still_running;

    // file descriptor variables for selecting over multi requests
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    DLOG("{image-cache} Loader thread: Starting");

    // init the handle pool and free handles
    for (i = 0; i < MAX_REQUESTS; i++) {
        request_pool[i] = (struct request*) malloc(sizeof(struct request));
        request_pool[i]->handle = curl_easy_init();
    }

    pthread_mutex_lock(&m_request_mutex);
    while (m_request_thread_running) {
        // while there are free handles start up new requests
        while (request_count < MAX_REQUESTS) {
            volatile struct load_item *load_item;
            LIST_POP(m_load_items, load_item);

            // If nothing to load,
            if (!load_item) {
                break;
            }

            DLOG("{image-cache} Loader thread: Queuing up %s", load_item->url);

            // got a new load_item so create a new request and set up the curl handle
            struct request *request = request_pool[request_count];
            curl_easy_reset(request->handle);
            request->image.bytes = 0;
            request->image.size = 0;
            request->image.is_image = false;
            request->header.bytes = 0;
            request->header.size = 0;
            request->header.is_image = false;

            // the load item for this request is stored for use after the request finishes
            request->load_item = load_item;

            if (strncmp(load_item->url, "//", 2) == 0) {
                // if request begins with "//", change to "http://..."
                // 6 accounts for "http:" and '\0'
                char *request_url = malloc(strlen(load_item->url) + 6);
                sprintf(request_url, "http:%s", load_item->url);
                curl_easy_setopt(request->handle, CURLOPT_URL, request_url);
                free(request_url);
            } else {
                curl_easy_setopt(request->handle, CURLOPT_URL, load_item->url);
            }

            // if we have a cached version of the file on disk then try to get its etag
            if (image_exists_in_cache(load_item->url)) {
                request->etag = get_etag_for_url(load_item->url);
            } else {
                request->etag = 0;
            }

            // if we have an etag add it to the request header
            if (request->etag) {
                DLOG("{image-cache} Loader thread: We have an ETAG for %s, sending it to server", load_item->url);

                static const char *FORMAT = "If-None-Match: \"%s\"";
                size_t header_str_len = strlen(FORMAT) + strlen(request->etag) + 1;
                char *etag_header_str = malloc(header_str_len);
                snprintf(etag_header_str, header_str_len, FORMAT, request->etag);

                struct curl_slist *headers = curl_slist_append(0, etag_header_str);

                curl_easy_setopt(request->handle, CURLOPT_HTTPHEADER, headers);

                free(etag_header_str);
            }

#ifdef IMGCACHE_VERBOSE
            curl_easy_setopt(request->handle, CURLOPT_VERBOSE, 1L);
#else
            curl_easy_setopt(request->handle, CURLOPT_VERBOSE, 0L);
#endif
            curl_easy_setopt(request->handle, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(request->handle, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(request->handle, CURLOPT_WRITEHEADER, &request->header);
            curl_easy_setopt(request->handle, CURLOPT_WRITEDATA, &request->image);

            // Disable SSL verification steps
            curl_easy_setopt(request->handle, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(request->handle, CURLOPT_SSL_VERIFYHOST, 0L);

            // If HTTPS,
            if (strncasecmp(load_item->url, "https://", 8) == 0) {
                curl_easy_setopt(request->handle, CURLOPT_USE_SSL, CURLUSESSL_TRY);
            }

            // Follow redirects to work with Facebook API et al
            curl_easy_setopt(request->handle, CURLOPT_FOLLOWLOCATION, 1L);

            // timeout for long requests
            curl_easy_setopt(request->handle, CURLOPT_TIMEOUT, 20);
            curl_easy_setopt(request->handle, CURLOPT_FAILONERROR, 1L);

            // add this handle to the group of multi requests
            if (curl_multi_add_handle(multi_handle, request->handle) != CURLM_OK) {
                LOG("{image-cache} WARNING: curl_multi_add_handle failed for %s", load_item->url);
            }

            request_count++;
        }

        // if no requests are currently being processed sleep until signaled
        if (0 >= request_count) {
            pthread_cond_wait(&m_request_cond, &m_request_mutex);
            continue;
        }

        // unlock to process any ongoing curl requests
        pthread_mutex_unlock(&m_request_mutex);

        if (curl_multi_perform(multi_handle, &still_running) != CURLM_OK) {
            LOG("{image-cache} WARNING: curl_multi_perform failed");
        }

        DLOG("{image-cache} Loader thread: Performing. still running = %d, request count = %d", still_running, request_count);

        // loop until at least one request finishes processing
        do {
            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            // set a default timeout before getting one from curl
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            /* get file descriptors from the transfers */
            curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

            int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

            switch(rc) {
            case -1:
                /* select error */
                break;
            case 0: /* timeout */
            default: /* action */
                if (curl_multi_perform(multi_handle, &still_running) != CURLM_OK) {
                    LOG("{image-cache} WARNING: curl_multi_perform failed");
                }
                break;
            }
        } while (still_running == request_count);

        DLOG("{image-cache} Loader thread: Completed at least one.  Still running = %d, request count = %d", still_running, request_count);

        bool update_etag_cache = false;

        // at least one request has finished
        CURLMsg *msg; /* for picking up messages with the transfer status */
        int msgs_left; /* how many messages are left */
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                int idx, found = 0;

                /* Find out which handle this message is about */
                for (idx = 0; idx < request_count; idx++) {
                    found = (msg->easy_handle == request_pool[idx]->handle);
                    if (found) {
                        break;
                    }
                }

                struct request *request = request_pool[idx];
                if (msg->data.result == CURLE_OK) {
                    DLOG("{image-cache} Loader thread: Finished request: %s with result %d and image size %zu", request->load_item->url, msg->data.result, request->image.size);
                    struct etag_data *etag_data = 0;

                    // Check to see if this url already has etag data
                    // if it doesnt create it now
                    pthread_mutex_lock(&m_etag_mutex);
                    HASH_FIND_STR(m_etag_cache, request->load_item->url, etag_data);
                    if (!etag_data) {
                        etag_data = etag_add(strdup(request->load_item->url), request->etag ? strdup(request->etag) : 0);
                    } else {
                        DLOG("{image-cache} Loader thread: Loaded existing etag data for %s", request->load_item->url);
                    }
                    pthread_mutex_unlock(&m_etag_mutex);

                    // if we got an image back from the server send the image data to the worker thread for processing
                    if (request->image.size > 0) {
                        char *etag = parse_response_headers(request->header.bytes);

                        DLOG("{image-cache} Loader thread: Got an updated image for %s (%zd bytes) etag=%d", request->load_item->url, request->image.size, etag ? 1 : 0);

                        queue_work_item(request->load_item->url, request->image.bytes, request->image.size, false, true);

                        // If something changed,
                        if (etag_data->etag || etag) {
                            update_etag_cache = true;
                        }

                        // Update etag
                        free(etag_data->etag);
                        etag_data->etag = etag ? strdup(etag) : 0;
                    } else {
                        queue_work_item(request->load_item->url, 0, 0, false, true);

                        free(request->image.bytes);
                        DLOG("{image-cache} Loader thread: Did not get an image from server for %s", request->load_item->url);
                    }
                } else {
                    DLOG("{image-cache} Loader thread: WARNING: CURL returned fail response code %d while requesting %s", msg->data.result, request->load_item->url);

                    // free any bytes acquired during the request
                    free(request->image.bytes);

                    queue_work_item(request->load_item->url, 0, 0, true, true);
                }

                free(request->etag);
                free(request->header.bytes);
                free(request->load_item->url);
                free((void*)request->load_item);
                curl_multi_remove_handle(multi_handle, request->handle);

                // move this request to the end of the request pool so it can be freed
                struct request *temp = request_pool[request_count - 1];
                request_pool[request_count - 1] = request_pool[idx];
                request_pool[idx] = temp;

                request_count--;
            }
        }

        if (update_etag_cache) {
            // save all etags to a file
            write_etags_to_cache();
        }

        pthread_mutex_lock(&m_request_mutex);
    }

    pthread_mutex_unlock(&m_request_mutex);

    DLOG("{image-cache} Loader thread: Good night!");

    curl_multi_cleanup(multi_handle);

    for (i = 0; i < MAX_REQUESTS; i++) {
        curl_easy_cleanup(request_pool[i]->handle);
        free(request_pool[i]);
    }
}


//// Worker Thread

static void callback_cached_image(char *url, bool report_error) {
    char *filename = get_filename_from_url(url);
    char *path = get_full_path(filename);

    struct image_data image;
    image.url = url;
    image.bytes = 0;
    image.size = 0;

    pthread_mutex_lock(&m_file_io_mutex);

    // Open the file
    int fd = open(path, O_RDONLY);

    bool success = false;

    if (fd == -1) {
        DLOG("{image-cache} WARNING: callback_cached_image open failed errno=%d for %s", errno, path);
    } else {
        off_t len = lseek(fd, 0, SEEK_END);

#ifndef __ANDROID__
        // These are BSD/iOS-only
        fcntl(fd, F_NOCACHE, 1);
        fcntl(fd, F_RDAHEAD, 1);
#endif

        if (len > 0) {
            void *raw = mmap(0, (size_t)len, PROT_READ, MAP_PRIVATE, fd, 0);

            if (raw == MAP_FAILED) {
                LOG("{image-cache} WARNING: callback_cached_image failed errno=%d for %s len=%d", errno, path, (int)len);
            } else {
                DLOG("{image-cache} Reading cached image data: %s bytes=%d", url, (int)len);

                image.bytes = raw;
                image.size = (size_t)len;
                success = true;
            }
        } else {
            DLOG("{image-cache} WARNING: callback_cached_image file length 0 errno=%d for %s len=%d", errno, path, (int)len);
        }

        close(fd);
    }

    // If it is time to run the callback,
    if (success || report_error) {
        m_image_load_callback(&image);
    }

    // If successfully mapped,
    if (success) {
        munmap(image.bytes, image.size);
    }

    pthread_mutex_unlock(&m_file_io_mutex);

    free(filename);
    free(path);
}

static bool save_image(struct image_data *image) {
    bool success = true;
    char *filename = get_filename_from_url(image->url);
    char *path = get_full_path(filename);

    pthread_mutex_lock(&m_file_io_mutex);

    FILE *f = fopen(path, "wb");
    if (!f) {
        LOG("{image-cache} WARNING: Unable to open to save file %s errno=%d", path, errno);
        success = false;
    } else {
        size_t bytes_written = fwrite(image->bytes, 1, image->size, f);

        if (bytes_written != image->size) {
            success = false;
            LOG("{image-cache} ERROR: Wrote %zu but expected %zu bytes for %s errno=%d", bytes_written, image->size, path, errno);

            bool removed = remove(path);
            if (!removed) {
                LOG("{image-cache} ERROR: Failed to remove file %s errno=%d", path, errno);
            }
        } else {
            DLOG("{image-cache} Saved updated image to cache: %s bytes=%d", image->url, (int)image->size);
        }
        fclose(f);
    }

    pthread_mutex_unlock(&m_file_io_mutex);

    free(filename);
    free(path);
    return success;
}

// Expunge old files from cache
static void clean_cache() {
    DIR *dir = opendir(m_file_cache_path);
    if (!dir) {
        LOG("{image-cache} WARNING: Unable to open directory to clean cache errno=%d", errno);
    } else {
        struct dirent *entry = 0;

        time_t now;
        time(&now);

        int count = 0;
        bool update_cache = false;

        while ((entry = readdir(dir))) {
            const char *filename = entry->d_name;

            if (filename[0] == FILENAME_PREFIX[0] &&
                    filename[1] == FILENAME_PREFIX[1] &&
                    strlen(filename) == FILENAME_LENGTH) {

                char *path = get_full_path(filename);

                if (count >= CACHE_MAX_SIZE) {
                    remove(path);
                    kill_etag_for_url_hash(filename + FILENAME_PREFIX_BYTES);
                    update_cache = true;
                    DLOG("{image-cache} Removed cache file %s (ran out of room)", path);
                } else {
                    struct stat attrib;
                    if (0 == stat(path, &attrib)) {
                        int delta = (int)difftime(now, attrib.st_atime);

                        if (delta > CACHE_MAX_TIME) {
                            remove(path);
                            kill_etag_for_url_hash(filename + FILENAME_PREFIX_BYTES);
                            update_cache = true;
                            DLOG("{image-cache} Removed cache file %s (file too old %d)", path, delta);
                        } else {
                            ++count;
                        }
                    }
                }

                free(path);
            }
        }

        if (update_cache) {
            write_etags_to_cache();
        }

        closedir(dir);
    }
}

// worker thread
static void worker_run(void *args) {
    // Run these off a side thread to avoid blocking startup:

    read_etags_from_cache();

    clean_cache();

    // Start request thread after cache is fixed up
    m_request_thread = threads_create_thread(image_cache_run, 0);

    // Local work item list
    volatile struct work_item *local_items = 0;
    volatile struct work_item *item;

    pthread_mutex_lock(&m_worker_mutex);

    // While running,
    while (m_worker_thread_running) {
        // Copy all work items to local list
        while (m_work_items) {
            LIST_POP(m_work_items, item);
            LIST_PUSH(local_items, item);
        }

        // If no items to work on,
        if (!local_items) {
            pthread_cond_wait(&m_worker_cond, &m_worker_mutex);
            continue;
        }

        pthread_mutex_unlock(&m_worker_mutex);

        // While items are in list,
        while (local_items) {
            LIST_POP(local_items, item);

            struct image_data *image = (struct image_data *)&item->image;

            // if no image bytes were provided try loading the image from disk
            // otherwise save the image
            if (0 == image->bytes) {
                // If server response came back,
                if (item->tried_server) {
                    // If image was not in cache either,
                    if (!image_exists_in_cache(image->url)) {
                        // Callback with failure now
                        m_image_load_callback(image);
                    }
                    // Otherwise: Already delivered as it exists
                    // TODO: This may never call the callback if the file was not accessible
                } else {
                    // Callback on success but not failure
                    callback_cached_image(image->url, false);
                }
            } else {
                m_image_load_callback(image);

                save_image(image);

                LOG("{image-cache} Updated: %s (bytes = %d)", image->url, (int)image->size);
            }

            free_work_item(item);
        }

        pthread_mutex_lock(&m_worker_mutex);
    }

    pthread_mutex_unlock(&m_worker_mutex);
}


//// API

void image_cache_init(const char *path, image_cache_cb load_callback) {
    LOG("{image-cache} Initializing");

    curl_global_init(CURL_GLOBAL_ALL);

    m_file_cache_path = strdup(path); // intentional leak

    m_image_load_callback = load_callback;
    m_request_thread_running = true;
    m_worker_thread_running = true;

    m_worker_thread = threads_create_thread(worker_run, 0);
}

void image_cache_destroy() {
    LOG("{image-cache} Shutting down...");

    pthread_mutex_lock(&m_request_mutex);
    m_request_thread_running = false;
    pthread_cond_signal(&m_request_cond);
    pthread_mutex_unlock(&m_request_mutex);

    pthread_mutex_lock(&m_worker_mutex);
    m_worker_thread_running = false;
    pthread_cond_signal(&m_worker_cond);
    pthread_mutex_unlock(&m_worker_mutex);

    // Join worker thread first, since it starts the request thread
    threads_join_thread(&m_worker_thread);
    threads_join_thread(&m_request_thread);

    clear_cache();
    clear_work_items();
    free(m_file_cache_path);

    LOG("{image-cache} ...Good night.");
}

void image_cache_remove(const char *url) {
    DLOG("{image-cache} Removing image from cache: %s", url);

    if (image_exists_in_cache(url)) {
        char *filename = get_filename_from_url(url);
        if (filename) {
            char *path = get_full_path(filename);
            if (path) {
                // Remove file from disk
                remove(path);

                // Also remove its etag so if we request it we do not expect it to be there
                kill_etag_for_url(url);

                // And update the etag database now
                write_etags_to_cache();

                free(path);
            }
            free(filename);
        }
    }
}

void image_cache_load(const char *url) {
    // If image is already in cache,
    if (image_exists_in_cache(url)) {
        DLOG("{image-cache} Image exists in cache so attempt to return that: %s", url);
        queue_work_item(url, 0, 0, true, false);
    }

    // TODO: Do not request from server again within a certain amount of time

    // But also load it from the server again in case it has changed
    volatile struct load_item *load_item = (struct load_item *)malloc(sizeof(struct load_item));
    load_item->url = strdup(url);

    DLOG("{image-cache} Async loading: %s", url);

    pthread_mutex_lock(&m_request_mutex);
    LIST_PUSH(m_load_items, load_item);
    pthread_cond_signal(&m_request_cond);
    pthread_mutex_unlock(&m_request_mutex);
}
