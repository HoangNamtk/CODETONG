
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ========== WIFI ==========
const char* ssid = "ESP32_ROBOT";
const char* password = "12345678";

WebServer server(80);

// ========== L298N ==========
#define IN1 13
#define IN2 12
#define ENA 14

#define IN3 27
#define IN4 26
#define ENB 25

int speedPWM = 180;

// ========== PCA9685 ==========
#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PWMServoDriver pwm =
  Adafruit_PWMServoDriver(0x40);

// Kenh servo theo code cua ban
#define SERVO_TRAI_TREN    0
#define SERVO_PHAI_TREN    1
#define SERVO_TRAI_DUOI    2
#define SERVO_PHAI_DUOI    3
#define SERVO_TRAI_TREN1   4
#define SERVO_PHAI_TREN2   5
#define SERVO_TRAI_DUOI3   6
#define SERVO_PHAI_DUOI4   7

#define SERVOMIN 150
#define SERVOMAX 600

int servoAngles[8] = {
  90, 90, 90, 90, 90, 90, 90, 90
};

// ========== DIEU KHIEN SERVO ==========
int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void setServoAngle(uint8_t channel, int angle) {
  if (channel > 7) return;

  angle = constrain(angle, 0, 180);
  servoAngles[channel] = angle;

  pwm.setPWM(channel, 0, angleToPulse(angle));
}

// ========== DIEU KHIEN DONG CO ==========
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  ledcWrite(ENA, speedPWM);
  ledcWrite(ENB, speedPWM);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  ledcWrite(ENA, speedPWM);
  ledcWrite(ENB, speedPWM);
}

void turnLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  ledcWrite(ENA, speedPWM);
  ledcWrite(ENB, speedPWM);
}

void turnRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  ledcWrite(ENA, speedPWM);
  ledcWrite(ENB, speedPWM);
  
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

// ========== GIAO DIEN WEB ==========
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport"
 content="width=device-width, initial-scale=1">
<title>Robot ESP32</title>

<style>
* { box-sizing: border-box; }

body {
  margin: 0;
  padding: 16px;
  font-family: Arial, sans-serif;
  background: #101827;
  color: white;
}

main {
  max-width: 800px;
  margin: auto;
}

h1, h2 { color: #38bdf8; }

.panel {
  background: #1e293b;
  padding: 16px;
  border-radius: 14px;
  margin: 16px 0;
}

.pad {
  max-width: 340px;
  margin: auto;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 10px;
}

button {
  border: 0;
  border-radius: 12px;
  padding: 15px 5px;
  background: #2563eb;
  color: white;
  font-size: 17px;
  font-weight: bold;
  cursor: pointer;
  touch-action: none;
  user-select: none;
}

button:active { background: #0284c7; }

.stop { background: #dc2626; }
.empty { visibility: hidden; }

input[type=range] { width: 100%; }

.servo-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.servo {
  background: #263449;
  padding: 12px;
  border-radius: 10px;
}

.servo label {
  display: block;
  margin-bottom: 12px;
  font-size: 14px;
}

.angle {
  color: #7dd3fc;
  font-weight: bold;
}

button.small {
  padding: 8px;
  font-size: 13px;
  margin-top: 8px;
}

button.secondary { background: #475569; }

.actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 14px;
}

#status { color: #86efac; }

@media (max-width: 420px) {
  body { padding: 9px; }
  .servo-grid { grid-template-columns: 1fr; }
}
</style>
</head>

<body>
<main>
<h1>ROBOT ESP32</h1>
<p>Điều khiển động cơ và 8 servo</p>
<p>Trạng thái: <span id="status">Sẵn sàng</span></p>

<section class="panel">
<h2>1. Điều khiển di chuyển</h2>

<div class="pad">
  <div class="empty"></div>
  <button data-move="F">▲ TIẾN</button>
  <div class="empty"></div>

  <button data-move="L">◀ TRÁI</button>
  <button class="stop" id="stop">■ DỪNG</button>
  <button data-move="R">PHẢI ▶</button>

  <div class="empty"></div>
  <button data-move="B">▼ LÙI</button>
  <div class="empty"></div>
</div>

<h3>Tốc độ PWM: <span id="speedValue">180</span></h3>
<input id="speed" type="range"
 min="0" max="255" value="180">
</section>

<section class="panel">
<h2>2. Điều khiển 8 servo</h2>
<p>Chọn góc riêng cho từng servo.</p>

<div class="actions">
  <button class="secondary" id="all90">
    Tất cả về 90°
  </button>
  <button class="secondary" id="all0">
    Tất cả về 0°
  </button>
  <button class="secondary" id="all180">
    Tất cả về 180°
  </button>
</div>

<div id="servos" class="servo-grid"></div>
</section>

</main>

<script>
const statusEl = document.getElementById("status");

async function request(url) {
  try {
    const r = await fetch(url);
    if (!r.ok) throw new Error("HTTP");
    statusEl.textContent = "Đã kết nối";
    return true;
  } catch (e) {
    statusEl.textContent = "Lỗi kết nối";
    return false;
  }
}

// ----- Dong co -----
function move(c) {
  request("/cmd?c=" + c);
}

document.querySelectorAll("[data-move]").forEach(btn => {
  btn.addEventListener("pointerdown", e => {
    e.preventDefault();
    btn.setPointerCapture(e.pointerId);
    move(btn.dataset.move);
  });

  btn.addEventListener("pointerup", e => {
    e.preventDefault();
    move("S");
  });

  btn.addEventListener("pointercancel", () => move("S"));
  btn.addEventListener("lostpointercapture", () => move("S"));
});

document.getElementById("stop").addEventListener("click", () => {
  move("S");
});

const speed = document.getElementById("speed");

speed.addEventListener("input", () => {
  document.getElementById("speedValue").textContent = speed.value;
  request("/speed?v=" + speed.value);
});

// ----- Tao 8 bo dieu khien servo -----
const names = [
  "Servo trái trên",
  "Servo phải trên",
  "Servo trái dưới",
  "Servo phải dưới",
  "Servo trái trên 2",
  "Servo phải trên 2",
  "Servo trái dưới 2",
  "Servo phải dưới 2"
];

const servoBox = document.getElementById("servos");

names.forEach((name, ch) => {
  const card = document.createElement("div");
  card.className = "servo";

  card.innerHTML = `
    <label>${name} — Kênh ${ch}</label>
    <input type="range" min="0" max="180" value="90"
      id="angle${ch}">
    <div>Góc: <span class="angle" id="value${ch}">90°</span></div>
    <button class="small" id="apply${ch}">Đặt góc</button>
    <button class="small secondary" id="home${ch}">Về 90°</button>
  `;

  servoBox.appendChild(card);

  const slider = card.querySelector(`#angle${ch}`);
  const label = card.querySelector(`#value${ch}`);

  slider.addEventListener("input", () => {
    label.textContent = slider.value + "°";
  });

  card.querySelector(`#apply${ch}`).addEventListener("click", () => {
    request(`/servo?ch=${ch}&angle=${slider.value}`);
  });

  card.querySelector(`#home${ch}`).addEventListener("click", () => {
    slider.value = 90;
    label.textContent = "90°";
    request(`/servo?ch=${ch}&angle=90`);
  });
});

// Dat tat ca servo den goc mong muon
async function setAll(angle) {
  for (let ch = 0; ch < 8; ch++) {
    const slider = document.getElementById("angle" + ch);
    slider.value = angle;
    document.getElementById("value" + ch).textContent = angle + "°";
  }

  // Gui lan luot de tranh gui qua nhieu yeu cau dong thoi
  for (let ch = 0; ch < 8; ch++) {
    const ok = await request(`/servo?ch=${ch}&angle=${angle}`);
    if (!ok) break;
  }
}

document.getElementById("all90").onclick = () => setAll(90);
document.getElementById("all0").onclick = () => setAll(0);
document.getElementById("all180").onclick = () => setAll(180);

// Khi roi trang, thu gui lenh dung
window.addEventListener("pagehide", () => {
  navigator.sendBeacon("/cmd?c=S");
});
</script>
</body>
</html>
)rawliteral";

// ========== XU LY LENH WEB ==========
void handleCommand() {
  String c = server.arg("c");
  c.toUpperCase();

  if (c == "F") {
    forward();
  }
  else if (c == "B") {
    backward();
  }
  else if (c == "L") {
    turnLeft();
  }
  else if (c == "R") {
    turnRight();
  }
  else {
    stopCar();
  }

  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("v")) {
    speedPWM = constrain(server.arg("v").toInt(), 0, 255);
  }

  server.send(200, "text/plain", String(speedPWM));
}

void handleServo() {
  if (!server.hasArg("ch") || !server.hasArg("angle")) {
    server.send(400, "text/plain", "Missing parameter");
    return;
  }

  int ch = server.arg("ch").toInt();
  int angle = server.arg("angle").toInt();

  if (ch < 0 || ch > 7 || angle < 0 || angle > 180) {
    server.send(400, "text/plain", "Invalid value");
    return;
  }

  setServoAngle(ch, angle);

  server.send(200, "text/plain", "OK");
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  // L298N
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);
  stopCar();

  // PCA9685
  Wire.begin(SDA_PIN, SCL_PIN);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);

  // Dat ca 8 servo ve 90 do
  for (uint8_t ch = 0; ch < 8; ch++) {
    setServoAngle(ch, 90);
    delay(30);
  }

  // Wi-Fi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("ROBOT ESP32 READY");
  Serial.print("Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("Web: http://");
  Serial.println(WiFi.softAPIP());

  // Cac dia chi web
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/cmd", HTTP_GET, handleCommand);
  server.on("/speed", HTTP_GET, handleSpeed);
  server.on("/servo", HTTP_GET, handleServo);

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Web server started");
}

// ========== LOOP ==========
void loop() {
  server.handleClient();

  // Van giu dieu khien dong co qua Serial
  if (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') return;

    switch (c) {
      case 'F':
      case 'f': forward(); break;

      case 'B':
      case 'b': backward(); break;

      case 'L':
      case 'l': turnLeft(); break;

      case 'R':
      case 'r': turnRight(); break;

      case 'S':
      case 's': stopCar(); break;
    }
  }
}
