#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "core/log.h"
 
#include "curl/curl.h"
#include "crypto/sha1.h"

#include "image_cache.h"

struct data {
	char *bytes;
	size_t size;
	bool is_image;
};

struct image_data *image_cache_fetch_remote_image(const char *url, const char *etag);
static size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t real_size = size * nmemb;
	struct data *d = (struct data*)userp;
	size_t new_size = d->size + real_size + (d->is_image ? 0 : 1);
	d->bytes = (char*)realloc(d->bytes, new_size);
	if (d->bytes == NULL) {
		//ran out of memory!
		printf("Error, out of memory!\n");
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

static CURL *curl_handle;
static char *file_cache_path;

struct image_data *image_cache = NULL;
static const char *etag_file_name = ".etags";

void add_etags_to_memory_cache(char *f) {
    char *url = strtok(f, " ");
    while (url) {
        char *etag = strtok(NULL, "\n");
        if (etag) {
            printf("adding etag %s : %s\n", url, etag);
			struct image_data *data = (struct image_data*)malloc(sizeof(struct image_data));
			data->bytes = NULL;
			data->size = 0;
			data->etag = strdup(etag);
			data->url = strdup(url);
			HASH_ADD_KEYPTR(hh, image_cache, data->url, strlen(data->url), data);
		}
        url = strtok(NULL, " ");
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
			printf("Error opening etags cache file.  Does the directory exist?\n");
		} else {
			fseek(f, 0, SEEK_END);
			size_t file_size = ftell(f);
			rewind(f);

			char *file_buffer = (char*)malloc(sizeof(char) * file_size);
			if (!file_buffer) {
				printf("error ran out of memory!\n");
			} else {
				size_t bytes_read = fread(file_buffer, 1, file_size, f);
				if (bytes_read != file_size) {
					//printf("error reading file: size %li read %li\n", file_size, bytes_read);
				}
			}
			fclose(f);
            printf("============== read file\n\n%s\n\n===========", file_buffer);
			
            add_etags_to_memory_cache(file_buffer);
            
            free(file_buffer);

		}
	}
	free(etags_file_path);
}

void write_etags_to_cache() {
	struct image_data *data = NULL;
	struct image_data *tmp = NULL;
	char *path = get_full_path(etag_file_name);
	FILE *f = fopen(path, "w");
	static const char *format = "%s %s\n";
	HASH_ITER(hh, image_cache, data, tmp) {
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

void image_cache_init(const char *path) {
	curl_global_init(CURL_GLOBAL_ALL);	
	curl_handle = curl_easy_init();
	file_cache_path = strdup(path);
	read_etags_from_cache();
}

void clear_cache() {
	struct image_data *data = NULL;
	struct image_data *tmp = NULL;
	HASH_ITER(hh, image_cache, data, tmp) {
		free((void*)data->url);
		free((void*)data->etag);
		free(data);
	}
}

void image_cache_destroy() {
	write_etags_to_cache();
	clear_cache();
	curl_easy_cleanup(curl_handle);
    free(file_cache_path);
}


const char *get_etag_for_url(const char *url) {
	const char *etag = NULL;
	struct image_data *data = NULL;
	HASH_FIND_STR(image_cache, url, data);
	if (data) {
		etag = data->etag;	
	} else {
		printf("didn't find image in cache\n");
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
		printf("Error opening cache file for url %s\n", image_data->url);
	} else {
		fseek(f, 0, SEEK_END);
		size_t file_size = ftell(f);
		rewind(f);

		file_buffer  = (char*)malloc(sizeof(char) * file_size);
		if (!file_buffer) {
			printf("error ran out of memory!\n");
		} else {
			size_t bytes_read = fread(file_buffer, 1, file_size, f);
			if (bytes_read != file_size) {
				//printf("error reading file - size: %li read: %li\n", file_size, bytes_read);
				file_buffer = NULL;
			}
		}
        image_data->bytes = file_buffer;
        image_data->size = file_size;

	}
}

bool save_image_and_etag_for_url(const char *url, struct image_data *image) {
	char *filename = get_filename_from_url(url);
	char *path = get_full_path(filename); 
	free(filename);
	//TODO ERROR CHECKING!!!!!!!!!!
	FILE *f = fopen(path, "wb");
//	size_t bytes_written = 
    fwrite(image->bytes, sizeof(char), image->size, f);
//	LOG("wrote %d bytes\n", bytes_written);
	fclose(f);
	free(path);
    write_etags_to_cache();
	return true;	
}

bool image_exists_in_cache(const char *url) {
	char *filename = get_filename_from_url(url);
	char *file_path = get_full_path(filename);
	free(filename);
	bool exists = access(file_path, F_OK) != -1;
	free(file_path);
	return exists;
}

struct image_data *image_cache_get_image(const char *url) {
	const char *etag = NULL;
	if (image_exists_in_cache(url)) {
		//if we don't have a local copy cached, don't bother with
		//etags - we need to download it again
		etag = get_etag_for_url(url);
		printf("local copy, we might not have to get a new one\n");
	} else {
		printf("no local copy, fetching remote\n");
	}
	struct image_data *image_data = image_cache_fetch_remote_image(url, etag);
	if (!image_data->bytes) {
		printf("didn't fetch - getting cached version\n");
		get_cached_image_for_url(image_data);
	} else {
		printf("saving updated image and etag\n");
		save_image_and_etag_for_url(url, image_data);
	}
	return image_data;
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

struct image_data *image_cache_fetch_remote_image(const char *url, const char *etag) {
	struct data image;
	image.bytes = (char*)malloc(1);
	image.size = 0;
	image.is_image = false;
	struct data header;
	header.bytes = (char*)malloc(1);
	header.size = 0;
	header.is_image = false;
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	struct curl_slist *headers = NULL;
	if (etag) {
		printf("we have an etag, sending it to the server\n");
		static const char *etag_header_format = "If-None-Match: \"%s\"";
		size_t header_str_len = strlen(etag_header_format) + strlen(etag) + 1;
		char *etag_header_str = (char*)malloc(sizeof(char)*header_str_len);
		snprintf(etag_header_str, header_str_len, etag_header_format, etag);
		headers = curl_slist_append(headers, etag_header_str);
		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
		free(etag_header_str);
	}
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, false);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, true);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl_handle,   CURLOPT_WRITEHEADER, &header);
	curl_easy_setopt(curl_handle,   CURLOPT_WRITEDATA, &image);
	
	curl_easy_perform(curl_handle);
	struct image_data *image_data = NULL;
	HASH_FIND_STR(image_cache, url, image_data);
	if (!image_data) {
		image_data = (struct image_data*)malloc(sizeof(struct image_data));
		image_data->url = strdup(url);
		image_data->size = 0;
		image_data->bytes = NULL;
		if (etag) {
			image_data->etag = strdup(etag);
		} else {
			image_data->etag = NULL;
		}
		HASH_ADD_KEYPTR(hh, image_cache, image_data->url, strlen(image_data->url), image_data);
	}
	
	if (image.size > 0) {
		printf("got an updated image!\n");
		//we got an image back
		image_data->bytes = image.bytes;
		image_data->size = image.size;
		free(image_data->etag);
		image_data->etag = strdup(parse_etag_from_headers(header.bytes));
        save_image_and_etag_for_url(url, image_data);

	} else {
		printf("didn't get an image from the server - probably already up to date\n");
	}

	free(header.bytes);
	return image_data;
}
 

