#include <smartfarm2_inferencing.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h> 

/* --- PIN CONFIG (S3 WROOM) --- */
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5
#define Y9_GPIO_NUM    16
#define Y8_GPIO_NUM    17
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    12
#define Y5_GPIO_NUM    10
#define Y4_GPIO_NUM     8
#define Y3_GPIO_NUM     9
#define Y2_GPIO_NUM    11
#define VSYNC_GPIO_NUM  6
#define HREF_GPIO_NUM   7
#define PCLK_GPIO_NUM  13

#define WATER_PUMP 2
#define NUTRIENT_PUMP 42
#define SOIL_SENSOR 1

/* --- NETWORK SETTINGS --- */
IPAddress local_IP(192, 168, 0, 184); 
IPAddress gateway(192, 168, 0, 1);    // UPDATED TO YOUR ROUTER
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);
String aiResult = "Waiting for data...";
int soilValue = 0;
bool autoPilot = true; // Automation Toggle
uint8_t *snapshot_buf;

/* --- WEB PORTAL --- */
const char* htmlPage = R"rawtext(
<!DOCTYPE html><html><head><title>AI Smart Farm</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { font-family: sans-serif; text-align: center; background: #222; color: #fff; }
  .box { background: #333; padding: 15px; margin: 10px auto; max-width: 450px; border-radius: 10px; }
  .btn { padding: 10px 20px; font-size: 16px; margin: 5px; cursor: pointer; border: none; border-radius: 5px; }
  .on { background: #2ecc71; color: white; } .off { background: #e74c3c; color: white; }
  #stream { width: 100%; border-radius: 5px; }
  span { font-weight: bold; color: #f1c40f; }
</style>
</head>
<body>
  <h2>Smart Farm AI Portal</h2>
  <div class="box"><img src="/stream" id="stream"></div>
  <div class="box">
    <p>Plant Health: <span id="ai">Detecting...</span></p>
    <p>Soil Moisture: <span id="soil">0</span></p>
    <hr>
    <button class="btn on" onclick="fetch('/toggle?d=water')">Water Pump</button>
    <button class="btn on" onclick="fetch('/toggle?d=nutri')">Nutrient Pump</button>
    <button class="btn off" onclick="fetch('/toggle?d=auto')">Toggle Auto-Pilot</button>
  </div>
  <script>
    setInterval(() => {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('ai').innerText = d.ai;
        document.getElementById('soil').innerText = d.soil;
      });
    }, 2000);
  </script>
</body></html>)rawtext";

/* --- HANDLERS --- */
void handleStream() {
    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n");
    while (client.connected()) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) break;
        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(WATER_PUMP, OUTPUT);
    pinMode(NUTRIENT_PUMP, OUTPUT);

    // 1. WiFi Manager + Static IP
    WiFiManager wm;
    wm.setSTAStaticIPConfig(local_IP, gateway, subnet);
    if (!wm.autoConnect("SmartFarm-Setup")) ESP.restart();

    // 2. Camera Setup
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000; config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA; config.jpeg_quality = 12;
    config.fb_count = 1; config.fb_location = CAMERA_FB_IN_PSRAM;
    esp_camera_init(&config);

    // 3. Web Server
    server.on("/", [](){ server.send(200, "text/html", htmlPage); });
    server.on("/stream", handleStream);
    server.on("/status", [](){
        String json = "{\"ai\":\"" + aiResult + "\", \"soil\":" + String(soilValue) + "}";
        server.send(200, "application/json", json);
    });
    server.on("/toggle", [](){
        String d = server.arg("d");
        if(d == "water") digitalWrite(WATER_PUMP, !digitalRead(WATER_PUMP));
        if(d == "nutri") digitalWrite(NUTRIENT_PUMP, !digitalRead(NUTRIENT_PUMP));
        if(d == "auto") autoPilot = !autoPilot;
        server.send(200);
    });
    server.begin();
}

void loop() {
    server.handleClient();
    
    // Non-blocking Sensor/AI logic
    static unsigned long lastRun = 0;
    if (millis() - lastRun > 3000) {
        soilValue = analogRead(SOIL_SENSOR);
        runInference();
        
        if (autoPilot) {
            if (soilValue < 1200) digitalWrite(WATER_PUMP, HIGH);
            else digitalWrite(WATER_PUMP, LOW);
            
            if (aiResult.indexOf("Low") != -1) digitalWrite(NUTRIENT_PUMP, HIGH);
            else digitalWrite(NUTRIENT_PUMP, LOW);
        }
        lastRun = millis();
    }
}

void runInference() {
    snapshot_buf = (uint8_t*)malloc(320 * 240 * 3);
    if (!snapshot_buf) return;
    
    // ... [Same Edge Impulse capture and classification logic from earlier] ...
    // Update aiResult variable with result.classification[i].label
    free(snapshot_buf);
}