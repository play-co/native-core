# This builds a debug version of the image cache test app, which can be checked with valgrind et al

CC=clang
CFLAGS=-g -O0 -ggdb -I./include -DIMGCACHE_STANDALONE -DIMGCACHE_VERBOSE
LDFLAGS=-lcurl

all: src/image_cache.c src/murmur.c test/test.c
	$(CC) -o imagecache src/image_cache.c src/murmur.c test/test.c $(CFLAGS) $(LDFLAGS)
