LUA_LIB=lua
#LUA_LIB=lua5.1
SHARED_OPTION=bundle
#SHARED_OPTION=shared

CC=gcc
CFLAGS=-std=c99 -Werror -pedantic -fPIC -ggdb -Os
LDFLAGS=-$(SHARED_OPTION) -fPIC -l$(LUA_LIB) -lpthread
INCLUDE=-I/usr/local/include -I/opt/local/include -I/usr/include/lua5.1
LIBS=-L/usr/local/lib -L/usr/lib -L/opt/local/lib
SOURCES=src/blocks/blocks.c src/blocks/thread.c src/blocks/lua_util.c src/blocks/mailbox.c
OBJECTS=$(SOURCES:.c=.o)
LIBRARY=blocks.so

all: $(SOURCES) $(LIBRARY)
	
$(LIBRARY): $(OBJECTS) 
	$(CC) $(LIBS) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS)
