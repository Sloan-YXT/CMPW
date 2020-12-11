server:server.o data.o database.o cutil.o
	gcc server.o data.o database.o cutil.o -lpthread -lmysqlclient -lcjson -o server
CFLAGS=-w
ifdef DEBUG
CFLAGS+= -g
endif
server.o:server.c
	gcc -c server.c -o server.o $(CFLAGS)
%.o:%.c %.h
	gcc -c $< -o $@ $(CFLAGS)
clean:
	rm server *.o
.PHONY:clean