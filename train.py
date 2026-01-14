from ultralytics import YOLO

model = YOLO("yolo11n.pt")  # dùng model có sẵn để train tiếp

model.train(
    data="G:/esp32-camera/yolocamera/dataset/data.yaml", 
    epochs=50,
    imgsz=640,
    batch=8
)
