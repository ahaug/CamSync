
TARGET  := ../bin/example3

CC      := g++
CFLAGS  := -g -Wall -I../../include
LDFLAGS := -L ../.. -lFCam -ljpeg -lpthread
FILES   := example3

OBJS = $(addsuffix .o, $(FILES))

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

build: $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -rf *.o $(TARGET) .\#* \#* *~ core
