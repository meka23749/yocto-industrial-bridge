CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = bridge-engine

all: $(TARGET)

$(TARGET): bridge_engine.c
	$(CC) $(CFLAGS) -o $(TARGET) bridge_engine.c

clean:
	rm -f $(TARGET)
