import socket
import sqlite3
import time

def gatekeeper():
    while True: # Fixed 'true' to 'True'
        try:
            # Connect to C Daemon
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            client.connect("/tmp/rfid.sock")
            print("Connected to RFID Daemon...")

            while True:
                try:
                    # Receive data
                    raw = client.recv(20)
                    if not raw: 
                        print("Daemon closed connection. Retrying...")
                        break
                
                    
                    # Skip empty packets if the daemon sent zeros on error
                    if all(b == 0 for b in raw):
                        continue

                    uid = ":".join(f"{b:02X}" for b in raw[:4])
                    name = raw[4:].decode('ascii', errors='ignore').strip()

                    # CHECK SECURITY DATABASE
                    with sqlite3.connect("security.db") as db_sec:
                        cursor = db_sec.cursor()
                        cursor.execute("SELECT name FROM authorized_users WHERE uid=?", (uid,))
                        is_authorized = cursor.fetchone()

                    status = "ALLOWED" if is_authorized else "DENIED"

                    # SAVE TO DATALOGGER DATABASE
                    with sqlite3.connect("datalogger.db") as db_log:
                        db_log.execute("INSERT INTO scans (uid, name_on_card, status) VALUES (?, ?, ?)", 
                                    (uid, name, status))
                        db_log.commit()

                    # PRINT RESULT
                    if is_authorized:
                        print(f"[ACCESS GRANTED] Welcome {is_authorized[0]}!")
                    else:
                        print(f"[ACCESS DENIED] Unknown Card: {uid}")

                except socket.error as e:
                    print(f"Socket error during read: {e}")
                    break # Break inner loop to reconnect

        except (ConnectionRefusedError, FileNotFoundError):
            print("Daemon not ready. Retrying in 2s...")
            time.sleep(2)
        except Exception as e:
            print(f"Unexpected error: {e}")
            time.sleep(2)

if __name__ == "__main__":
    gatekeeper()
