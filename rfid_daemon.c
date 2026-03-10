#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "MFRC522.h"
#include "SPI.h" 
#include <sys/stat.h>


#define SOCKET_PATH "/tmp/rfid.sock"

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    byte rfid_buffer[18]; // 16 data + 2 CRC
    byte defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    spiFile = SPI_initPort("/dev/spidev0.0");
    if (spiFile < 0) {
        perror("Error opening SPI port");
        return 1;
    }

    // 1. Initialize Hardware (via your .so)
    MFRC522_init();
    printf("[DAEMON] Hardware Initialized.\n");

    // 2. Setup Unix Domain Socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH); // Remove existing socket file
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
        perror("bind"); exit(1);
    }
chmod("/tmp/rfid.sock", 0666); 
    if (listen(server_fd, 5) < 0) { perror("listen"); exit(1); }

    printf("[DAEMON] Waiting for Python client to connect at %s...\n", SOCKET_PATH);

    // 3. Wait for Python to connect (Blocking)
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) { perror("accept"); exit(1); }
    printf("[DAEMON] Python Connected! Monitoring for cards...\n");

    // 4. The Infinite Scan Loop
    for (;;) {
        if (PICC_IsNewCardPresent()) {
            if (PICC_ReadCardSerial()) {
                // Read Block 1 (where we wrote "Hana Til")
                // Use Key A (0x60) or your Private Key if you updated the trailer
                if (MIFARE_Read(PICC_CMD_MF_AUTH_KEY_A, 0x01, defaultKey, rfid_buffer) == 0) {
                    printf("[DAEMON] Card Detected! Sending data to Python...\n");
                    // 1. Create a 20-byte packet buffer
                    byte packet[20];
    
    // 2. Pack the 4 bytes of the UID (assuming UID_t is your global UID struct)
                    memcpy(packet, UID_t.uidByte, 4);
    
    // 3. Pack the 16 bytes of the Name
                    memcpy(&packet[4], rfid_buffer, 16);
                    // Send exactly 16 bytes to the Python client
                    if (send(client_fd, packet, 20, 0) < 0) {
        printf("[ERROR] Python disconnected. Waiting for reconnect...\n");
        close(client_fd);
        client_fd = accept(server_fd, NULL, NULL);
    }
                }
                
                // Cleanup for the next tap
                MIFARE_StopSession(); 
            }
        }
        usleep(200000); // 100ms "Breathing" for Linux
    }

    close(server_fd);
    return 0;
}
