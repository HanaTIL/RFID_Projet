import sqlite3

# --- Create the SECURITY Database (The Rules) ---
conn_sec = sqlite3.connect("security.db")
conn_sec.execute("CREATE TABLE IF NOT EXISTS authorized_users (uid TEXT PRIMARY KEY, name TEXT)")
# Add yourself to the whitelist
conn_sec.execute("INSERT OR IGNORE INTO authorized_users VALUES ('74:FE:01:04', 'Hana Til')")
conn_sec.commit()
conn_sec.close()

# --- Create the DATALOGGER Database (The History) ---
conn_log = sqlite3.connect("datalogger.db")
conn_log.execute("CREATE TABLE IF NOT EXISTS scans (uid TEXT, name_on_card TEXT, status TEXT, time DATETIME DEFAULT CURRENT_TIMESTAMP)")
conn_log.commit()
conn_log.close()

print("Databases initialized: security.db and datalogger.db")
