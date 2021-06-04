#include <M5StickC.h>
#include <Wire.h>
#include "Madgwick_Quaternion.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

//Wifi SSID and password
const char * networkSSID = "iot@sph"; //your wifi SSID
const char * networkPass = "DCiik4kW"; //your wifi password

/*
  // Set your Static IP address
  IPAddress local_IP(10, 134, 1, 232);
  IPAddress gateway(10, 134, 1, 254);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(10, 134, 1, 131);   //optional
  IPAddress secondaryDNS(8, 8, 8, 8); //optional
*/
//--------------------------------------------------------------------//
bool serial_output = true; //Using USB Cable...Ideal for Realtime Camera Tracking Solution <<<running with Python_LiveLink_Data_Sender>>>
bool wifi_udp_output = true; // Using Wifi...Ideal for external control over network (data transfer speed might be vary based on your router and arduino board!!!)


//--------------------------------------------------------------------//

//UE4 LiveLink IP & Port
const char * udpAddress = "10.134.1.204"; // your UE4_LiveLink IP Address
const int udpPort = 54321; //Port


//--------------------------------------------------------------------//

//create UDP instance
WiFiUDP udp;

//--------------------------------------------------------------------//

Madgwick_Quaternion M_Q_Filter;

//--------------------------------------------------------------------//

//General Variables
bool networkConnected = false;

//--------------------------------------------------------------------//

float acc[3];
float accOffset[3];
float gyro[3];
float gyroOffset[3];
float mag[3];
float magOffset[3];
float magmax[3];
float magmin[3];
uint8_t setup_flag = 1;
uint8_t action_flag = 1;

float heading = 0;

float pitch = 0.0F;
float roll  = 0.0F;
float yaw   = 0.0F;

float quat_w = 0.0F;
float quat_x = 0.0F;
float quat_y = 0.0F;
float quat_z = 0.0F;

uint8_t smoothen = 100;
float strength = 1;

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//


void setup() {

  Serial.begin(115200);
  while (!Serial);

  // Initialize the M5StickC object
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 0);

  //--------------------------------------------------------------------//

  if (wifi_udp_output) {

    /*
      // config fixed ip wifi
      if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
      }
    */
    //--------------------------------------------------------------------//

    delay(100); //wait 100ms before moving on
    connectToNetwork(); //starts Wifi connection
    while (!networkConnected) {
      delay(200);
    }

    //--------------------------------------------------------------------//

    //This initializes udp and transfer buffer
    udp.begin(udpPort);
  }

  //--------------------------------------------------------------------//

  M5.MPU6886.Init();

  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);


  //--------------------------------------------------------------------//

  calibrate6886();

  //--------------------------------------------------------------------//

  M_Q_Filter.begin(smoothen); //100Hz

  //--------------------------------------------------------------------//

  calibrate_waiting(10);

  //--------------------------------------------------------------------//

  ////

  M5.Lcd.fillScreen(BLACK);

}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void connectToNetwork() {
  //logger("Connecting to SSID: " + String(networkSSID), "info");
  M5.Lcd.println("Connecting to SSID: " + String(networkSSID));
  Serial.println("Connecting to SSID: " + String(networkSSID));

  WiFi.disconnect(true);
  WiFi.onEvent(WiFiEvent);

  WiFi.mode(WIFI_STA); //station
  WiFi.setSleep(false);

  WiFi.begin(networkSSID, networkPass);
}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      M5.Lcd.println("Network connected!");
      M5.Lcd.println("SSID: " + String(networkSSID));
      M5.Lcd.println(WiFi.localIP());
      M5.Lcd.println(WiFi.macAddress());

      Serial.println("Network connected!");
      Serial.println("SSID: " + String(networkSSID));
      Serial.println(WiFi.localIP());
      Serial.println(WiFi.macAddress());

      networkConnected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      M5.Lcd.println("Network connection lost!");
      Serial.println("Network connection lost!");

      networkConnected = false;
      break;
  }
}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

//MPU6886 Calibration
void calibrate6886() {

  float gyroSum[3];
  float accSum[3];
  int counter = 500;
  for (int i = 0; i < counter; i++) {
    M5.MPU6886.getGyroData(&gyro[0], &gyro[1], &gyro[2]);
    M5.MPU6886.getAccelData(&acc[0], &acc[1], &acc[2]);
    gyroSum[0] += gyro[0];
    gyroSum[1] += gyro[1];
    gyroSum[2] += gyro[2];
    accSum[0] += acc[0];
    accSum[1] += acc[1];
    accSum[2] += acc[2];
    delay(2);
  }
  gyroOffset[0] = gyroSum[0] / counter;
  gyroOffset[1] = gyroSum[1] / counter;
  gyroOffset[2] = gyroSum[2] / counter;
  accOffset[0] = accSum[0] / counter;
  accOffset[1] = accSum[1] / counter;
  accOffset[2] = (accSum[2] / counter) - 1.0; //Gravitational Acceleration 1G, assuming that the M5 button is facing upward

}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

//////
void calibrate_waiting(uint32_t timeout)
{
  int16_t value_x_min = 0;
  int16_t value_x_max = 0;
  int16_t value_y_min = 0;
  int16_t value_y_max = 0;
  int16_t value_z_min = 0;
  int16_t value_z_max = 0;
  uint32_t timeStart = 0;

  timeStart = millis();

  while ((millis() - timeStart) < timeout)
  {
    if (digitalRead(M5_BUTTON_HOME) == LOW) {
      setup_flag = 1;
      while (digitalRead(M5_BUTTON_HOME) == LOW);
      break;
    }
    delay(100);
  }

}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void applycalibration() {

  M5.MPU6886.getGyroData(&gyro[0], &gyro[1], &gyro[2]);
  M5.MPU6886.getAccelData(&acc[0], &acc[1], &acc[2]);

  gyro[0] -= gyroOffset[0];
  gyro[1] -= gyroOffset[1];
  gyro[2] -= gyroOffset[2];
  acc[0] -= accOffset[0];
  acc[1] -= accOffset[1];
  acc[2] -= accOffset[2];

  //fake magnetometer data cuz MPU6886 doesn't come with BMM 150 chip
  mag[0] = 0;
  mag[1] = 0;
  mag[2] = 0;
}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void IMU_Update() {

  heading = atan2(mag[0], mag[1]);
  if (heading < 0)
    heading += 2 * PI;
  if (heading > 2 * PI)
    heading -= 2 * PI;

  //--------------------------------------------------------------------//


  M_Q_Filter.update(gyro[0]*strength, gyro[1]*strength, gyro[2]*strength, acc[0], acc[1], acc[2], mag[0], mag[1], mag[2]);

  //--------------------------------------------------------------------//

  roll = M_Q_Filter.getRoll();
  pitch = M_Q_Filter.getPitch();
  yaw   = M_Q_Filter.getYaw();

  //for quaternion output (by zeketan)
  quat_w = M_Q_Filter.getQuat_W();
  quat_x = M_Q_Filter.getQuat_X();
  quat_y = M_Q_Filter.getQuat_Y();
  quat_z = M_Q_Filter.getQuat_Z(); //-0.005 anti yaw drifting

}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//


void Print_IMU_Info() {

  //--------------------------------------------------------------------//
  //m5 battery level
  int batteryLevel = floor(100.0 * (((M5.Axp.GetVbatData() * 1.1 / 1000) - 3.0) / (4.07 - 3.0)));
  batteryLevel = batteryLevel > 100 ? 100 : batteryLevel;

  //M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("IMU INFO");

  M5.Lcd.setCursor(70, 0);
  M5.Lcd.println("Battery: " + String(batteryLevel) + "%");

  //  M5.Lcd.setCursor(0, 10);
  //  M5.Lcd.println("  X       Y       Z");
  //
  //  M5.Lcd.setCursor(0, 20);
  //  M5.Lcd.printf("%6.2f, %6.2f, %6.2f", gyro[0], gyro[1], gyro[2]);//deg
  //  M5.Lcd.setCursor(140, 20);
  //  M5.Lcd.print("o/s");
  //
  //  M5.Lcd.setCursor(0, 30);
  //  M5.Lcd.printf("%5.2f, %5.2f, %5.2f", acc[0], acc[1], acc[2]);
  //  M5.Lcd.setCursor(140, 30);
  //  M5.Lcd.print("G");
  //
  //  M5.Lcd.setCursor(0, 40);
  //  M5.Lcd.printf("headingDegrees: %2.1f", heading * 180 / M_PI);
  //
  //  M5.Lcd.setCursor(0, 50);
  //  M5.Lcd.println("  Roll    Pitch   Yaw");
  //
  M5.Lcd.setCursor(0, 60);
  M5.Lcd.printf("%5.2f, %5.2f, %5.2f", roll, pitch, yaw);

  M5.Lcd.setCursor(0, 70);
  M5.Lcd.printf("%5.2f, %5.2f, %5.2f, %5.2f", quat_w, quat_x, quat_y, quat_z);

  //Serial.printf("%5.2f, %5.2f, %5.2f, %5.2f\r\n", quat_w, quat_x, quat_y, quat_z);
  //Serial.printf("%5.2f, %5.2f, %5.2f\r\n", roll, pitch, yaw);


}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void sending_imu_data()
{

  DynamicJsonDocument writeData(256);

  // Create the Json string from imu sensor
  JsonObject JO = writeData.createNestedObject("Z_Tracker01");
  JO["FrameData"] = "Property";
  JsonArray values = JO.createNestedArray("UserData");
  values.add(quat_w);
  values.add(quat_x);
  values.add(quat_y);
  values.add(quat_z);
  values.add(0); //blank data
  values.add(0); //blank data
  values.add(0); //blank data
  values.add(0); //blank data
  values.add(0); //blank data
  values.add(0); //blank data

  //--------------------------------------------------------------------//

  if (wifi_udp_output) {
    // Send UDP packet
    udp.beginPacket(udpAddress, udpPort);
    serializeJson(writeData, udp);
    udp.endPacket();
  }

  //--------------------------------------------------------------------//

  if (serial_output) {
    serializeJson(writeData, Serial); // what being sent.
    Serial.println("");
  }

  //--------------------------------------------------------------------//

}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

// Not in use
void M5_LCD_Print_Network_Info() {

  //m5 battery level
  int batteryLevel = floor(100.0 * (((M5.Axp.GetVbatData() * 1.1 / 1000) - 3.0) / (4.07 - 3.0)));
  batteryLevel = batteryLevel > 100 ? 100 : batteryLevel;

  //--------------------------------------------------------------------//

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);

  M5.Lcd.printf("Network info");
  M5.Lcd.println("SSID: " + String(networkSSID));
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.println(WiFi.macAddress());
  M5.Lcd.println();
  M5.Lcd.println("Battery: " + String(batteryLevel) + "%");
  //M5.Lcd.println(String(batteryLevel) + "%");
}

//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void loop() {

  applycalibration();

  //--------------------------------------------------------------------//

  IMU_Update();

  //--------------------------------------------------------------------//

  Print_IMU_Info();

  //--------------------------------------------------------------------//

  sending_imu_data();

  //--------------------------------------------------------------------//]

  //delay(10);

  //--------------------------------------------------------------------//

  //  // for re-calibration
  //  if (!setup_flag) {
  //    setup_flag = 1;
  //    calibrate6886();
  //    calibrate_waiting(100000);
  //  }
  //
  //  //--------------------------------------------------------------------//
  //
  //  if (digitalRead(M5_BUTTON_HOME) == LOW) {
  //    setup_flag = 0;
  //    while (digitalRead(M5_BUTTON_HOME) == LOW);
  //  }
  //
  //  if (setup_flag == 0) {
  //    M5.Lcd.setCursor(140, 70);
  //    M5.Lcd.printf("CAL");
  //  } else {
  //    M5.Lcd.printf("   ");
  //  }

  //--------------------------------------------------------------------//
}
