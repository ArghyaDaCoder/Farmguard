# Farmguard
Precision Agriculture system using AI+IoT

________________________________________________________________________________________________________________________________________________________________

🌱 FarmGuard

FarmGuard is an IoT + Computer Vision–based plant disease monitoring system that uses an ESP32-CAM to capture leaf images, a Flask backend for inference, and Roboflow-hosted models for detection and classification. Alerts are sent in real time via Telegram.

The system is designed for precision agriculture use-cases, with a focus on modularity, deployability, and clear separation of code and credentials.

🔧 System Architecture (High Level)

> ESP32-CAM
Captures images on button press and sends them to the backend over HTTPS.

> Flask Backend
Receives images, performs:

> Leaf detection

> Disease classification

> Logging and visualization

> Roboflow Models
Used for leaf detection and disease classification.

> Telegram Bot (Optional)
Sends alerts when a disease is detected.
