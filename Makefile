# --- Variables ---
CC = gcc

CFLAGS = -Wall -Iinclude -fPIC


LDFLAGS = -L. -Wl,-rpath,.

LIB_NAME = libmfrc522.so
ADMIN_BIN = admin_tool
DAEMON_BIN = rfid_daemon

# --- Source Files ---
LIB_SRCS = src/SPI.c src/MFRC522.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

MAIN_SRC = main.c
DAEMON_SRC = rfid_daemon.c

# --- Targets ---

all: $(LIB_NAME) $(ADMIN_BIN) $(DAEMON_BIN)

$(LIB_NAME): $(LIB_OBJS)
	$(CC) -shared -o $@ $^
	@echo "Shared library $(LIB_NAME) created."

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


$(ADMIN_BIN): $(MAIN_SRC) $(LIB_NAME)
	$(CC) $(CFLAGS) $(MAIN_SRC) $(LDFLAGS) -lmfrc522 -o $(ADMIN_BIN)
	@echo "Binary $(ADMIN_BIN) created."
$(DAEMON_BIN): $(DAEMON_SRC) $(LIB_NAME)
	$(CC) $(CFLAGS) $(DAEMON_SRC) $(LDFLAGS) -lmfrc522 -o $(DAEMON_BIN)
	@echo "Binary $(DAEMON_BIN) created."


clean:
	rm -f src/*.o *.so $(ADMIN_BIN)
	@echo "Cleanup complete."

run:
	./$(ADMIN_BIN)

