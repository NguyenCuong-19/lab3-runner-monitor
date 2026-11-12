# Lab 3: Runner Monitor (ESP32 + FreeRTOS + MQTT)

Sinh viên: Nguyễn Văn Cường   
MSSV: 231432185  
Lớp: N02

Hệ thống giám sát và cảnh báo người chạy: ESP32 đọc BMP180 (nhiệt độ, áp suất, độ cao),
chạy 3 task FreeRTOS, gửi dữ liệu lên MQTT broker và điều khiển LED/Loa từ web.

## Liên kết
- Mô phỏng Wokwi: https://wokwi.com/projects/480463985996579841
- Báo cáo đầy đủ: [Lap3_NguyenVanCuong.docx](Lap3_NguyenVanCuong.docx)
- Demo web: https://nguyencuong-19.github.io/lab3-runner-monitor/

## Cấu trúc
- `ESP32/sketch.ino`: code ESP32 (3 task: truyền data Core 0, đọc data và cảnh báo Core 1)
- `ESP32/diagram.json`: sơ đồ mạch Wokwi
- `ESP32/libraries.txt`: thư viện (Adafruit BMP085, PubSubClient)
- `index.html`, `style.css`, `script.js`: giao diện web
