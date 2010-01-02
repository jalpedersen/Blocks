CC=gcc
CFLAGS=-std=c99 -Werror -pedantic -fpic -ggdb -Os 
LDFLAGS=-shared -fpic -llua -lpthread
INCLUDE=-I/usr/local/include
LIBS=-L/usr/local/lib
SOURCES=src/blocks/blocks.c src/blocks/thread.c src/blocks/lua_util.c
OBJECTS=$(SOURCES:.c=.o)
LIBRARY=blocks.so

all: $(SOURCES) $(LIBRARY)
	
$(LIBRARY): $(OBJECTS) 
	$(CC) $(LIBS) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS)