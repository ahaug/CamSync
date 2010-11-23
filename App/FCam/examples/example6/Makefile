
TARGET  := ../bin/example6

CC      := g++
CFLAGS  := -g -Wall -I../../include
LDFLAGS := -L ../.. -lFCam -ljpeg -lpthread -lpulse-simple
FILES   := example6 SoundPlayer

OBJS = $(addsuffix .o, $(FILES))

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

build: $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -rf *.o $(TARGET) .\#* \#* *~ core
