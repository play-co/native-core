#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
 
#include "curl/curl.h"
#include "crypto/sha1.h"

#include "image_cache.h"

#define LOG(...) printf(__VA_ARGS__)
//#include "core/log.h"
//#define printf(...) LOG(__VA_ARGS__)
#define MAX_REQUESTS 10

struct data {
	char *bytes;
	size_t size;
	bool is_image;
};

struct load_item {
	char *url;
	void *meta_data;

	struct load_item *next;
};

struct request {
	CURL *handle;
	const char *etag;
	struct data image;
	struct data header;
	struct load_item *load_item;
};

struct work_item {
	struct image_data *image_data;
	struct work_item *next;
};

static void (*image_load_callback)(struct image_data*);

// Request thread variables
static pthread_t request_thread;
static pthread_mutex_t request_mutex = PTHREAD_MUTEX_INITIALIZER; 
static pthread_cond_t request_cond = PTHREAD_COND_INITIALIZER; 
// To modify these variables you must hold the request_mutex lock
static bool request_thread_running = true;
static struct load_item *load_items = NULL;
static struct load_item *load_items_tail = NULL;

// Worker thread variables
static pthread_t worker_thread;
static pthread_mutex_t worker_mutex = PTHREAD_MUTEX_INITIALIZER; 
static pthread_cond_t worker_cond = PTHREAD_COND_INITIALIZER; 
// To modify these variables you must hold the worker_mutex lock
static bool worker_thread_running = true;
static struct work_item *work_items = NULL;
static struct work_item *work_items_tail = NULL;

// Local function declarations
struct image_data *image_cache_fetch_remote_image(const char *url, const char *etag);
void *image_cache_run(void* args);
void *worker_run(void *args);

// Simple macros for manipulating the work/request lists
// ex. load_items is the head of the load items list and load_items_tail is the end of the list

// Add an item to the end of the list
#define PUSH_BACK(list_name, item) {  \
		if (!list_name) {\
			list_name = item;\
		}\
		if (list_name##_tail) {\
			list_name##_tail->next = item;\
		}\
		list_name##_tail = item;\
        item->next = NULL;\
} 

// Remove the item at the head of the list
#define POP_FRONT(list_name) {\
	list_name = list_name->next;\
	if (!list_name) {\
		list_name##_tail = NULL;\
	}\
}

// Clear list
#define CLEAR_LIST(list_name) {\
	list_name = NULL;\
	list_name##_tail = NULL;\
}\

static char *file_cache_path;

static struct etag_data *etag_cache = NULL;
static const char *etag_file_name = ".etags";

static size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t real_size = size * nmemb;
	struct data *d = (struct data*)userp;
	size_t new_size = d->size + real_size + (d->is_image ? 0 : 1);
	d->bytes = (char*)realloc(d->bytes, new_size);
	if (d->bytes == NULL) {
		//ran out of memory!
		LOG("Error, out of memory!\n");
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


void add_etags_to_memory_cache(char *f) {
	char *saveptr = NULL;
    char *url = strtok_r(f, " ", &saveptr);
    while (url) {
        char *etag = strtok_r(NULL, "\n", &saveptr);
        if (etag) {
            LOG("adding etag %s : %s\n", url, etag);
			struct etag_data *data = (struct etag_data*)malloc(sizeof(struct etag_data));
			data->etag = strdup(etag);
			data->url = strdup(url);
			HASH_ADD_KEYPTR(hh, etag_cache, data->url, strlen(data->url), data);
		}
        url = strtok_r(NULL, " ", &saveptr);
    }
}

char *get_full_path(const char *filename) {
	static const char *format = "%s/%s";
	size_t length = strlen(filename) + strlen(file_cache_path) + 1 + 1;
	char *file_path = (char *)malloc(sizeof(char) * length);
	snprintf(file_path, length, format, file_cache_path, filename);
	return file_path;
}
/*
 * url -> etag mappings are stored in a file.  Each line looks like
 * 	http://example.com/foo.png 383761229c544a77af3df6dd1cc5c01d
 */
void read_etags_from_cache() {
	char *etags_file_path = get_full_path(etag_file_name);
	if (access(etags_file_path, F_OK) != -1) {
		FILE *f = fopen(etags_file_path, "r");
		if (!f) {
			LOG("Error opening etags cache file.  Does the directory exist?\n");
		} else {
			fseek(f, 0, SEEK_END);
			size_t file_size = ftell(f);
			rewind(f);

			char *file_buffer = (char*)malloc(sizeof(char) * file_size);
			if (!file_buffer) {
				LOG("error ran out of memory!\n");
			} else {
				size_t bytes_read = fread(file_buffer, 1, file_size, f);
				if (bytes_read != file_size) {
					//LOG("error reading file: size %li read %li\n", file_size, bytes_read);
				}
			}
			fclose(f);
            LOG("============== read file\n\n%s\n\n===========", file_buffer);
			
            add_etags_to_memory_cache(file_buffer);
            
            free(file_buffer);

		}
	}
	free(etags_file_path);
}

void write_etags_to_cache() {
	struct etag_data *data = NULL;
	struct etag_data *tmp = NULL;
	char *path = get_full_path(etag_file_name);
	FILE *f = fopen(path, "w");
	static const char *format = "%s %s\n";
	HASH_ITER(hh, etag_cache, data, tmp) {
		if (data->etag && data->url) {
			size_t length = strlen(data->etag) + strlen(data->url) + strlen(format) + 1;
			char *line = (char*)malloc(sizeof(char) * length);
			snprintf(line, length, format, data->url, data->etag);
			fwrite(line, sizeof(char), strlen(line), f);
			free(line);
		}
	}
    fwrite("\n", sizeof(char), 1, f);
	fclose(f);
	free(path);
}

void image_cache_init(const char *path, void (*load_callback)(struct image_data*)) {
	curl_global_init(CURL_GLOBAL_ALL);	
	file_cache_path = strdup(path);
	read_etags_from_cache();

	image_load_callback = load_callback;
	request_thread_running = true;
	worker_thread_running = true;
	pthread_create(&request_thread, NULL, image_cache_run, NULL);
	pthread_create(&worker_thread, NULL, worker_run, NULL);
}

void clear_cache() {
	struct etag_data *data = NULL;
	struct etag_data *tmp = NULL;
	HASH_ITER(hh, etag_cache, data, tmp) {
		free((void*)data->url);
		free((void*)data->etag);
		free(data);
	}
}

void image_cache_destroy() {
	pthread_mutex_lock(&request_mutex);
	request_thread_running = false;
	pthread_cond_signal(&request_cond);
	pthread_mutex_unlock(&request_mutex);

	pthread_mutex_lock(&worker_mutex);
	worker_thread_running = false;
	pthread_cond_signal(&worker_cond);
	pthread_mutex_unlock(&worker_mutex);

	pthread_join(request_thread, NULL);
	pthread_join(worker_thread, NULL);

	write_etags_to_cache();
	clear_cache();
    free(file_cache_path);
}


const char *get_etag_for_url(const char *url) {
	const char *etag = NULL;
	struct etag_data *data = NULL;
	HASH_FIND_STR(etag_cache, url, data);
	if (data) {
		etag = data->etag;
	} else {
		LOG("didn't find image in cache\n");
	}
	return etag;
}

char *get_filename_from_url(const char *url) {
	SHA1Context ctx;	
	SHA1Reset(&ctx);
	SHA1Input(&ctx, (const unsigned char*)url, strlen(url));
	static const int digest_size = 20;
	unsigned char result[digest_size];
	SHA1Result(&ctx, result);
	char *filename = malloc(sizeof(char)*digest_size * 2);
	char *buf_ptr = filename;
    int i;
	for (i = 0; i < digest_size; i++) {
		buf_ptr += sprintf(buf_ptr, "%x", result[i]);
	};
	return filename;
}

void get_cached_image_for_url(struct image_data *image_data) {
	char *filename = get_filename_from_url(image_data->url);
	char *path = get_full_path(filename);
	char *file_buffer = NULL;
	free(filename);
	FILE *f = fopen(path, "rb");
	if (!f) {
		LOG("Error opening cache file for url %s\n", image_data->url);
	} else {
		fseek(f, 0, SEEK_END);
		size_t file_size = ftell(f);
		rewind(f);

		file_buffer  = (char*)malloc(sizeof(char) * file_size);
		if (!file_buffer) {
			LOG("error ran out of memory!\n");
		} else {
			size_t bytes_read = fread(file_buffer, 1, file_size, f);
			if (bytes_read != file_size) {
				//LOG("error reading file - size: %li read: %li\n", file_size, bytes_read);
				file_buffer = NULL;
			}
		}
        image_data->bytes = file_buffer;
        image_data->size = file_size;

	}
}

bool save_image_for_url(const char *url, struct image_data *image) {
    bool success = true;
	char *filename = get_filename_from_url(url);
	char *path = get_full_path(filename);
	free(filename);
    
	FILE *f = fopen(path, "wb");
    if (!f) {
        LOG("error opening file %s\n", path);
        success = false;
    } else {
        size_t bytes_written = fwrite(image->bytes, sizeof(char), image->size, f);
        if (bytes_written != image->size) {
            success = false;
            LOG("error writing file - wrote %zu but expected %zu bytes\n", bytes_written, image->size);
            bool removed = remove((const char*)path);
            if (!removed) {
                LOG("Failed to remove file - this is probably not good!\n");
            }
        } else {
            LOG("Wrote cache file for image %s\n", url);
        }
        fclose(f);
    }

	free(path);
	return success;
}

bool image_exists_in_cache(const char *url) {
	char *filename = get_filename_from_url(url);
	char *file_path = get_full_path(filename);
	free(filename);
	bool exists = access(file_path, F_OK) != -1;
	free(file_path);
	return exists;
}

static char *code_from_etag_line(char *line) {
	char *code = NULL;
	char *tok = strtok(line, "\"");
	if (tok) {
		code = strtok(NULL, "\"");	
	}
	return code;
}

char *parse_etag_from_headers(char *headers) {
	char *etag = NULL;
	char *tok = strtok(headers, "\n");
	while (tok != NULL) {
		if (!strncmp("ETag", tok, 4)) {
			etag = code_from_etag_line(tok);
			break;
		}
		tok = strtok(NULL, "\n");
	}
	return etag;
}

void image_cache_load(const char *url) {
	// if the file exists return the path to the image...
	if (image_exists_in_cache(url)) {
		struct image_data *image_data = (struct image_data*)malloc(sizeof(struct image_data));
		image_data->url = strdup(url);
		image_data->bytes = NULL;
		image_data->size = 0;

		// add work to the work list
		struct work_item *work_item = (struct work_item*) malloc(sizeof(struct work_item));
		work_item->image_data = image_data;

		pthread_mutex_lock(&worker_mutex);
		PUSH_BACK(work_items, work_item);
		pthread_cond_signal(&worker_cond);
		pthread_mutex_unlock(&worker_mutex);
	}

	struct load_item *load_item = (struct load_item*) malloc(sizeof(struct load_item));
	load_item->url = strdup(url);
	load_item->meta_data = NULL;
	load_item->next = load_items;

	pthread_mutex_lock(&request_mutex);
	PUSH_BACK(load_items, load_item);
	pthread_cond_signal(&request_cond);
	pthread_mutex_unlock(&request_mutex);

	LOG("starting load for: %s\n", url);
}

void *image_cache_run(void* args) {
	struct request *request_pool[MAX_REQUESTS];
	struct timeval timeout;
	int i;

	// number of requests currently being processed
	int request_count = 0;

	CURLM *multi_handle = curl_multi_init();

	// store the timeout requested by curl
	long curl_timeo = -1;

	// number of multi requests still running
	int still_running;

	// file descriptor variables for selecting over multi requests
	fd_set fdread;
	fd_set fdwrite;
	fd_set fdexcep;
	int maxfd = -1;

	// init the handle pool and free handles
	for (i = 0; i < MAX_REQUESTS; i++) {
		request_pool[i] = (struct request*) malloc(sizeof(struct request));
		request_pool[i]->handle = curl_easy_init();
	}


	pthread_mutex_lock(&request_mutex);
	while (request_thread_running) {
		// while there are free handles start up new requests
		while (request_count < MAX_REQUESTS) {
			struct load_item *load_item;
	
			// if there are any load requests take the first one otherwise break and continue
			if (load_items) {
				load_item = load_items;	
				POP_FRONT(load_items);
			} else {
				break;
			}

			// got a new load_item so create a new request and set up the curl handle
			struct request *request = request_pool[request_count];
			curl_easy_reset(request->handle);
			LOG("ADDING REQUEST: %p\n", request);
			request->image.bytes = (char*)malloc(1);
			request->image.size = 0;
			request->image.is_image = false;
			request->header.bytes = (char*)malloc(1);
			request->header.size = 0;
			request->header.is_image = false;
		
			// the load item for this request is stored for use after the request finishes
			request->load_item = load_item;
			curl_easy_setopt(request->handle, CURLOPT_URL, load_item->url);

			// if we have a cached version of the file on disk then try to get its etag
			if (image_exists_in_cache(load_item->url)) {
				request->etag = get_etag_for_url(load_item->url);
			} else {
				request->etag = NULL;
			}
	
			// if we have an etag add it to the request header
			if (request->etag) {
				struct curl_slist *headers = NULL;
				LOG("we have an etag, sending it to the server\n");
				static const char *etag_header_format = "If-None-Match: \"%s\"";
				size_t header_str_len = strlen(etag_header_format) + strlen(request->etag) + 1;
				char *etag_header_str = (char*)malloc(sizeof(char)*header_str_len);
				snprintf(etag_header_str, header_str_len, etag_header_format, request->etag);
				headers = curl_slist_append(headers, etag_header_str);
				curl_easy_setopt(request->handle, CURLOPT_HTTPHEADER, headers);
				free(etag_header_str);
			}
			

			curl_easy_setopt(request->handle, CURLOPT_VERBOSE, false);
			curl_easy_setopt(request->handle, CURLOPT_NOPROGRESS, true);
			curl_easy_setopt(request->handle, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(request->handle, CURLOPT_WRITEHEADER, &request->header);
			curl_easy_setopt(request->handle, CURLOPT_WRITEDATA, &request->image);

			// Disable SSL verification steps
			curl_easy_setopt(request->handle, CURLOPT_SSL_VERIFYPEER, false);
			curl_easy_setopt(request->handle, CURLOPT_SSL_VERIFYHOST, 0);

			// If HTTPS,
			if (strncasecmp(load_item->url, "https://", 8) == 0) {
				curl_easy_setopt(request->handle, CURLOPT_USE_SSL, CURLUSESSL_TRY);
			}

			// timouet if currently 15 seconds
			curl_easy_setopt(request->handle, CURLOPT_TIMEOUT, 15);

			// add this handle to the group of multi requests
			curl_multi_add_handle(multi_handle, request->handle);
				
			request_count++;
		}
	
		// if no requests are currently being processed sleep until signaled
		if (!request_count) {
			pthread_cond_wait(&request_cond, &request_mutex);
			continue;
		}

		// unlock to process any ongoing curl requests
		pthread_mutex_unlock(&request_mutex);

		curl_multi_perform(multi_handle, &still_running);
		LOG("before request count: %d of %d\n", still_running, request_count);

		// loop until at least one request finishes processing
		do {
			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdexcep);


			// set a default timeout before getting one from curl
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			curl_multi_timeout(multi_handle, &curl_timeo);
			if(curl_timeo >= 0) {
				timeout.tv_sec = curl_timeo / 1000;
				if(timeout.tv_sec > 1)
					timeout.tv_sec = 1;
				else
					timeout.tv_usec = (curl_timeo % 1000) * 1000;
			}

			/* get file descriptors from the transfers */ 
			curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

			int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

			switch(rc) {
				case -1:
					/* select error */ 
					break;
				case 0: /* timeout */ 
				default: /* action */ 
					curl_multi_perform(multi_handle, &still_running);
					break;
			}
		} while (still_running == request_count);

		LOG("after request count: %d\n", still_running);

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
				LOG("finished request: %s\n", request->load_item->url);

				struct etag_data *etag_data = NULL;
				// Check to see if this url already has etag data
				// if it doesnt create it now
				HASH_FIND_STR(etag_cache, request->load_item->url, etag_data);
				if (!etag_data) {
					etag_data = (struct etag_data*)malloc(sizeof(struct etag_data));
					etag_data->url = strdup(request->load_item->url);
					if (request->etag) {
						etag_data->etag = strdup(request->etag);
					} else {
						etag_data->etag = NULL;
					}
					HASH_ADD_KEYPTR(hh, etag_cache, etag_data->url, strlen(etag_data->url), etag_data);
				} else {
					LOG("loaded existing etag data\n");
				}


				// if we got an image back from the server send the image data to the worker thread for processing
				if (request->image.size > 0) {
					struct image_data *image_data = (struct image_data*)malloc(sizeof(struct image_data));
					image_data->url = strdup(request->load_item->url);

					LOG("got an updated image! %zd\n", request->image.size);
					//we got an image back
					image_data->bytes = request->image.bytes;
					image_data->size = request->image.size;
					free(etag_data->etag);
					
					char *etag = parse_etag_from_headers(request->header.bytes);
					if (etag) {
						etag_data->etag = strdup(etag);
					} else {
						LOG("no etag for %s\n", etag_data->url);
					}

					// save all etags to a file
					write_etags_to_cache();

					// create a new work item
					struct work_item *work_item = (struct work_item*) malloc(sizeof(struct work_item));
					work_item->image_data = image_data;
					work_item->next = work_items;

					// add the work item to the work list
					pthread_mutex_lock(&worker_mutex);
					PUSH_BACK(work_items, work_item);
					pthread_cond_signal(&worker_cond);
					pthread_mutex_unlock(&worker_mutex);

				} else {
					free(request->image.bytes);
					LOG("didn't get an image from the server - probably already up to date\n");
				}

				free(request->header.bytes);
				free(request->load_item->url);
				free(request->load_item);
				curl_multi_remove_handle(multi_handle, request->handle);


				// move this request to the end of the request pool so it can be freed
				struct request *temp = request_pool[request_count - 1];
				request_pool[request_count - 1] = request_pool[idx];
				request_pool[idx] = temp;

				request_count--;

			}
		}	

		pthread_mutex_lock(&request_mutex);
	}
	pthread_mutex_unlock(&request_mutex);

	curl_multi_cleanup(multi_handle);
	for (i = 0; i < MAX_REQUESTS; i++) {
		curl_easy_cleanup(request_pool[i]->handle);
		free(request_pool[i]);
	}
	return NULL;
}

// worker thread
void *worker_run(void *args) {
	pthread_mutex_lock(&worker_mutex);
	while (worker_thread_running) {
		// handle callbacks
		if (!work_items) {
			pthread_cond_wait(&worker_cond, &worker_mutex);
			continue;
		}

		// clear list
		struct work_item *item = work_items;
		CLEAR_LIST(work_items);

		pthread_mutex_unlock(&worker_mutex);
		
		// process each item in the work queue
		while (item) {
			struct work_item *prev_item = item;

			// if no image bytes were provided try loading the image from disk
			// otherwise save the image
			if (!item->image_data->bytes) {
				LOG("no need to fetch/update image: %s\n", item->image_data->url);
				get_cached_image_for_url(item->image_data);
			} else {
				LOG("saving updated image and etag: %s\n", item->image_data->url);
				save_image_for_url(item->image_data->url, item->image_data);
			}

			// call the provided image load callback with image data
			image_load_callback(item->image_data);	

			// move to the next item in the list and free memory for the processed item
			item = item->next;

			free(prev_item->image_data->bytes);
			free(prev_item->image_data->url);
			free(prev_item->image_data);
			free(prev_item);
		}
	
		pthread_mutex_lock(&worker_mutex);
	}

	// free any remaining work items
	while (work_items) {
		struct work_item *prev_item = work_items;
		work_items = work_items->next;

		free(prev_item->image_data->bytes);
		free(prev_item->image_data->url);
		free(prev_item->image_data);
		free(prev_item);
	}

	pthread_mutex_unlock(&worker_mutex);
	return NULL;
}

