CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11
LDFLAGS ?=
TARGET = bridge-engine

all: $(TARGET)

$(TARGET): bridge_engine.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) bridge_engine.c

clean:
	rm -f $(TARGET)
