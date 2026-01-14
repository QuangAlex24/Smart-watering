import os
import glob

def keep_latest_images(folder_path="detections", limit=10):
    # Lấy tất cả file ảnh trong thư mục
    image_files = glob.glob(os.path.join(folder_path, "*.jpg")) + \
                  glob.glob(os.path.join(folder_path, "*.png")) + \
                  glob.glob(os.path.join(folder_path, "*.jpeg"))

    # Nếu tổng số ảnh > giới hạn
    if len(image_files) > limit:
        # Sắp xếp theo thời gian chỉnh sửa (cũ → mới)
        image_files.sort(key=os.path.getmtime)

        # Tính số ảnh cần xóa
        to_delete = len(image_files) - limit

        # Xóa ảnh cũ
        for i in range(to_delete):
            print("Deleting:", image_files[i])
            os.remove(image_files[i])

# ========= SỬ DỤNG =========
keep_latest_images("detections", limit=10)
