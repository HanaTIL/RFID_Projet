import socket
import sqlite3

def gatekeeper():
    # Connect to C Daemon
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect("/tmp/rfid.sock")

    while True:
        #  Receive data (4 bytes UID + 16 bytes Name = 20 total)
        raw = client.recv(20)
        if not raw: break
        
        uid = ":".join(f"{b:02X}" for b in raw[:4])
        name = raw[4:].decode('ascii', errors='ignore').strip()

        #  CHECK SECURITY DATABASE
        with sqlite3.connect("security.db") as db_sec:
            cursor = db_sec.cursor()
            cursor.execute("SELECT name FROM authorized_users WHERE uid=?", (uid,))
            is_authorized = cursor.fetchone()

        status = "ALLOWED" if is_authorized else "DENIED"

        #  SAVE TO DATALOGGER DATABASE
        with sqlite3.connect("datalogger.db") as db_log:
            db_log.execute("INSERT INTO scans (uid, name_on_card, status) VALUES (?, ?, ?)", 
                           (uid, name, status))
            db_log.commit()

        # PRINT RESULT
        if is_authorized:
            print(f"[ACCESS GRANTED] Welcome {is_authorized[0]}!")
        else:
            print(f"[ACCESS DENIED] Unknown Card: {uid}")

if __name__ == "__main__":
    gatekeeper()
