# 🌱 Smart-watering Sử Dụng ESP32 & ESP32-CAM

## 📌 Giới thiệu

Dự án Chậu cây thông minh ứng dụng IoT và Computer Vision, sử dụng ESP32 và ESP32-CAM để:

Giám sát độ ẩm đất

Tự động tưới cây

Phát hiện sâu bệnh bằng camera

Gửi thông báo cho người dùng qua ứng dụng

Điều khiển và theo dõi hệ thống thông qua Node-RED

Hệ thống giúp người dùng chăm sóc cây trồng hiệu quả, giảm công sức và tăng năng suất.

# 🎯 Mục tiêu dự án

Tự động hóa quá trình tưới cây dựa trên độ ẩm đất

Phát hiện sâu bệnh sớm bằng hình ảnh từ ESP32-CAM

Gửi cảnh báo đến người dùng khi phát hiện sâu

Cho phép điều khiển thủ công hoặc tự động qua giao diện

Hiển thị và quản lý dữ liệu thời gian thực trên Node-RED

# 🧩 Chức năng chính
🌿 Giám sát & tưới cây

Đọc dữ liệu từ cảm biến độ ẩm đất

Tự động bật/tắt motor bơm nước thông qua relay

Cho phép người dùng:

Cài đặt ngưỡng độ ẩm (%)

Điều chỉnh thời gian tưới (2–3s hoặc hơn)

Điều khiển tưới thủ công

# 📷 Phát hiện sâu bệnh

ESP32-CAM chụp ảnh cây trồng

Gửi ảnh về server xử lý (YOLO / AI model)

Khi phát hiện sâu:

Gửi thông báo cho người dùng qua app

Hiển thị cảnh báo trên Node-RED

# 🖥️ Giao diện Node-RED

Hiển thị:

Độ ẩm đất (%)

Trạng thái bơm nước

Trạng thái phát hiện sâu

Điều khiển:

Bật/tắt tưới nước

Chuyển chế độ Auto / Manual

Điều khiển servo xoay camera

# 🔧 Phần cứng sử dụng

ESP32

ESP32-CAM

Cảm biến độ ẩm đất

Servo MG90S (xoay camera)

Relay

Motor bơm nước

Đèn LED

Màn hình OLED (tùy chọn)

# 💻 Phần mềm & Công nghệ

Arduino IDE (lập trình ESP32)

Python (xử lý ảnh, AI, server)

YOLO (phát hiện sâu bệnh)

Node-RED (dashboard & điều khiển)

MQTT / HTTP REST API

WiFi
