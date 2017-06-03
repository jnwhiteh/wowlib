LUA_DIR=/usr/local
LUA_INCDIR=$(LUA_DIR)/include
LUA_LIBDIR=$(LUA_DIR)/lib/lua/5.1
LIBFLAG=-shared

lwowlib.so: lwowlib.o
	$(CC) $(CFLAGS) $(LIBFLAG) -o wow.so lwowlib.o

lwowlib.o: lwowlib.c
	$(CC) -c -o lwowlib.o lwowlib.c -I$(LUA_INCDIR)

install: lwowlib.so
	mkdir -p $(LUA_LIBDIR)
	cp wow.so $(LUA_LIBDIR)

clean:
	rm -f wow.so lwowlib.o
