#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MS5837.h"
#include <MPU6050.h>
#include "esp_sleep.h"

// ===== WiFi =====
const char* ssid = "DIVE_SYSTEM";
const char* password = "12345678";

WebServer server(80);

// ===== Pins =====
#define BUZZER 5
#define TOUCH_PIN 4
#define BATTERY_PIN 35

// ===== Sensors =====
MS5837 depthSensor;
MPU6050 mpu;

// ===== Variables =====
float depth = 0;
float smoothDepth = 0;
float baseDepth = 0;
float smoothTilt = 0;
float battery = 0;

unsigned long touchStart = 0;
bool touching = false;

// ===== Buzzer =====
void buzzerTone(int freq){
  if(freq == 0){
    ledcWrite(BUZZER, 0);
  } else {
    ledcWriteTone(BUZZER, freq);
  }
}

// ===== Battery =====
float readBattery(){
  int raw = analogRead(BATTERY_PIN);
  float voltage = raw * (3.3 / 4095.0) * 2.0;
  float percent = (voltage - 3.3) * 100 / (4.2 - 3.3);
  return constrain(percent, 0, 100);
}

// ===== HTML =====
String htmlPage(){
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { background:#0f2027; color:white; text-align:center; font-family:Arial; }
.card { background:#1c2b36; margin:10px; padding:20px; border-radius:15px; }
.val { font-size:40px; }
.alert { color:red; font-size:25px; }
</style>
</head>
<body>

<h2>🌊 Smart Diver Protection System</h2>

<div class="card">
Depth
<div class="val" id="depth">0</div> cm
</div>

<div class="card">
Tilt
<div class="val" id="tilt">0</div>
</div>

<div class="card">
Battery
<div class="val" id="bat">0</div> %
</div>

<div id="alert"></div>

<script>
setInterval(()=>{
 fetch("/data")
  .then(r=>r.json())
  .then(d=>{
    document.getElementById("depth").innerText = d.depth.toFixed(1);
    document.getElementById("tilt").innerText = d.tilt.toFixed(1);
    document.getElementById("bat").innerText = d.battery.toFixed(0);

    if(d.depth >= 30){
      document.getElementById("alert").innerHTML = "⚠️ DANGER";
    } else {
      document.getElementById("alert").innerHTML = "";
    }
  })
  .catch(e=>console.log(e));
},1000);
</script>

</body>
</html>
)rawliteral";
}

// ===== API =====
void handleData(){
  String json = "{";
  json += "\"depth\":" + String(smoothDepth) + ",";
  json += "\"tilt\":" + String(smoothTilt) + ",";
  json += "\"battery\":" + String(battery);
  json += "}";
  server.send(200, "application/json", json);
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Wire.begin(21,22);

  depthSensor.init();
  depthSensor.setModel(MS5837::MS5837_02BA);
  depthSensor.setFluidDensity(997);

  mpu.initialize();

  ledcAttach(BUZZER, 2000, 8);
  pinMode(TOUCH_PIN, INPUT);

  delay(2000);
  depthSensor.read();
  baseDepth = depthSensor.depth()*100;

  WiFi.softAP(ssid,password);
  Serial.println("IP:");
  Serial.println(WiFi.softAPIP());

  // 🔥 مهم جدًا (هذا كان سبب المشكلة)
  server.on("/", [](){ server.send(200,"text/html",htmlPage()); });
  server.on("/data", handleData);

  server.begin();
}

// ===== Loop =====
void loop() {

  server.handleClient();

  // ===== Depth =====
  depthSensor.read();
  depth = (depthSensor.depth()*100) - baseDepth;
  if(depth < 0) depth = 0;
  smoothDepth = (0.1*depth)+(0.9*smoothDepth);

  // ===== Tilt =====
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax,&ay,&az);
  float rawTilt = atan2(ax,az)*57.3;
  smoothTilt = (0.1*rawTilt)+(0.9*smoothTilt);

  // ===== Battery =====
  battery = readBattery();

  // ===== Touch =====
  int touch = digitalRead(TOUCH_PIN);

  if(touch == HIGH){

    if(!touching){
      touching = true;
      touchStart = millis();
    }

    buzzerTone(1000);

    if(millis() - touchStart >= 60000){
      esp_deep_sleep_start();
    }

  } else {
    touching = false;

    if(smoothDepth >= 30){
      buzzerTone(2000);
    } else {
      buzzerTone(0);
    }
  }

  delay(200);
}