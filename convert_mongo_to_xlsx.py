import pandas as pd
from pymongo import MongoClient

# --- Koneksi MongoDB ---
client = MongoClient("")
db = client["dispenserDB"]

# --- Ambil data dari dua koleksi ---
data_levels = list(db["water_levels"].find())
data_usage = list(db["water_usage"].find())

# --- Convert ke DataFrame ---
df_levels = pd.DataFrame(data_levels)
df_usage = pd.DataFrame(data_usage)

# --- Drop kolom "_id" jika tidak diperlukan ---
df_levels.drop(columns=["_id"], inplace=True, errors="ignore")
df_usage.drop(columns=["_id"], inplace=True, errors="ignore")

# --- Simpan ke file Excel dengan dua sheet ---
with pd.ExcelWriter("data_dispenser.xlsx", engine="openpyxl") as writer:
    df_levels.to_excel(writer, sheet_name="Water Levels", index=False)
    df_usage.to_excel(writer, sheet_name="Water Usage", index=False)

print("✅ Data berhasil diekspor ke 'data_dispenser.xlsx'")
