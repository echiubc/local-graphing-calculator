CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
LDFLAGS ?= -mwindows -lgdi32 -luser32 -lcomdlg32 -lm
TARGET = graphcalc.exe
SRC = src/main.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	del /Q $(TARGET) 2>NUL || exit 0
