import json
import requests
import ssl
from paho.mqtt.client import Client
from datetime import datetime
from pymongo import MongoClient
from pymongo.server_api import ServerApi


# --- KONFIGURASI MONGODB ---
MONGO_URI = "" # Ganti dengan URI MongoDB Anda
mongo_client = MongoClient(MONGO_URI, server_api=ServerApi('1'))
db = mongo_client["dispenserDB"]
collection_levels = db["water_levels"]
collection_usage = db["water_usage"]

# --- KONFIGURASI MQTT ---
MQTT_BROKER = "" # Ganti dengan alamat broker MQTT Anda
MQTT_PORT = 8883
MQTT_USERNAME = "" # Ganti dengan username MQTT Anda
MQTT_PASSWORD = "" # Ganti dengan password MQTT Anda
MQTT_TOPICS = [("dispenser/level", 0), ("dispenser/penggunaan", 0)]

# --- WEBHOOKS DISCORD ---
WEBHOOK_URL_WATERLEVEL = "" # Ganti dengan URL webhook untuk level air
WEBHOOK_URL_USAGE = "" # Ganti dengan URL webhook untuk penggunaan air
WATERLEVEL_MSG_ID = "" # Ganti dengan ID pesan Discord untuk level air

# --- UTILITY FUNCTIONS ---
def get_mililiter_from_miliseconds(duration_ms):
    volume_per_milisecond = 0.04045
    return round(duration_ms * volume_per_milisecond, 2)

def format_datetime_for_display(dt: datetime) -> str:
    return dt.strftime("%H:%M:%S - %d %B %Y")

def update_waterlevel_message(embed):
    edit_url = WEBHOOK_URL_WATERLEVEL + f"/messages/{WATERLEVEL_MSG_ID}"
    response = requests.patch(edit_url, json={"embeds": [embed]})
    if response.status_code == 200:
        print("✅ Discord water level message updated.")
    else:
        print(f"❌ Gagal update pesan water level: {response.status_code}")
        print(response.text)

def send_usage_embed(embed):
    response = requests.post(WEBHOOK_URL_USAGE, json={"embeds": [embed]})
    if response.status_code != 204:
        print(f"❌ Gagal kirim penggunaan: {response.status_code}")
        print(response.text)
    else:
        print("✅ Pemakaian air terkirim.")

# --- MQTT CALLBACKS ---
def on_connect(client, userdata, flags, rc):
    print("✅ Terhubung ke MQTT Broker dengan kode:", rc)
    client.subscribe(MQTT_TOPICS)

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        topic = msg.topic
        current_utc_time = datetime.now()

        if topic == "dispenser/level":
            level = payload.get("tank_level_cm", "N/A")

            embed = {
                "title": "📊 Level Tangki Air",
                "color": 0x1abc9c,
                "fields": [
                    {"name": "📏 Jarak Sensor ke Air", "value": f"{level} cm", "inline": True},
                    {"name": "🕒 Last Update", "value": f"<t:{int(current_utc_time.timestamp())}:R>", "inline": False}
                ],
                "footer": {"text": "Live Tank Monitoring by Kautsar"},
            }

            update_waterlevel_message(embed)
            print(f"🔵 Data level air baru diterima dan pesan Discord diupdate: {level} cm")

            try:
                collection_levels.insert_one({
                    "level_cm": level,
                    "timestamp": current_utc_time,
                    "raw_payload": payload
                })
                print("✅ Data water level disimpan ke MongoDB.")
            except Exception as e:
                print(f"❌ Gagal simpan ke MongoDB (level): {e}")

        elif topic == "dispenser/penggunaan":
            duration = payload.get("duration_ms", "N/A")
            volume_ml = get_mililiter_from_miliseconds(duration)

            embed = {
                "title": "🚰 Pemakaian Air",
                "color": 0xf39c12,
                "fields": [
                    {"name": "💧 Volume", "value": f"{volume_ml} ml", "inline": True},
                    {"name": "⏱️ Durasi", "value": f"{duration} ms", "inline": True},
                    {"name": "🕒 Waktu", "value": format_datetime_for_display(current_utc_time), "inline": False}
                ],
                "footer": {"text": "Automatic Dispenser by Kautsar"},
            }

            send_usage_embed(embed)

            try:
                collection_usage.insert_one({
                    "duration_ms": duration,
                    "volume_ml": volume_ml,
                    "timestamp": current_utc_time,
                    "raw_payload": payload
                })
                print("✅ Data pemakaian air disimpan ke MongoDB.")
            except Exception as e:
                print(f"❌ Gagal simpan ke MongoDB (usage): {e}")

    except Exception as e:
        print(f"❗ Error handle pesan MQTT: {e}")

# --- MAIN ---
def main():
    client = Client()
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.tls_set(cert_reqs=ssl.CERT_NONE)
    client.tls_insecure_set(True)

    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(MQTT_BROKER, MQTT_PORT)
    client.loop_forever()

if __name__ == "__main__":
    main()
