LUA_LIB=lua
#LUA_LIB=lua5.1
SHARED_OPTION=bundle
#SHARED_OPTION=shared

CC=gcc
CFLAGS=-std=c99 -Werror -pedantic -fPIC -ggdb -Os

INCLUDE=-I/usr/local/include -I/opt/local/include -I/usr/include/lua5.1
LIBS=-L/usr/local/lib -L/usr/lib -L/opt/local/lib

BLOCKS_SOURCES=src/blocks/blocks.c src/blocks/process.c src/blocks/lua_util.c src/blocks/mailbox.c
BLOCKS_OBJECTS=$(BLOCKS_SOURCES:.c=.o)
BLOCKS_LDFLAGS=-$(SHARED_OPTION) -fPIC -l$(LUA_LIB) -lpthread
LIBRARY=blocks.so

EVA_SOURCES=src/http/main.c
EVA_OBJECTS=$(EVA_SOURCES:.c=.o)
MAIN_EXEC=eva
EVA_LDFLAGS=-lmicrohttpd


all: $(BLOCKS_SOURCES) $(LIBRARY) $(EVA_SOURCES) $(MAIN_EXEC)
	
$(LIBRARY): $(BLOCKS_OBJECTS) 
	$(CC) $(LIBS) $(BLOCKS_LDFLAGS) $(BLOCKS_OBJECTS) -o $@

$(MAIN_EXEC): $(EVA_OBJECTS) 
	$(CC) $(LIBS) $(EVA_LDFLAGS) $(EVA_OBJECTS) -o $@

.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) $< -o $@

clean:
	rm $(BLOCKS_OBJECTS)
