/**
On power-on, the esp will either: 
1) produce a AP hotspot 'Willa_Mower' for user login with password ‘BobdaSML' on the user's device.
  The user can then configure a new network for the mower to use by visiting 192.168.4.1 in the user's browser.
or If the network has previously been configured and is avaliable
2) the mower will connect to the previously used network.
Finally, the mower can be remotely controlled by the user's device by the given ip address.
*/
#include <WiFiClient.h>
#include <HardwareSerial.h>
// #include <ESPmDNS.h>

#include <WebServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...


SSD1306Wire display(0x3c, 21, 22);


// #include <WiFi.h>
// select which pin will trigger the configuration portal when set to LOW

int timeout = 120; // seconds to run for

//const int rc0 = 5; // GPIO pin for RC channel 2
//const int rc1 = 12; // GPIO pin for RC channel 6
//const int relay3 = 13; // GPIO pin for drive motor relay 


// radio inputs
// const int RC_CH2_PIN = 4;   // RC channel 2 (steering) D4
// const int RC_CH6_PIN = 21;  // RC channel 6 (throttle) D21 , could use 25,26,27
// Relay pin
const int MOTOR_PIN = 22;      // Motor on/off control D22 aka relay3
// const int MOTOR_DIR1_PIN = 23; // Motor direction control 1 (FB HL) D23
const int ACTUATOR_PIN1 = 16;   // Actuator control Extend RX2
const int ACTUATOR_PIN2 = 17;   // Actuator control Retract TX2

const int BLADE_PIN =23;   

//int stateForBlade = 0;

#define TRIGGER_PIN 19 // D19 For reset WiFi credential
#define TX_PIN 33
#define RX_PIN 32



WebServer server(80);

enum motorState {
  STOPPED,  FORWARD,  BACKWARD,  LEFT,  RIGHT
};

enum bladeState {
  ON, OFF
};

volatile int throttle = 0; // RC channel 6 (throttle)
volatile int steering = 0; // RC channel 2 (steering)
int motorState = STOPPED;
int bladeState = OFF;

// Define pulse width ranges for RC receiver channels
const int RC_CH2_RIGHT_MIN = 1100;
const int RC_CH2_RIGHT_MAX = 1300;
const int RC_CH2_LEFT_MIN = 1700;
const int RC_CH2_LEFT_MAX = 1900;
const int RC_CH6_FORWARD_MIN = 1100; 
const int RC_CH6_FORWARD_MAX = 1300;
const int RC_CH6_BACKWARD_MIN = 1700;
const int RC_CH6_BACKWARD_MAX = 1900;

// HTML and CSS for the web page
// String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>body {font-family: Arial; margin: 0;}"
//               "/* Style the header */.header {padding: 60px; text-align: center; background: #1abc9c; color: white;}"
//               "/* Style the buttons */.button {background-color: #008CBA; border: none; color: white; padding: 16px 32px; text-align: center; text-decoration: none;"
//               "display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer;}.button2 {background-color: #f44336;}"
//               ".button-container {text-align: center;}"
//               "#stream-container {text-align: center;}"
//               "#stream {max-width: 100%; height: auto; display: block; margin: auto;}"
//               "</style></head><body>"
//               "<div class='header'><h1>Remote Control Lawn Mower</h1><p>Control your lawn mower from anywhere</p></div>"
//               "<div id='stream-container'>"
//               "<img id='stream' src='http://192.168.168.248:81/stream'>"
//               "</div>"
//               "<div style='padding:20px' class='button-container'><a href='/forward'><button class='button'>Forward</button></a></div>"
//               "<div style='padding:20px' class='button-container'><a href='/left'><button class='button'>Left</button></a>"
//               "<a href='/stop'><button class='button button2'>Stop</button></a>"
//               "<a href='/right'><button class='button'>Right</button></a></div><div style='padding:20px' class='button-container'>"
//               "<a href='/backward'><button class='button'>Backward</button></a></div>"
//               "</body></html>";    


String html = "<!DOCTYPE html> <html>  <head> <meta name='viewport' content='width=device-width, initial-scale=1'> <style> /* Your existing CSS styles */ body { font-family: Arial; margin: 0; }  /* Style the header */ .header { padding: 60px; text-align: center; background: #1abc9c; color: white; }  /* Style the buttons */ .button { background-color: #008CBA; border: none; color: white; padding: 16px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; }  .button2 { background-color: #f44336; }  .button-container { text-align: center; } #mower-state {color: white;padding: 60px;text-align: center; background: #1abc9c;} #stream-container { text-align: center; }   #stream { max-width: 100%; height: auto; display: block; margin: auto; } </style> </head>  <body> <div class='header'> <h1>Remote Control Lawn Mower</h1> <p>Control your lawn mower from anywhere</p> </div>";    ; 
              
// String htmlWithIP = "<img id='stream' src='http://192.168.168.248:81/stream'>";
String myServerIP;
String receivedData;
String htmlWithIP = "<img id='stream' src='http://";
String htmlWithIPRemain;
String html1 ="</div>"
              "<div style='padding:20px' class='button-container'> <button class='button' onclick='sendCommand(\"forward\");'>Forward</button> </div> <div style='padding:20px' class='button-container'> <button class='button' onclick='sendCommand(\"left\");'>Left</button> <button class='button button2' onclick='sendCommand(\"stop\");'>Stop</button> <button class='button' onclick='sendCommand(\"right\");'>Right</button> </div> <div style='padding:20px' class='button-container'> <button class='button' onclick='sendCommand(\"backward\");'>Backward</button> </div> <!-- Display the mower state here --> <div style='padding:20px' class='button-container'> <button class='button' onclick='sendCommand(\"bladeon\");'>Blade On</button> <button class='button button2' onclick='sendCommand(\"bladeoff\");'>Blade Off</button> </div><div id='mower-state'><p>Mower State: </p></div><script> function sendCommand(command){ fetch('/' + command, { method: 'POST', }).then(response => { if (!response.ok) { throw new Error('Network response was not ok'); } console.log('Command sent successfully:', command); history.pushState({}, null, '/' + command); if (command === 'right') { updateMowerState('Right'); } if (command === 'left') { updateMowerState('Left'); } if (command === 'backward') { updateMowerState('Backward'); } if (command === 'forward') { updateMowerState('Forward'); } if (command === 'stop') { updateMowerState('Stop'); } }).catch(error => { console.error('Error sending command:', error); }); } function updateMowerState(state) { var mowerStateElement = document.getElementById('mower-state'); mowerStateElement.innerHTML = \"<p>Mower State: \" + state + \"</p>\"; } </script></body></html>"; 

String webPageHTML;


void handleRoot() {
  server.send(200, "text/html", webPageHTML);
}
void handleForward() {
  motorState = FORWARD;
  server.send(200, "text/html", "<div class='header'><p>Mower State: Forward</p></div>");
}
void handleBackward() {
  motorState = BACKWARD;
  server.send(200, "text/html",  "<div class='header'><p>Mower State: Backward</p></div>");
}
void handleLeft() {
  motorState = LEFT;
  server.send(200, "text/html", "<div class='header'><p>Mower State: Left</p></div>");
}
void handleRight() {
  motorState = RIGHT;
  server.send(200, "text/html",  "<div class='header'><p>Mower State: Right</p></div>");
}
void handleStop() {
  motorState = STOPPED;
  server.send(200, "text/html", "<div class='header'><p>Mower State: Stopped</p></div>");
}

void handleBladeOn() {
  bladeState = ON;
  Serial.println("The Blade is on");
  server.send(200, "text/html", "<div class='header'><p>Mower State: bladeon</p></div>");
}

void handleBladeOff() {
  bladeState = OFF;
  Serial.println("The Blade is off");
  server.send(200, "text/html", "<div class='header'><p>Mower State: bladeoff</p></div>");
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found");
}

// Handle motor control path request
void handleMotor() {
  String state = server.arg("state");
  if (state == "forward") {
    motorState = FORWARD;
  } else if (state == "backward") {
    motorState = BACKWARD;
  } else if (state == "left") {
    motorState = LEFT;
  } else if (state == "right") {
    motorState = RIGHT;
  } else if (state == "stop") {
    motorState = STOPPED;
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// Handle actuator control path request
void handleActuator() {
  String state = server.arg("state");
  if (state == "extend") {
    digitalWrite(ACTUATOR_PIN1, HIGH);
    digitalWrite(ACTUATOR_PIN2, LOW);
  } else if (state == "retract") {
    digitalWrite(ACTUATOR_PIN1, LOW);
    digitalWrite(ACTUATOR_PIN2, HIGH);
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleBlade() {
  if (bladeState == ON){
    digitalWrite(BLADE_PIN, HIGH);
    Serial.println("OnOnOn");
    //showBladeOn();
  }
  if (bladeState == OFF){
    digitalWrite(BLADE_PIN, LOW);
    Serial.println("OfOfOf");
    //showConnected();
  }
}


void showWillaIP(void) {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Connect To");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 16, "192.168.4.1");
  display.display();
}

void showConnected(void) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Connect");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 16, myServerIP);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 32, "To Control Mower");
  display.display();
}

void showFoward(void) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "It's Foward");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 16, "Connect");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 26, myServerIP);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 36, "To Control Mower");
  display.display();
}

void showBladeOn(void) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "BLADE ON");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 16, "Connect");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 26, myServerIP);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 36, "To Control Mower");
  display.display();
}

void showCredentialCleaned(void) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "WiFi Credential");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 16, "Has Been Cleaned");
  //setFont(ArialMT_Plain_16);
  //display.drawString(0, 32, "To Control Mower");
  display.display();
}

void showStateOnScreen(void) {
  if (bladeState == ON && motorState == FORWARD){
    showBladeOn();
  }else if(bladeState == ON && motorState != FORWARD){
    showBladeOn();
  }else if(bladeState == OFF && motorState == FORWARD){
    showFoward();
  }else{
    showConnected();
  }
}


void setup() {
  //Wire.begin(0x3C,21,22);//sda,scl
  //display.clearDisplay();
  
  Serial.println("\n Starting");
  display.init();
  display.flipScreenVertically();
  display.clear();
  
  showWillaIP();

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  // espSerial.begin(9600);
  
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  //showWillaIP();
  WiFiManager wm;
 


bool res;
 

  res = wm.autoConnect("Willa_Mower","BobdaSML"); // password protected ap
 
    if(!res) {
 
        Serial.println("Failed to connect");
 
        // ESP.restart();
 
    }else{
      myServerIP = WiFi.localIP().toString();
      showConnected();
    }

//while (WiFi.status() != WL_CONNECTED) {
    //delay(10000);
   // Serial.println("Connecting to WiFi...");
 //  }
  
  // pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // pinMode(RC_CH2_PIN, INPUT); //rc0
  //attachInterrupt(digitalPinToInterrupt(RC_CH2_PIN), [](){
  //  throttle = pulseIn(RC_CH2_PIN, HIGH, 20000) / 10;
  //}, CHANGE);
  
  // pinMode(RC_CH6_PIN, INPUT); //rc1
  //attachInterrupt(digitalPinToInterrupt(RC_CH6_PIN), [](){
  //  steering = pulseIn(RC_CH6_PIN, HIGH, 20000) / 10;
  //}, CHANGE);
  pinMode(MOTOR_PIN, OUTPUT); // relay3
  pinMode(ACTUATOR_PIN1, OUTPUT);
  pinMode(ACTUATOR_PIN2, OUTPUT);
  pinMode(BLADE_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(BLADE_PIN, LOW);
  digitalWrite(ACTUATOR_PIN1, LOW);
  digitalWrite(ACTUATOR_PIN2, LOW);
  // Serial.println("Connected to WiFi");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  
  server.on("/forward", handleForward);
  server.on("/left", handleLeft);
  server.on("/stop", handleStop);
  server.on("/right", handleRight);
  server.on("/backward", handleBackward);
  server.onNotFound(handleNotFound);
  
  server.on("/motor", handleMotor); //n
  server.on("/actuator", handleActuator);//n

  server.on("/bladeon", handleBladeOn);
  server.on("/bladeoff", handleBladeOff);
  
  server.begin();
  
}


void loop() {
delay(50);
handleBlade();
showStateOnScreen();
  
if ( digitalRead(TRIGGER_PIN) == LOW) {
  bool res;
    WiFiManager wm;    
 
    //reset settings - for testing
    //wm.resetSettings();
  
    // set configportal timeout
    wm.resetSettings();
    Serial.print("Cleaned");
    showCredentialCleaned();
    delay(5000);
    ESP.restart();
  }

  if (WiFi.status() != WL_CONNECTED){
    delay(5000);
    Serial.print("Unconneted");
  }


  WiFiManager wm; 
  String ssid;
  String passWord;
  ssid = wm.getWiFiSSID();
  passWord = wm.getWiFiPass();

  
  Serial1.print(ssid);
  Serial1.print(",");
  Serial1.print(passWord);
  Serial1.print(".");
  Serial.println(ssid);
  Serial.println(passWord);
  // espSerial.println(ssid);
  // espSerial.println(ssid);

receivedData = Serial1.readStringUntil(',');
//String receivedData;

// if(receivedData == "192.168.168.248"){
//   Serial.println("yes, it is equal");
// }


Serial.println(receivedData);

htmlWithIPRemain = receivedData + ":81/stream'>";

webPageHTML = html + htmlWithIP + htmlWithIPRemain + html1;

//Serial.println(webPageHTML);
server.handleClient();
  // put your main code here, to run repeatedly:
  // Serial.println(WiFi.localIP());
  // int CH2 = pulseIn(RC_CH2_PIN, HIGH, 25000);
  // int CH6 = pulseIn(RC_CH6_PIN, HIGH, 25000);

  // Determine motor state based on RC inputs and current motorState
  if (motorState == FORWARD) { // || (RC_CH2_PIN > RC_CH6_FORWARD_MIN && RC_CH6_PIN < RC_CH6_FORWARD_MAX)) {
    digitalWrite(MOTOR_PIN, HIGH);
    Serial.println("It does work forward");
    //showFoward();
    // digitalWrite(MOTOR_DIR1_PIN, HIGH);
    // digitalWrite(MOTOR_DIR2_PIN, LOW); // Backward is unnecessary
    digitalWrite(ACTUATOR_PIN1, LOW); // Retract actuator when moving forward
    digitalWrite(ACTUATOR_PIN2, LOW); // Retract actuator when moving forward
  //} else if (motorState == BACKWARD || (CH6 > RC_CH6_BACKWARD_MIN && CH6 < RC_CH6_BACKWARD_MAX)) {
  //  digitalWrite(MOTOR_PIN, HIGH);
  //  digitalWrite(MOTOR_DIR1_PIN, LOW); // could use this as stop
  //  digitalWrite(MOTOR_DIR2_PIN, HIGH);
  //  digitalWrite(ACTUATOR_PIN1, LOW); // Retract actuator when moving forward
  //  digitalWrite(ACTUATOR_PIN2, LOW); // Retract actuator when moving forward
  } else if (motorState == LEFT) { // || (RC_CH2_PIN > RC_CH2_LEFT_MIN && RC_CH6_PIN < RC_CH2_LEFT_MAX)) {
    // digitalWrite(MOTOR_PIN, HIGH);
    // digitalWrite(MOTOR_DIR1_PIN, LOW);
    // digitalWrite(MOTOR_DIR2_PIN, HIGH);
    //showConnected();
    Serial.println("It does work left");
    digitalWrite(ACTUATOR_PIN1, LOW); // Retract actuator when turning left
    digitalWrite(ACTUATOR_PIN2, HIGH); // Retract actuator when moving forward
    delay(1000);
    digitalWrite(ACTUATOR_PIN2, LOW); // Retract actuator when moving forward
  } else if (motorState == RIGHT) { //T || (RC_CH2_PIN > RC_CH2_RIGHT_MIN && RC_CH6_PIN < RC_CH2_RIGHT_MAX)) {
    // digitalWrite(MOTOR_PIN, HIGH);
    // digitalWrite(MOTOR_DIR1_PIN, HIGH);
    // digitalWrite(MOTOR_DIR2_PIN, LOW);
    //showConnected();
    Serial.println("It does work right");
    digitalWrite(ACTUATOR_PIN1, HIGH); // Extend actuator when turning right
    digitalWrite(ACTUATOR_PIN2, LOW); // Retract actuator when moving forward
    delay(1000);
    digitalWrite(ACTUATOR_PIN1, LOW); // Retract actuator when moving forward
  } else { // if STOP
    digitalWrite(MOTOR_PIN, LOW);
    Serial.println("It is stop");
    digitalWrite(ACTUATOR_PIN1, LOW); // Retract actuator when moving forward
    digitalWrite(ACTUATOR_PIN2, LOW); // Retract actuator when moving forward
    
  }
}
