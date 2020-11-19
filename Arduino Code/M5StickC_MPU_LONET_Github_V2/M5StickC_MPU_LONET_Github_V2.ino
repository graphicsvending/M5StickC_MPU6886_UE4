#include <M5StickC.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PinButton.h>

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

//Wifi SSID and password
const char * networkSSID = "admin123"; // your SSID
const char * networkPass = "12345"; // your Password

/*
// Set your Static IP address
IPAddress local_IP(0, 0, 0, 0);
IPAddress gateway(0, 0, 0, 0);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(0, 0, 0, 0);   //optional
IPAddress secondaryDNS(8, 8, 8, 8); //optional
*/

//--------------------------------------------------------------------//

//UE4 LONET_LiveLink IP & Port
const char * udpAddress = "127.0.0.1"; // your UE4_LiveLink IP Address
const int udpPort = 3333; //Port

//UE4 LONET_LiveLink Data Packet Structure
byte prefix = 0XF2;
uint8_t buffer[50];
int arbitrary1, arbitrary2, arbitrary3, arbitrary4, arbitrary5, arbitrary6;


//--------------------------------------------------------------------//

//create UDP instance
WiFiUDP udp;

//--------------------------------------------------------------------//

//M5StickC variables
PinButton btnM5(37); //the "M5" button on the device
PinButton btnAction(39); //the "Action" button on the device

//--------------------------------------------------------------------//

//General Variables
bool networkConnected = false;
int currentScreen = 0;
int currentBrightness = 10; //12 is Max level

//--------------------------------------------------------------------//

float accX, accY, accZ;
float gyroX, gyroY, gyroZ;
float roll, pitch, yaw;

#define RAD_TO_DEG 57.324

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void setup() {
  Serial.begin(115200); // to monitor activity
  while (!Serial);

  //--------------------------------------------------------------------//
  
  /*    // config fixed ip wifi
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }
  */
  //--------------------------------------------------------------------//

  // Initialize the M5StickC object
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);

  //--------------------------------------------------------------------//

  delay(100); //wait 100ms before moving on
  connectToNetwork(); //starts Wifi connection
  while (!networkConnected) {
    delay(200);
  }

  //--------------------------------------------------------------------//

  //This initializes udp and transfer buffer
  udp.begin(udpPort);

  //--------------------------------------------------------------------//

  M5.MPU6886.Init();
  
}

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void loop() {

  btnM5.update();
  btnAction.update();

  MPU_Roll_Pitch_Yaw();

  Sending_LONET_LiveLink_User_Data();

  //--------------------------------------------------------------------//
  //page selection
  if (btnM5.isClick()) {
    switch (currentScreen) {
      case 0:
        page001();
        currentScreen = 1;
        break;
      case 1:
        page000();
        currentScreen = 0;
        break;
    }
  }

  //--------------------------------------------------------------------//
  //Brightness Adjustment
  if (btnAction.isClick()) {
    updateBrightness();
  }

  //--------------------------------------------------------------------//

  //Wait for 1 second
  delay(5);
}

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void page000() {

  //m5 battery level
  int batteryLevel = floor(100.0 * (((M5.Axp.GetVbatData() * 1.1 / 1000) - 3.0) / (4.07 - 3.0)));
  batteryLevel = batteryLevel > 100 ? 100 : batteryLevel;

  //--------------------------------------------------------------------//

  //displays the current network connection
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);

  M5.Lcd.println("SSID: " + String(networkSSID));
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.println(WiFi.macAddress());
  M5.Lcd.println();
  M5.Lcd.println("Battery:");
  M5.Lcd.println(String(batteryLevel) + "%");
}

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void page001() {

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);

  //--------------------------------------------------------------------//


}



//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void updateBrightness() {
  if (currentBrightness >= 12) {
    currentBrightness = 7;
  }
  else {
    currentBrightness++;
  }
  M5.Axp.ScreenBreath(currentBrightness);
}


//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void MPU_Roll_Pitch_Yaw() {

  // put your main code here, to run repeatedly:
  M5.MPU6886.getGyroData(&gyroX, &gyroY, &gyroZ);
  M5.MPU6886.getAccelData(&accX, &accY, &accZ);

  //reference = https://myenigma.hatenablog.com/entry/2015/11/09/183738
  roll = atan(accY / accZ) * RAD_TO_DEG;
  pitch = atan(-accX / sqrtf(accY * accY + accZ * accZ)) * RAD_TO_DEG;
  yaw = 0;
 

  M5.Lcd.setCursor(0, 30);
  M5.Lcd.printf("%5.1f, %5.1f, %5.1f", roll, pitch, yaw);
  Serial.printf("%5.1f, %5.1f, %5.1f\r\n", roll, pitch, yaw);

}

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

void Sending_LONET_LiveLink_User_Data() {

  //arbitrary6 = analogRead(inputPin1);

  arbitrary1 = roll;
  arbitrary2 = pitch;
  arbitrary3 = yaw;
  arbitrary4 = 0; //not in use
  arbitrary5 = 0; //not in use
  arbitrary6 = 0; //not in use

  udp.beginPacket(udpAddress, udpPort);
  memset(buffer, prefix, 1);
  udp.write(buffer, 1);
  udp.printf(",Source_Name001,%d,%d,%d,%d,%d,%d", arbitrary1, arbitrary2, arbitrary3, arbitrary4, arbitrary5, arbitrary6);
  udp.endPacket();
}

//0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//

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



//000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000//
