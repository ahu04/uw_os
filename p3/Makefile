
# Add your own .h files to the right side of the assingment below.
INCLUDES = wsh.h seq.h parse.h

# Do all C compies with gcc (at home you could try clang)
CC = gcc

LDFLAGS = -g -L. -lcii
IFLAGS = -I. 
CFLAGS =  -g -O2 -std=gnu18 -Wall -Wextra -Werror -Wpedantic $(IFLAGS)
# 
#    'make all' will build all$ executables
#
#    Note that "all" is the default target that make will build
#    if nothing is specifically requested
#
all: wsh

# 
#    'make clean' will remove all object and executable files
#
clean:
	rm -f wsh *.o

# 
#    To get any .o, compile the corresponding .c
#
%.o:%.c $(INCLUDES) 
	$(CC) $(CFLAGS) -c $< $(LDFLAGS)

#
# Individual executables
#
#    Each executable depends on one or more .o files.
#    Those .o files are linked together to build the corresponding
#    executable.
#
wsh: wsh.o parse.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: wsh
	./wsh

dbg: wsh
	valgrind ./wsh

fmt: 
	clang-format -i *.[ch]
