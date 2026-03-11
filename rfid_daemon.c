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

    //  Initialize Hardware (via .so)
    MFRC522_init();
    printf("[DAEMON] Hardware Initialized.\n");

    //  Setup Unix Domain Socket
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

    //  Wait for Python to connect (Blocking)
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) { perror("accept"); exit(1); }
    printf("[DAEMON] Python Connected! Monitoring for cards...\n");

    int check_timer = 0;


    //  The Infinite Scan Loop
for (;;) {
    // 1. Basic check: Is a card there?
    if (PICC_IsNewCardPresent() && PICC_ReadCardSerial()) {
        
        // Prepare a fresh 20-byte packet
        unsigned char packet[20] = {0}; 
        
        // Always put the UID in the first 4 bytes
        memcpy(packet, UID_t.uidByte, 4);

        // Try to read the name (Block 1)
        if (MIFARE_Read(PICC_CMD_MF_AUTH_KEY_A, 0x01, defaultKey, rfid_buffer) == 0) {
            // Success: Add name to the next 16 bytes
            memcpy(&packet[4], rfid_buffer, 16);
            printf("[DAEMON] Full Read: Sending to Python...\n");
        } else {
            // Failure: Put "Unknown" in the name slot so Python isn't stuck
            memcpy(&packet[4], "Unknown         ", 16);
            printf("[DAEMON] Partial Read (UID only): Sending to Python...\n");
        }

        // CRITICAL: Always send 20 bytes to unblock Python's recv(20)
        if (send(client_fd, packet, 20, 0) < 0) {
            printf("[CONN] Python lost. Waiting for reconnect...\n");
            close(client_fd);
            client_fd = accept(server_fd, NULL, NULL);
        }

        MIFARE_StopSession();
    } else {
         check_timer++;

    if (check_timer >= 25) { // Check every 5 seconds
        byte ver = MFRC522_ReadRegister(VersionReg);
        byte mode = MFRC522_ReadRegister(TModeReg); // 0x2A

        // If version is gone OR if the chip reset to factory defaults (mode != 0x80)
        if (ver == 0x00 || ver == 0xFF || mode != 0x80) {
            printf("[RECOVERY] Power loss or disconnect detected. Re-initializing...\n");
            MFRC522_init(); 
        }
        
        check_timer = 0; 
    }
    }
    usleep(200000); // 0.2s pause
}


    close(server_fd);
    return 0;
}
