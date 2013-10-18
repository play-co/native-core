CC=clang
CFLAGS=-ggdb -I./include
LDFLAGS=-lcurl

all: src/image_cache.c test/test.c
	$(CC) -o imagecache src/image_cache.c test/test.c $(CFLAGS) $(LDFLAGS)
