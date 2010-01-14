SHARED_OPTION=bundle
#SHARED_OPTION=shared
BLOCKS_EXTRA_LDFLAGS=-llua
#EVA_EXTRA_LDFLAGS=-Wl,-E

LUA_LOC=3rd-party/lua-5.1.4
LUA_INC=$(LUA_LOC)/src
LUA_STATIC_LIB=$(LUA_LOC)/src/liblua.a

CC=gcc
CFLAGS=-std=c99 -Werror -pedantic -fPIC -ggdb -Os

INCLUDE=-Isrc -I$(LUA_INC) -I/usr/local/include -I/opt/local/include -I/usr/include
LIBS=-L/usr/local/lib -L/usr/lib -L/opt/local/lib


BLOCKS_SOURCES=src/blocks/blocks.c src/blocks/process.c src/util/lua_util.c src/blocks/mailbox.c
BLOCKS_OBJECTS=$(BLOCKS_SOURCES:.c=.o)
BLOCKS_LDFLAGS=-$(SHARED_OPTION) -fPIC -lpthread $(BLOCKS_EXTRA_LDFLAGS)
LIBRARY=blocks.so

EVA_SOURCES=src/http/main.c src/util/lua_util.c
EVA_OBJECTS=$(EVA_SOURCES:.c=.o)
MAIN_EXEC=eva
EVA_LDFLAGS=-lmicrohttpd -lm -ldl $(EVA_EXTRA_LDFLAGS)


all: $(BLOCKS_SOURCES) $(LIBRARY) $(EVA_SOURCES) $(MAIN_EXEC)

$(LIBRARY): $(BLOCKS_OBJECTS) 
	$(CC) $(LIBS) $(BLOCKS_LDFLAGS) $(BLOCKS_OBJECTS) -o $@

$(MAIN_EXEC): $(EVA_OBJECTS) 
	$(CC) $(LIBS) $(EVA_LDFLAGS) $(EVA_OBJECTS) $(LUA_STATIC_LIB) -o $@

.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(BLOCKS_OBJECTS) $(EVA_OBJECTS)
