# esp32-location-intelligence
IoT spatial intelligence project using WROOM ESP32 and NEO-6M GPS. Logs spatio-temporal data offline to an SD card and processes stopping points using Python &amp; DBSCAN.
# 📍 ESP32 GPS Spatial Clustering & Route Discovery

An end-to-end Data Science and IoT project that collects real-world spatio-temporal data (Latitude, Longitude, Speed, Timestamp) using an **ESP32** and **NEO-6M GPS module**, logging coordinates to an SD card for offline spatial analysis.

## 🚀 Features
- **Offline Data Logging:** Stores GPS coordinates in CSV format on a MicroSD card module.
- **Stop-Point Filtering:** Filters out noise and moving points using speed thresholds.
- **Unsupervised Spatial Clustering:** Applies **DBSCAN** with the Haversine metric to group dense coordinate fixes into semantic places (e.g., Home, University, Frequent Spots).
- **Interactive Map Visualization:** Generates standalone HTML maps via **Folium** for visual trajectory inspection.

## 🛠️ Tech Stack
- **Hardware:** ESP32 (WROOM / XIAO S3), NEO-6M GPS Module, MicroSD Card Module
- **Firmware:** C++ / Arduino IDE (TinyGPS++, SPI, SD)
- **Data Science:** Python, Pandas, NumPy, Scikit-Learn (DBSCAN), Folium
