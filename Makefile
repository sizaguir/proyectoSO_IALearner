CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS_X11 = -lX11
LDFLAGS_PTHREAD = -lpthread

BIN_DIR = bin

LEARNER = $(BIN_DIR)/ialearner
CLIENT = $(BIN_DIR)/window_client
LAUNCHER = $(BIN_DIR)/launcher

all: folders $(LEARNER) $(CLIENT) $(LAUNCHER)

folders:
	mkdir -p $(BIN_DIR)

$(LEARNER): ialearner.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS_PTHREAD)

$(CLIENT): window_client.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS_X11) $(LDFLAGS_PTHREAD)

$(LAUNCHER): launcher.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BIN_DIR)/*