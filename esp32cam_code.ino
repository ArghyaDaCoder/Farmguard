#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// —— CONFIG ——
const char* ssid = " ";
const char* password = " ";

// Replace with your live Render URL + /upload endpoint
const char* serverURL = "https://plant-detector-v82l.onrender.com/upload?cam=Cam1"; //Change Cam1 to desired name of camera / region

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"


// ⭐️ NEW: GPIO pin definitions
#define BUTTON_PIN 12
#define GREEN_LED 2
#define BLUE_LED 14

void sendToFlaskServer(camera_fb_t *fb) {
  if (!fb || !fb->buf) {
    Serial.println("❌ No frame to send!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Don't verify SSL certs

  HTTPClient http;
  http.begin(client, serverURL);
  http.addHeader("Content-Type", "image/jpeg");

  Serial.printf("📤 Sending image (%d bytes) to: %s\n", fb->len, serverURL);
  int res = http.POST(fb->buf, fb->len);

  Serial.print("📬 HTTP Response Code: ");
  Serial.println(res);

  if (res > 0) {
    String payload = http.getString();
    Serial.println("📩 Server Response:");
    Serial.println(payload);
  } else {
    Serial.println("🔥 Failed to send image!");
    Serial.println(http.errorToString(res));
  }

  http.end();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("🔌 Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(GREEN_LED, HIGH);  // ⭐️ NEW
  } else {
    Serial.println("\n❌ Failed to connect to WiFi.");
    digitalWrite(GREEN_LED, LOW);   // ⭐️ NEW
  }
}

void setup() {
    pinMode(33, OUTPUT);   // Try 33 first
  digitalWrite(33, LOW); // LOW = ON for onboard LED (active low)
    pinMode(4, OUTPUT);   // Try 33 first
  digitalWrite(4, HIGH);
  digitalWrite(4, LOW);
  digitalWrite(4, HIGH);
  digitalWrite(4, LOW                                                         ); // LOW = ON for onboard LED (active low)

  Serial.begin(115200);
  delay(1000);

  // ⭐️ NEW: Setup LEDs and button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(4, OUTPUT); // Flash already used
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  connectToWiFi();

  // DON'T TOUCH CAMERA INIT – PSRAM cries if messed with 😭
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.jpeg_quality = 15;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("❌ Camera init failed!");
    while (true) delay(1000);
  }

  sensor_t *s = esp_camera_sensor_get();  
  s->set_brightness(s, 2);     
  s->set_contrast(s, 2);       
  s->set_saturation(s, 2);     
  s->set_gainceiling(s, (gainceiling_t)6);  
  s->set_awb_gain(s, 1);       
  s->set_exposure_ctrl(s, 1);  
  s->set_aec2(s, 1);           
  s->set_gain_ctrl(s, 1);      
  s->set_bpc(s, 1);            
  s->set_wpc(s, 1);            
  s->set_whitebal(s, 1);     
  s->set_wb_mode(s, 0);      

  Serial.printf("📦 Free heap after init: %u bytes\n", ESP.getFreeHeap());
}

// Add these two variables right above your loop() function
unsigned long lastCaptureTime = 0;
const unsigned long captureInterval = 1200000; // 20 minutes in milliseconds

void loop() {
  // 1. Check if WiFi is still connected
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(GREEN_LED, LOW); // Turn off green LED if WiFi disconnected
  } else {
    digitalWrite(GREEN_LED, HIGH); // Ensure green LED is ON if WiFi connected
  }

  // 2. Check conditions: Was the button pressed? OR has 20 minutes passed?
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  bool timerTriggered = (millis() - lastCaptureTime >= captureInterval);

  if (WiFi.status() == WL_CONNECTED && (buttonPressed || timerTriggered)) {
    
    if (buttonPressed) {
      delay(50); // Debounce the button
      Serial.println("📸 Button pressed! Capturing...");
    } else {
      Serial.println("⏱️ 20 minutes elapsed! Auto-capturing...");
    }

    // --- CAPTURE SEQUENCE ---
    digitalWrite(BLUE_LED, HIGH); // Turn on active LED
    digitalWrite(4, HIGH);        // Flash ON
    
    esp_camera_fb_return(esp_camera_fb_get());  // 🧼 Flush the old frame
    delay(100);  // Small delay to ensure next frame loads fresh
    camera_fb_t *fb = esp_camera_fb_get();  // Now capture

    if (!fb || !fb->buf) {
      Serial.println("❌ Capture failed");
    } else {
      Serial.println("✅ Image captured! Sending...");
      sendToFlaskServer(fb);
      esp_camera_fb_return(fb);
      Serial.printf("📦 Free heap after sending: %u bytes\n", ESP.getFreeHeap());
    }

    digitalWrite(BLUE_LED, LOW);  // Turn off active LED
    digitalWrite(4, LOW);         // Flash OFF
    
    // 3. ⭐️ RESET THE TIMER ⭐️
    // Whether triggered by button or timer, reset the stopwatch to 0
    lastCaptureTime = millis(); 
    
    delay(1000); // 1 second cooldown
  }
}
