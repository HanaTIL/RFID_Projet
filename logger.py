import socket
import sqlite3
import signal
import sys

# 1. Setup SQLite Database
def init_db():
    conn = sqlite3.connect("rfid_access.db")
    cursor = conn.cursor()
    # Create a table for logs: ID, Name, and Timestamp
    cursor.execute('''CREATE TABLE IF NOT EXISTS access_logs 
                      (id INTEGER PRIMARY KEY AUTOINCREMENT, 
                       name TEXT, 
                       timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)''')
    conn.commit()
    return conn

# 2. Connect to the C Daemon "Phone Line"
SOCKET_PATH = "/tmp/rfid.sock"

def main():
    db_conn = init_db()
    cursor = db_conn.cursor()

    print(f"[*] Connecting to C Daemon at {SOCKET_PATH}...")
    
    try:
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect(SOCKET_PATH)
        print("[+] Connected! Waiting for RFID scans...")

        while True:
            # 3. Receive exactly 16 bytes (The Name)
            data = client.recv(16)
            if not data:
                print("[!] C Daemon closed the connection.")
                break

            # 4. Clean the data (remove trailing spaces/nulls)
            # Use 'latin-1' or 'ascii' to match your C buffer
            raw_name = data.decode('ascii', errors='ignore').strip()
            
            if raw_name:
                print(f"[EVENT] Scanned: {raw_name}")

                # 5. Save to SQLite
                cursor.execute("INSERT INTO access_logs (name) VALUES (?)", (raw_name,))
                db_conn.commit()
                print(f"[DB] Logged {raw_name} to rfid_access.db")

    except FileNotFoundError:
        print(f"[ERROR] Socket file {SOCKET_PATH} not found. Is the C Daemon running?")
    except ConnectionRefusedError:
        print("[ERROR] Connection refused. Start the C Daemon first!")
    except KeyboardInterrupt:
        print("\n[*] Shutting down...")
    finally:
        db_conn.close()
        print("[*] Database closed.")

if __name__ == "__main__":
    main()
