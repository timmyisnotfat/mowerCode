/**********************************************************************
  Filename    : Camera Web Server
  Description : The camera images captured by the ESP32S3 are displayed on the web page.
  Auther      : www.freenove.com
  Modification: 2022/10/31
**********************************************************************/
/**********************************************************************
  Do NOT use GPIO 4-13, GPIO 15 -18 in this board.
**********************************************************************/
#include "esp_camera.h"
#include <WiFi.h>
#include <HardwareSerial.h>

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD

#include "camera_pins.h"

// ===========================
// Enter your WiFi credentials
// ===========================
// const char* ssid     = "axp88888";
// const char* password = "axp888888";

String receivedSSID;
String receivedPassword;
String sendIPaddress;

#define RX_PIN 42
#define TX_PIN 41

// #define TX_PIN_ForIP 17
// #define RX_PIN_FORIP 18

// HardwareSerial espSerial(13);

void startCameraServer();

void setup() {
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.begin(115200);
  delay(5000);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 16;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  // for larger pre-allocated frame buffer.
  if(psramFound()){
    config.jpeg_quality = 16;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    config.jpeg_quality = 16;
    config.frame_size = FRAMESIZE_VGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  s->set_vflip(s, 1); // flip it back
  s->set_brightness(s, 1); // up the brightness just a bit
  s->set_saturation(s, 0); // lower the saturation


  String receivedData = Serial1.readStringUntil('\n');
    int commaIndex = receivedData.indexOf(',');
    int dotIndex = receivedData.indexOf('.');

    if (commaIndex != -1) {
      // Extract the SSID and password from the received data
       receivedSSID = receivedData.substring(0, commaIndex);
       receivedPassword = receivedData.substring(commaIndex + 1,dotIndex);
       const char* ssidChar = receivedSSID.c_str();
      const char* passwordChar = receivedPassword.c_str();
      Serial.print("ssid=");
      Serial.print(ssidChar);
      Serial.print("pwd=");
      Serial.println(passwordChar);

      // String ssid = "axp88888";
      // String password = "axp888888";
      // if(receivedSSID == ssid){
      //   Serial.print("yesyesyes");
      // }else{
      //   Serial.print("nonono");
      // }

      // if(receivedPassword == password){
      //   Serial.print("hhihihihihi");
      // }else{
      //   Serial.print("noooooo");
      // }
  WiFi.begin(receivedSSID, receivedPassword);
      }

  // String ssid = "axp88888";
  //     String password = "axp888888";    
  
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  delay(1000);
  Serial.print(Serial.read());

  


  String ip_address = WiFi.localIP().toString();
  Serial.print("localIPString = ");
  Serial.println(ip_address);
  // int testNum = 192;
  Serial1.print(ip_address);
  // Serial.print(first_segment);
  Serial1.print(",");
}
