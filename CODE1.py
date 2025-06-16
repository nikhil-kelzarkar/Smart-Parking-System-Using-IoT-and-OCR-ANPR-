import cv2
import pytesseract
import serial
import sqlite3
from datetime import datetime
import time

# === SERIAL PORT SETTINGS ===
SERIAL_PORT = 'COM3'
BAUD_RATE = 9600

# === TESSERACT CONFIG ===
# Uncomment and set if tesseract is not in PATH
pytesseract.pytesseract.tesseract_cmd = r'C:\Program Files\Tesseract-OCR\tesseract.exe'

# === DATABASE SETUP ===
conn = sqlite3.connect("plates.db")
cursor = conn.cursor()
cursor.execute('''
CREATE TABLE IF NOT EXISTS vehicles (
    plate TEXT PRIMARY KEY,
    time TEXT
)
''')
conn.commit()

# === Open Serial Connection to ESP32 ===
esp = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
time.sleep(2)  # Allow ESP32 to reboot and get ready

# === Capture Image from Webcam ===
def capture_image():
    cam = cv2.VideoCapture(0)
    ret, frame = cam.read()
    cam.release()
    if not ret:
        print("❌ Failed to capture image")
        return None
    filename = "plate.jpg"
    cv2.imwrite(filename, frame)
    return filename

# === Extract Number Plate from Image ===
def recognize_plate(image_path):
    img = cv2.imread(image_path)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    result = pytesseract.image_to_string(gray, config='--psm 8')
    plate = ''.join(e for e in result if e.isalnum()).upper()
    return plate

# === Check and Update Database ===
def check_plate(plate):
    cursor.execute("SELECT * FROM vehicles WHERE plate = ?", (plate,))
    row = cursor.fetchone()
    if row:
        print(f"✅ Plate Found: {plate}")
    else:
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        cursor.execute("INSERT INTO vehicles (plate, time) VALUES (?, ?)", (plate, now))
        conn.commit()
        print(f"🆕 New Plate Logged: {plate} at {now}")

# === Trigger Servo via Serial ===
def open_gate():
    print("🔓 Opening Gate...")
    esp.write(b"SERVO_OPEN\n")

# === Main Flow ===
def main():
    print("📷 Monitoring for vehicle...")
    while True:
        input("🔔 Press Enter to simulate ultrasonic trigger...")  # Replace with GPIO ultrasonic trigger later

        img_path = capture_image()
        if img_path:
            plate = recognize_plate(img_path)
            if plate:
                print(f"🔍 Plate Detected: {plate}")
                check_plate(plate)
                open_gate()
            else:
                print("⚠️ No plate detected")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n🛑 Exiting...")
        conn.close()
        esp.close()
