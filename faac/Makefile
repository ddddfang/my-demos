MODULE_BIN = encode_audio

#MODULE_OBJS += $(patsubst %.c,%.o ,$(wildcard ./*.c))
MODULE_SRCS = $(shell find . -name "*.c")
MODULE_OBJS += $(patsubst %.c,%.o ,$(MODULE_SRCS))

CC = gcc
OUT_DIR = .
PUBLIC_FLAGS:=-g -O0 -Wall -std=gnu99

LDFLAGS = -lm -lz -lpthread  
LDFLAGS+= -L/usr/local/lib -lfaac
MODULE_INCLUDE = -I.
MODULE_INCLUDE+= -I-I/usr/local/include

%.o: %.c
	$(CC) $(PUBLIC_FLAGS) $(MODULE_INCLUDE) -c $< -o $@

all : clean $(MODULE_OBJS)
	$(CC) $(MODULE_OBJS) $(PUBLIC_FLAGS) $(LDFLAGS) -o $(OUT_DIR)/$(MODULE_BIN)

clean:
	-rm -f $(OUT_DIR)/$(MODULE_BIN)
	-rm -f $(MODULE_OBJS)
