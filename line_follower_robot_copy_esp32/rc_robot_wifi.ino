#include <WiFi.h>
#include <WebServer.h>

// Motor pins
int enableRightMotor = 16;
int rightMotorPin1 = 17;
int rightMotorPin2 = 18;
int enableLeftMotor = 19;
int leftMotorPin1 = 21;
int leftMotorPin2 = 22;

// PWM properties
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// WiFi credentials
const char* ssid = "ESP32_RC_ROBOT";
const char* password = "12345678";

// Default motor speed
int motorSpeed = 200;

WebServer server(80);

void rotateMotor(int rightMotorSpeed, int leftMotorSpeed)
{
  if (rightMotorSpeed < 0)
  {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, HIGH);
  }
  else if (rightMotorSpeed > 0)
  {
    digitalWrite(rightMotorPin1, HIGH);
    digitalWrite(rightMotorPin2, LOW);
  }
  else
  {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, LOW);
  }

  if (leftMotorSpeed < 0)
  {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, HIGH);
  }
  else if (leftMotorSpeed > 0)
  {
    digitalWrite(leftMotorPin1, HIGH);
    digitalWrite(leftMotorPin2, LOW);
  }
  else
  {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, LOW);
  }

  ledcWrite(enableRightMotor, abs(rightMotorSpeed));
  ledcWrite(enableLeftMotor, abs(leftMotorSpeed));
}

void handleRoot()
{
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 RC Robot</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: Arial, sans-serif;
      background: #1a1a2e;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      color: white;
    }
    .container {
      text-align: center;
      max-width: 400px;
      padding: 20px;
    }
    h1 { font-size: 24px; margin-bottom: 20px; color: #e94560; }
    .dpad {
      display: grid;
      grid-template-areas:
        ". up ."
        "left stop right"
        ". down .";
      grid-template-columns: 80px 80px 80px;
      gap: 8px;
      justify-content: center;
      margin: 20px 0;
    }
    .btn {
      width: 80px;
      height: 80px;
      font-size: 28px;
      border: none;
      border-radius: 12px;
      cursor: pointer;
      background: #16213e;
      color: white;
      touch-action: manipulation;
      user-select: none;
      transition: background 0.1s;
    }
    .btn:active, .btn.active {
      background: #e94560;
    }
    .btn-up    { grid-area: up; }
    .btn-down  { grid-area: down; }
    .btn-left  { grid-area: left; }
    .btn-right { grid-area: right; }
    .btn-stop  { grid-area: stop; background: #e94560; border-radius: 50%; }
    .speed-control {
      margin: 20px 0;
    }
    .speed-control input {
      width: 100%;
    }
    .speed-label {
      margin-bottom: 8px;
      font-size: 14px;
      color: #ccc;
    }
    .status {
      margin-top: 16px;
      padding: 10px;
      border-radius: 8px;
      background: #16213e;
      font-size: 14px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 RC Robot</h1>
    <div class="speed-control">
      <div class="speed-label">Kecepatan: <span id="speedValue">200</span></div>
      <input type="range" id="speedSlider" min="0" max="255" value="200" oninput="updateSpeed(this.value)">
    </div>
    <div class="dpad">
      <button class="btn btn-up"    ontouchstart="move('forward')"  ontouchend="move('stop')" onmousedown="move('forward')" onmouseup="move('stop')">&#9650;</button>
      <button class="btn btn-left"  ontouchstart="move('left')"    ontouchend="move('stop')" onmousedown="move('left')" onmouseup="move('stop')">&#9664;</button>
      <button class="btn btn-stop"  onclick="move('stop')">&#9632;</button>
      <button class="btn btn-right" ontouchstart="move('right')"   ontouchend="move('stop')" onmousedown="move('right')" onmouseup="move('stop')">&#9654;</button>
      <button class="btn btn-down"  ontouchstart="move('backward')" ontouchend="move('stop')" onmousedown="move('backward')" onmouseup="move('stop')">&#9660;</button>
    </div>
    <div class="status">
      <div>IP: <span id="ip">)rawliteral";
  html += WiFi.softAPIP().toString();
  html += R"rawliteral(</span></div>
      <div>Status: <span id="statusText">Berhenti</span></div>
    </div>
  </div>
  <script>
    let currentCmd = 'stop';
    let speed = 200;

    function updateSpeed(val) {
      speed = val;
      document.getElementById('speedValue').textContent = val;
      fetch('/speed?val=' + val);
    }

    function move(cmd) {
      if (cmd === currentCmd) return;
      currentCmd = cmd;
      fetch('/' + cmd + '?speed=' + speed);
      document.getElementById('statusText').textContent = cmd;
    }

    document.addEventListener('keydown', function(e) {
      const keyMap = {
        'ArrowUp': 'forward',
        'ArrowDown': 'backward',
        'ArrowLeft': 'left',
        'ArrowRight': 'right',
        ' ': 'stop'
      };
      if (keyMap[e.key]) {
        e.preventDefault();
        move(keyMap[e.key]);
      }
    });

    document.addEventListener('keyup', function(e) {
      const dirs = ['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'];
      if (dirs.includes(e.key)) {
        e.preventDefault();
        move('stop');
      }
    });
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleCommand()
{
  String cmd = server.uri();

  if (cmd == "/speed")
  {
    if (server.hasArg("val"))
    {
      motorSpeed = constrain(server.arg("val").toInt(), 0, 255);
    }
    server.send(200, "text/plain", "OK");
    return;
  }

  int speed = motorSpeed;

  if (server.hasArg("speed"))
  {
    speed = constrain(server.arg("speed").toInt(), 0, 255);
  }

  if (cmd == "/forward")
  {
    rotateMotor(speed, speed);
  }
  else if (cmd == "/backward")
  {
    rotateMotor(-speed, -speed);
  }
  else if (cmd == "/left")
  {
    rotateMotor(-speed, speed);
  }
  else if (cmd == "/right")
  {
    rotateMotor(speed, -speed);
  }
  else if (cmd == "/stop")
  {
    rotateMotor(0, 0);
  }

  server.send(200, "text/plain", "OK");
}

void setup()
{
  Serial.begin(115200);

  pinMode(enableRightMotor, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(enableLeftMotor, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);

  ledcAttach(enableRightMotor, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(enableLeftMotor, PWM_FREQ, PWM_RESOLUTION);

  rotateMotor(0, 0);

  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/forward", handleCommand);
  server.on("/backward", handleCommand);
  server.on("/left", handleCommand);
  server.on("/right", handleCommand);
  server.on("/stop", handleCommand);
  server.on("/speed", handleCommand);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}
