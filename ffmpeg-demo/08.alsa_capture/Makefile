# pkg-config  --cflags --libs alsa

MODULE_BIN = out

#MODULE_OBJS += $(patsubst %.c,%.o ,$(wildcard ./*.c))
MODULE_SRCS = $(shell find . -name "*.cpp")
MODULE_OBJS += $(patsubst %.cpp, %.o ,$(MODULE_SRCS))

CC = g++
OUT_DIR = .
PUBLIC_FLAGS:=-g -O0 -Wall

LDFLAGS = -lm -lz -lpthread  
LDFLAGS += -lasound

MODULE_INCLUDE = -I.
MODULE_INCLUDE += -I/usr/include/alsa

%.o: %.cc
	$(CC) $(PUBLIC_FLAGS) -std=c++11 $(MODULE_INCLUDE) -c $< -o $@

all : clean $(MODULE_OBJS)
	$(CC) $(MODULE_OBJS) $(PUBLIC_FLAGS) $(LDFLAGS) -o $(OUT_DIR)/$(MODULE_BIN)

clean:
	-rm -f $(OUT_DIR)/$(MODULE_BIN)
	-rm -f $(MODULE_OBJS)
