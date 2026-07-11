CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2
SRCS := $(wildcard *.c)
TARGET := bed

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
