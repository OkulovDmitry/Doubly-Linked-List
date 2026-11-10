CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -g
SRCS := list.c 
OBJS := $(SRCS:.c=.o)

ifeq ($(OS), Windows_NT)
	TARGET := list_test.exe
    RUN    := .\$(TARGET)
    RM     := del /Q /F
else
	TARGET := list_test
    RUN    := ./$(TARGET)
    RM     := rm -f
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c list.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	-$(RM) $(OBJS) $(TARGET) *.dot *.png list_dumps.txt

.PHONY: all run clean