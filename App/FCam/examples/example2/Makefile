
TARGET  := ../bin/example2

CC      := g++
CFLAGS  := -g -Wall -I../../include
LDFLAGS := -L ../.. -lFCam -ljpeg -lpthread
FILES   := example2

OBJS = $(addsuffix .o, $(FILES))

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

build: $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -rf *.o $(TARGET) .\#* \#* *~ core
