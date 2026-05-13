#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ESP32Servo.h>

#define DHTPIN 15
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// Pins
int led1=18, led2=4;
int servoPin1=13, servoPin2=5;
int IN1=16, IN2=17, IN3=14, IN4=12;
int ENA=25, ENB=26;
int rainPin=32;
int waterPin=35;
int moisturePin=34;

Servo servo1, servo2;
int speedVal=255;

// WiFi
const char* ssid="ESP32-DASH";
const char* password="12345678";

// PWM
void setupPWM(){
  ledcAttach(ENA,1000,8);
  ledcAttach(ENB,1000,8);

  ledcWrite(ENA, speedVal);
  ledcWrite(ENB, speedVal);
}

// Motor
void stopMotor(){
digitalWrite(IN1,0); digitalWrite(IN2,0);
digitalWrite(IN3,0); digitalWrite(IN4,0);
}
void forward(){
digitalWrite(IN1,1); digitalWrite(IN2,0);
digitalWrite(IN3,1); digitalWrite(IN4,0);
}
void reverse(){
digitalWrite(IN1,0); digitalWrite(IN2,1);
digitalWrite(IN3,0); digitalWrite(IN4,1);
}

void left(){
digitalWrite(IN1,1); digitalWrite(IN2,0);
digitalWrite(IN3,0); digitalWrite(IN4,1);
}

void right(){
digitalWrite(IN1,0); digitalWrite(IN2,1);
digitalWrite(IN3,1); digitalWrite(IN4,0);
}

// Sensor JSON
String getData(){
float t=dht.readTemperature();
float h=dht.readHumidity();
if(isnan(t) || isnan(h)){
  t = 0;
  h = 0;
}

int sum=0;
for(int i=0;i<5;i++){
  sum += analogRead(moisturePin);
}
int raw = sum/5;


// calibration (important)
int dry = 4095;   // dry soil value
int wet = 1800;   // wet soil value (measure kore adjust korte paro)

int moisturePercent = map(raw, dry, wet, 0, 100);
moisturePercent = constrain(moisturePercent, 0, 100);

// ===== WATER LEVEL (PERCENTAGE) =====
int wRaw = analogRead(waterPin);

// calibration (your measured values)
int waterMin = 500;   // lowest
int waterMax = 2100;  // highest

int w = map(wRaw, waterMin, waterMax, 0, 100);
w = constrain(w, 0, 100);

// ===== RAIN SENSOR =====
int rainAnalog = analogRead(rainPin);

String rain;
if(rainAnalog < 2000){
  rain = "Raining";
}else{
  rain = "No Rain";
}
// ===== SERIAL OUTPUT =====

// DHT

Serial.print("Temp: ");
Serial.print(t);
Serial.print(" °C | Humidity: ");
Serial.println(h);

// Rain
Serial.print("Rain Value: ");
Serial.print(rainAnalog);
Serial.print(" → ");
Serial.println(rain);

// Water %
Serial.print("Water Level: ");
Serial.print(w);
Serial.println(" %");

// Moisture
Serial.print("Moisture Value: ");
Serial.print(raw);
Serial.print(" → ");

if(raw < 3000){
  Serial.println("Soil Wet");
}else{
  Serial.println("Soil Dry");
}

// gap
Serial.println("------------------");

return "{\"temp\":"+String(t)+",\"hum\":"+String(h)+",\"moist\":"+String(moisturePercent)+",\"water\":"+String(w)+",\"rain\":\""+rain+"\"}";
}

// HTML
String page = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>

body{
background:#0b3d2e;
color:white;
font-family:sans-serif;
text-align:center;
}

.card{
background:rgba(20,83,45,0.85);
backdrop-filter:blur(10px);
padding:20px;
margin:15px;
border-radius:18px;
box-shadow:0 8px 25px rgba(0,0,0,0.5);
border:1px solid rgba(255,255,255,0.08);
}

.card p{
text-align:left;
margin-left:10px;
}

/* LED SWITCH */
.switch{
position:relative;
display:inline-block;
width:80px;
height:40px;
margin:10px;
}
.switch input{display:none;}

.slider{
position:absolute;
cursor:pointer;
background:#ffffff;
border-radius:999px;   /* 👈 perfect pill */
top:0;
left:0;
width:80px;
height:40px;
transition:0.4s;
}

.slider:before{
content:"";
position:absolute;
height:30px;
width:30px;
left:5px;
top:50%;
transform:translateY(-50%);  
background:#22c55e;
border-radius:50%;
transition:0.4s;
}

input:checked + .slider{
background:#22c55e;
}
input:checked + .slider:before{
background:white;
transform:translate(40px, -50%);   /* 👈 center + move */
}

/* MOVEMENT PAD */
.pad{
position:relative;
width:220px;
height:220px;
margin:auto;
}
.btn{
position:absolute;
width:70px;
height:70px;
border-radius:50%;
border:none;
font-size:22px;
color:white;
box-shadow:0 6px 12px rgba(0,0,0,0.5);
transition:all 0.15s ease;
}
/* CLICK EFFECT */
.btn:active{
transform:scale(0.9);
box-shadow:0 2px 4px rgba(0,0,0,0.6);
filter:brightness(1.3);
}

.btn.active{
transform:scale(0.9);
box-shadow:0 2px 4px rgba(0,0,0,0.6);
filter:brightness(1.3);
}

.up{ top:0; left:75px; background:#22c55e;}
.down{ bottom:0; left:75px; background:#3b82f6;}
.left{ left:0; top:75px; background:#ef4444;}
.right{ right:0; top:75px; background:#f59e0b;}

/* SLIDER BG color */
input[type=range]{
width:90%;
margin:15px;
appearance:none;
height:20px;
border-radius:10px;
outline:none;

/* 👇 dynamic fill */
background:linear-gradient(to right, #22c55e 0%, #22c55e 0%, #ffffff 0%, #ffffff 100%);
}

input[type=range]::-webkit-slider-thumb{
appearance:none;
width:30px;
height:30px;
background:#065f46;    /* Slider Circle color */
border-radius:50%;
cursor:pointer;
}


/* WATER LEVEL */
.water-box{
width:80%;
height:30px;
background:#022c22;
margin:10px auto;
border-radius:10px;
overflow:hidden;
}

.water-fill{
height:100%;
width:0%;
background:linear-gradient(90deg,#4ade80,#22c55e,#16a34a);
transition:width 0.5s;
}
*{
color:white !important;
}
</style></head>
<body>

<h2>SMART DASHBOARD</h2>

<div class="card">
<h3>Sensors</h3>

<p>Temp:  <span id=t></span></p>
<p>Hum:  <span id=h></span> %</p>
<p>Moist:  <span id=m></span> %</p>
<p>Water Level:  <span id=w></span> %</p>
<div class="water-box">
  <div id="waterFill" class="water-fill"></div>
</div>
<p>Rain:  <span id=r></span></p>
</div>

<div class="card">
<h3>CONTROL</h3>

<p>Pump</p>
<label class="switch">
  <input type="checkbox" onchange="fetch('/led1/'+(this.checked?'on':'off'))">
  <span class="slider"></span>
</label>

<p>Grass-Cutter</p>
<label class="switch">
  <input type="checkbox" onchange="fetch('/led2/'+(this.checked?'on':'off'))">
  <span class="slider"></span>
</label>

</div>

<div class="card">
<h3>Soil</h3>

<p>Sensor</p>
<input type="range" min="0" max="180" value="0" onchange="fetch('/s1?val='+this.value)">    

<p>Plough</p>
<input type="range" min="0" max="180" value="0" onchange="fetch('/s2?val='+this.value)">    
</div>

<div class="card">
<h3>Movement</h3>

<div class="pad">
  <button class="btn up"
    onmousedown="move('f')" onmouseup="stopMotor()"
    ontouchstart="move('f')" ontouchend="stopMotor()"></button>

  <button class="btn left"
    onmousedown="move('l')" onmouseup="stopMotor()"
    ontouchstart="move('l')" ontouchend="stopMotor()"></button>

  <button class="btn right"
    onmousedown="move('r')" onmouseup="stopMotor()"
    ontouchstart="move('r')" ontouchend="stopMotor()"></button>

  <button class="btn down"
    onmousedown="move('b')" onmouseup="stopMotor()"
    ontouchstart="move('b')" ontouchend="stopMotor()"></button>
</div>

<p>Speed</p>
<input type="range" min="0" max="255" value="255" onchange="fetch('/speed?val='+this.value)">
</div>


<script>
function move(dir){
  fetch('/'+dir);
}

function stopMotor(){
  fetch('/s');
}

setInterval(()=>{
fetch('/data').then(r=>r.json()).then(d=>{
t.innerText=d.temp;
h.innerText=d.hum;
m.innerText=d.moist;
w.innerText=d.water;
document.getElementById("waterFill").style.width = d.water + "%";
r.innerText=d.rain;
});
},1000);
document.querySelectorAll("input[type=range]").forEach(slider=>{
  function updateSlider(){
    let val = (slider.value - slider.min) / (slider.max - slider.min) * 100;
    slider.style.background = `linear-gradient(to right, #22c55e ${val}%, #ffffff ${val}%)`;
  }

  slider.addEventListener("input", updateSlider);

  // initial load
  updateSlider();
});
document.querySelectorAll(".btn").forEach(btn=>{
  btn.addEventListener("touchstart", ()=>{
    btn.classList.add("active");
  });

  btn.addEventListener("touchend", ()=>{
    btn.classList.remove("active");
  });
});

</script>

</body></html>
)rawliteral";

void setup(){
Serial.begin(115200);

pinMode(led1,OUTPUT);
pinMode(led2,OUTPUT);
digitalWrite(led1, 1); // OFF (Active LOW)
digitalWrite(led2, 1); // OFF

pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);

pinMode(rainPin, INPUT);

servo1.attach(servoPin1);
servo2.attach(servoPin2);
servo1.write(180);   // 👇 Servo initial 180 degree
servo2.write(30);   // 👇 Servo initial 30 degree

setupPWM();
dht.begin();

WiFi.softAP(ssid,password);

server.on("/",[]{server.send(200,"text/html",page);});
server.on("/data",[]{server.send(200,"application/json",getData());});

server.on("/led1/on",[]{digitalWrite(led1,0);server.send(200,"text","OK");});
server.on("/led1/off",[]{digitalWrite(led1,1);server.send(200,"text","OK");});

server.on("/led2/on",[]{digitalWrite(led2,0);server.send(200,"text","OK");});
server.on("/led2/off",[]{digitalWrite(led2,1);server.send(200,"text","OK");});

server.on("/s1",[]{
  int val = server.arg("val").toInt();
  int angle = 180 - val ;    //servo1 last angle
  servo1.write(angle);
  server.send(200,"text","OK");
});

server.on("/s2",[]{
  int val = server.arg("val").toInt();
  int angle = 30 - (val / 6);   // servo2: 30 → 0
  if(angle < 0) angle = 0;      // safety
  servo2.write(angle);
  server.send(200,"text","OK");
});

server.on("/f",[]{forward();});
server.on("/b",[]{reverse();});
server.on("/l",[]{left();});
server.on("/r",[]{right();});
server.on("/s",[]{stopMotor();});

server.on("/speed",[](){
  speedVal=server.arg("val").toInt();
ledcWrite(ENA,speedVal);
ledcWrite(ENB,speedVal);
server.send(200,"text","OK");
});

server.begin();
}

void loop(){
  server.handleClient();

  static unsigned long last=0;
  if(millis()-last>2000){
    last=millis();

    Serial.println("----- SENSOR -----");
    Serial.println(getData());
  }
}
