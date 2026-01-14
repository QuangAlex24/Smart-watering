from ultralytics import YOLO
from flask import Flask, request, jsonify
import cv2
import numpy as np
import os
import time
import threading
import glob
import telebot

# ========== CONFIG ==========
BOT_TOKEN = "7718061334:AAEZAUD92cpOKpbckyAyhcAj1bcCFilQ4uk"
CHAT_ID = "6896847753"

MODEL_PATH = "runs/detect/train/weights/best.pt"
SAVE_PATH = "detections"
CONF_THRESHOLD = 0.4
MAX_IMAGES = 10   # Giới hạn 10 ảnh

# ========== INIT ==========
os.makedirs(SAVE_PATH, exist_ok=True)
app = Flask(__name__)
bot = telebot.TeleBot(BOT_TOKEN)

print("🔍 Đang tải model YOLO...")
model = YOLO(MODEL_PATH)
print("✅ YOLO load xong!")


# ========== DỌN ẢNH CŨ ==========
def cleanup_old_images():
    # Lấy tất cả ảnh .jpg .png .jpeg
    images = (
        glob.glob(f"{SAVE_PATH}/*.jpg") +
        glob.glob(f"{SAVE_PATH}/*.png") +
        glob.glob(f"{SAVE_PATH}/*.jpeg")
    )

    # Sắp xếp theo thời gian cũ → mới
    images = sorted(images, key=os.path.getmtime)

    # Nếu số lượng ảnh > MAX_IMAGES, xóa ảnh cũ
    if len(images) > MAX_IMAGES:
        remove_count = len(images) - MAX_IMAGES
        for img in images[:remove_count]:
            try:
                os.remove(img)
                print(f"🧹 Xóa ảnh cũ: {img}")
            except:
                pass


# ========== GỬI TELEGRAM ==========
def send_telegram_image(path, confidence):
    caption = f"🐛 PHÁT HIỆN SÂU! Độ tin cậy: {confidence:.2f}"
    with open(path, "rb") as f:
        bot.send_photo(CHAT_ID, f, caption=caption)
    print("📤 Đã gửi Telegram!")


# ========== API NHẬN ẢNH ==========
@app.route("/upload", methods=["POST"])
def upload():
    try:
        print("\n========================")
        print("📥 ESP32-CAM gửi ảnh lên!")

        # ---- Nhận dữ liệu ảnh ----
        jpg_bytes = request.data
        if not jpg_bytes:
            return jsonify({"error": "Không nhận được dữ liệu ảnh"}), 400

        npimg = np.frombuffer(jpg_bytes, np.uint8)
        img = cv2.imdecode(npimg, cv2.IMREAD_COLOR)
        if img is None:
            return jsonify({"error": "Không decode được ảnh"}), 400

        print("📸 Ảnh nhận OK!", img.shape)

        # ---- Lưu ảnh gốc ----
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        raw_path = f"{SAVE_PATH}/raw_{timestamp}.jpg"
        cv2.imwrite(raw_path, img)
        print(f"💾 Lưu ảnh gốc: {raw_path}")

        # Dọn ảnh cũ trong thư mục
        threading.Thread(target=cleanup_old_images).start()

        # ---- Chạy YOLO ----
        results = model.predict(
            source=img,
            conf=CONF_THRESHOLD,
            verbose=False
        )

        result = results[0]
        boxes = result.boxes
        print(f"🔍 Số vật thể phát hiện: {len(boxes)}")

        if len(boxes) > 0:
            detected_path = f"{SAVE_PATH}/detected_{timestamp}.jpg"
            result.save(filename=detected_path)

            confidence = float(boxes.conf[0])
            print(f"🐛 SÂU PHÁT HIỆN ({confidence:.2f})")

            # Gửi telegram
            send_telegram_image(detected_path, confidence)

            return jsonify({
                "status": "detected",
                "confidence": confidence
            })

        else:
            print("✅ Không phát hiện sâu.")
            return jsonify({"status": "no_object"})

    except Exception as e:
        print("❌ Lỗi xử lý:", e)
        return jsonify({"error": str(e)})


# ========== CHẠY SERVER ==========
if __name__ == "__main__":
    print("🚀 Server Flask + YOLO đang chạy tại port 5000...")
    app.run(host="0.0.0.0", port=5000)
