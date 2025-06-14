import discord
from discord.ext import commands
from pymongo import MongoClient
from datetime import datetime
import matplotlib.pyplot as plt
import io
from matplotlib.ticker import MaxNLocator

# Bot setup
intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix="!", intents=intents)
dcbottoken = "" # Ganti dengan token bot Discord Anda

# MongoDB config
mongo_client = MongoClient("") # Ganti dengan URI MongoDB Anda
db = mongo_client["dispenserDB"]
collection_levels = db["water_levels"]
collection_usage = db["water_usage"]

# --- STATUS TERKINI ---
@bot.command(name="status")
async def status(ctx):
    latest = collection_levels.find_one(sort=[("timestamp", -1)])
    if latest:
        ts = latest["timestamp"].strftime("%d-%m-%Y %H:%M:%S")
        await ctx.send(f"📏 Jarak Sensor ke Air Terkini: **{latest['level_cm']} cm**\n🕒 Timestamp: {ts}")
    else:
        await ctx.send("❌ Tidak ada data yang tersedia.")

# --- GRAFIK LEVEL AIR (semua data) ---
@bot.command(name="grafik")
async def grafik(ctx):
    cursor = collection_levels.find().sort("timestamp", 1)

    timestamps = []
    levels = []

    for doc in cursor:
        timestamps.append(doc["timestamp"])
        levels.append(doc["level_cm"])

    if not levels:
        await ctx.send("❌ Tidak ada data level air yang tersedia.")
        return

    plt.figure(figsize=(10, 4))
    plt.plot(timestamps, levels, color="blue", marker="o")
    plt.title("Grafik Jarak Sensor ke Air")
    plt.xlabel("Waktu")
    plt.ylabel("Level (cm)")
    plt.grid(True)
    plt.tight_layout()

    buf = io.BytesIO()
    plt.savefig(buf, format="png")
    buf.seek(0)
    plt.close()

    await ctx.send(file=discord.File(buf, filename="grafik_level.png"))

# --- GRAFIK PEMAKAIAN AIR (sumbu x = data ke-n) ---
@bot.command(name="usagegrafik")
async def usagegrafik(ctx):
    cursor = collection_usage.find().sort("timestamp", 1)

    volumes = []
    indices = []

    for idx, doc in enumerate(cursor, start=1):
        volumes.append(doc.get("volume_ml", 0))
        indices.append(idx)

    if not volumes:
        await ctx.send("❌ Tidak ada data pemakaian air yang tersedia.")
        return

    plt.figure(figsize=(10, 4))
    plt.plot(indices, volumes, color="green", marker="o")
    plt.title("Grafik Pemakaian Air")
    plt.xlabel("Data ke-")
    plt.ylabel("Volume (ml)")
    plt.grid(True)
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))  # 👈 Paksa x-axis bulat
    plt.tight_layout()

    buf = io.BytesIO()
    plt.savefig(buf, format="png")
    buf.seek(0)
    plt.close()

    await ctx.send(file=discord.File(buf, filename="grafik_usage.png"))

# Jalankan bot
bot.run(dcbottoken)
