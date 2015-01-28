#path

#param
CC 		= g++
CFLAGS 	= -g -c -Wall -fPIC -D__MYDBM_MAIN_TEST__
OBJS 	= mydbm.o log.o crc32.o

#.PHONY
.PHONY : all build clean

all : build clean

build : $(OBJS)
	$(CC) -o $@ $^

clean : 
	rm -fr $(OBJS)

#OBJS
$(OBJS) : %.o : %.cc
	$(CC) $(CFLAGS) -o $@ $<
