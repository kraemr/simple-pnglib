CC=gcc
CFLAGS= -Wall -shared -fPIC
SRCS=globals.c spnglib_read.c spnglib_read_txt.c spnglib_write.c spnglib_write_txt.c
OBJS=$(SRCS:.c=.o)
TARGET=spnglib.dll

all: $(TARGET)

$(TARGET): $(OBJS)
        $(CC) $(CFLAGS) -o $@ $^
        $(info Building $@ from $^)
%.o: %.c
        $(CC) -c $(CFLAGS) -o $@ $<

clean:
        rm -f $(OBJS) $(TARGET)











