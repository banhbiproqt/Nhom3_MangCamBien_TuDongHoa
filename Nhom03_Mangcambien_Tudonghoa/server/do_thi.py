import pandas as pd
import matplotlib.pyplot as plt

# Đọc dữ liệu từ file Excel
file_path = "E:/NhatTan/Minh/Nhom03_Mangcambien_Tudonghoa/server/data_export.xlsx"
excel_data = pd.ExcelFile(file_path)

# Đọc từng sheet
logs = pd.read_excel(excel_data, "Logs")
command_status = pd.read_excel(excel_data, "CommandStatus")
rain_status = pd.read_excel(excel_data, "RainStatus")

# Chuyển đổi thời gian về dạng datetime nếu cần
logs['timestamp'] = pd.to_datetime(logs['timestamp'], errors='coerce')
command_status['updatedAt'] = pd.to_datetime(command_status['updatedAt'], errors='coerce')
rain_status['updatedAt'] = pd.to_datetime(rain_status['updatedAt'], errors='coerce')

# Loại bỏ các hàng có giá trị NaT (dữ liệu thời gian không hợp lệ)
logs.dropna(subset=['timestamp'], inplace=True)
command_status.dropna(subset=['updatedAt'], inplace=True)
rain_status.dropna(subset=['updatedAt'], inplace=True)

# Đếm số lượng trạng thái (logs) theo từng loại để vẽ cột
log_counts = logs['message'].value_counts()
command_counts = command_status['command'].value_counts()
rain_counts = rain_status['rainStatus'].value_counts()

# Vẽ đồ thị dạng cột cho Logs
plt.figure(figsize=(10, 6))
plt.bar(log_counts.index, log_counts.values, color='skyblue')
plt.title("Logs hoạt động của hệ thống")
plt.xlabel("Logs")
plt.ylabel("Số lượng")
plt.xticks(rotation=45)
plt.tight_layout()
plt.show()

# Vẽ đồ thị dạng cột cho CommandStatus
plt.figure(figsize=(10, 6))
plt.bar(command_counts.index, command_counts.values, color='orange')
plt.title("Lệnh điều khiển")
plt.xlabel("Lệnh")
plt.ylabel("Số lượng")
plt.xticks(rotation=45)
plt.tight_layout()
plt.show()

# Vẽ đồ thị dạng cột cho RainStatus
plt.figure(figsize=(10, 6))
plt.bar(rain_counts.index, rain_counts.values, color='lightblue')
plt.title("Trạng thái mưa")
plt.xlabel("Trạng thái")
plt.ylabel("Số lượng")
plt.xticks(rotation=45)
plt.tight_layout()
plt.show()
