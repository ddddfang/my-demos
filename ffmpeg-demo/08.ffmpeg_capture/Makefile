MODULE_BIN1 = video_capture
MODULE_BIN2 = audio_capture

##MODULE_OBJS += $(patsubst %.c,%.o ,$(wildcard ./*.c))
#MODULE_SRCS = $(shell find . -name "*.c")
#MODULE_OBJS += $(patsubst %.c,%.o ,$(MODULE_SRCS))

MODULE_OBJS1 += ./ffCameraReader.o
MODULE_OBJS2 += ./ffMicroReader.o

CC = gcc
OUT_DIR = .
PUBLIC_FLAGS:=-g -O0 -Wall -std=gnu99

LDFLAGS = -lm -lz -lpthread  
LDFLAGS+= -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -L/usr/local/lib
MODULE_INCLUDE = -I.
MODULE_INCLUDE += -I/usr/local/include

all : clean $(MODULE_BIN1) $(MODULE_BIN2)
	@echo "done."

$(MODULE_BIN1) : $(MODULE_OBJS1)
	$(CC) $(MODULE_OBJS1) $(PUBLIC_FLAGS) $(LDFLAGS) -o $(OUT_DIR)/$(MODULE_BIN1)

$(MODULE_BIN2) : $(MODULE_OBJS2)
	$(CC) $(MODULE_OBJS2) $(PUBLIC_FLAGS) $(LDFLAGS) -o $(OUT_DIR)/$(MODULE_BIN2)

%.o: %.c
	$(CC) $(PUBLIC_FLAGS) $(MODULE_INCLUDE) -c $< -o $@

clean:
	-rm -f $(OUT_DIR)/$(MODULE_BIN1)
	-rm -f $(OUT_DIR)/$(MODULE_BIN2)
	-rm -f $(MODULE_OBJS1)
	-rm -f $(MODULE_OBJS2)
