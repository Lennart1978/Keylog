CC = gcc
CFLAGS = -Os -s -Wall -Wextra -std=c99 -pedantic -fdata-sections -ffunction-sections -fno-asynchronous-unwind-tables -fno-ident -D_GNU_SOURCE
LDFLAGS = -Wl,--gc-sections

TARGET = keylog
SRC = keylog.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete. Binary size:"
	@size $(TARGET)
	@ls -lh $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
