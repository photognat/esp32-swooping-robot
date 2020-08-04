#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LOLIN_HP303B.h>
#include "SSD1306.h" 


LOLIN_HP303B HP303BPressureSensor;
SSD1306  display(0x3c, 5, 4);


int startRecordingHeight = 400;
int startShootingHeight = 50;
int commanderHeight = 0;
int myHeight = 0;
float lastAltReceived = 0;
long lastAltTimeReceived;

long startedShooting = 0;
long shootingTime = 20000;

long lastHeightReading = 0;

int recording = 0;

#define focus 12 //MIDDLE
#define shutter 14 // TIP

#define OFF 1
#define ON 0

void InitESPNow() {
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}


void startShooting(){
  startedShooting = millis();
  //pinMode(ground, OUTPUT);
  pinMode(focus, OUTPUT);
  pinMode(shutter, OUTPUT);
  
  delay(500);
  digitalWrite(focus, ON);
  delay(200);
  digitalWrite(shutter, ON);
  
}

void endShooting(){
  pinMode(focus, INPUT);
  pinMode(shutter, INPUT);
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);
  Serial.println("");

   memcpy(&commanderHeight, data, sizeof(commanderHeight));

  Serial.println("Height diff: " + String(commanderHeight - myHeight));
  Serial.println("My height: " + String(myHeight));

  digitalWrite(13, LOW);
  if (recording == 0)
    recording = 1;
    
  if (commanderHeight == 0 || myHeight == 0)
    return;
    
  if (abs(commanderHeight - myHeight) <= startRecordingHeight){
    digitalWrite(26, LOW);
    Serial.println("Start recording!");
    recording = 2;
  }

  int heightChangeSinceLastUpdate = lastHeightReading - commanderHeight;

  if (startedShooting == 0 && (
    (commanderHeight - myHeight - heightChangeSinceLastUpdate) <= startShootingHeight || 
    abs(commanderHeight - myHeight) <= startShootingHeight))
   {
      startShooting();
      Serial.println("Start shooting!");
  }

  lastHeightReading = commanderHeight;
}

float calcPressureAltitude2(float pressure) {
  //float pressure=((pressureValue/1024.0)+0.095)/0.000009;
 
// Calculate Height using formula above
float height = 0.0;
float A = 101325.0;
float B = 2.25577E-05;
float C = 5.25588;
 
  return (((pow((pressure/A),1/C))-1)/-B);
}

float getAltitude()
{
  if (lastAltReceived > (millis() - 1000))
    return lastAltReceived;

  lastAltTimeReceived = millis();

  unsigned char pressureCount = 20;
  //int32_t pressure[pressureCount];
  unsigned char temperatureCount = 20;
  int32_t temperature[temperatureCount];

  //This function writes the results of continuous measurements to the arrays given as parameters
  //The parameters temperatureCount and pressureCount should hold the sizes of the arrays temperature and pressure when the function is called
  //After the end of the function, temperatureCount and pressureCount hold the numbers of values written to the arrays
  //Note: The HP303B cannot save more than 32 results. When its result buffer is full, it won't save any new measurement results
  //int16_t ret = HP303BPressureSensor.getContResults(temperature, temperatureCount, pressure, pressureCount);
  int32_t pressure;
    int16_t oversampling = 3;
    float alt_meters = 0;
    int16_t  ret = HP303BPressureSensor.measureTempOnce(pressure);

     ret = HP303BPressureSensor.measurePressureOnce(pressure);

  if (ret != 0){
    Serial.println();
    Serial.println();
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
    return 0;
  } else {
   
   
    lastAltReceived = calcPressureAltitude2(pressure);
    return lastAltReceived;
  }
}

//TwoWire Wire1=TwoWire(1);

void setup() {
   Serial.begin(115200);
  Serial.println("Starting");
  
    Wire1.begin(27, 33);

  HP303BPressureSensor = LOLIN_HP303B();
  HP303BPressureSensor.begin(Wire1, 0x77);

  display.init();
  display.drawString(0, 0, "Testing!");
  display.display();

  startShooting();
  delay(1000);
  endShooting();
  startedShooting = 0;

  display.init();
  display.drawString(0, 0, "Done! Booting up now.");
  display.display();


  pinMode(26, OUTPUT);
  pinMode(13, OUTPUT);

  digitalWrite(13, HIGH);
  digitalWrite(26, HIGH);
  
  WiFi.mode(WIFI_STA);
  InitESPNow();
  esp_now_register_recv_cb(OnDataRecv);
}

long lastAltiCheck = 0;

void loop() {

  if (startedShooting > 0 && (millis() - startedShooting) > shootingTime){
    endShooting();
  }
  
  if ((millis() - lastAltiCheck) > 250){
    myHeight = (int)getAltitude();

    display.clear();
    display.drawString(0, 0, "My height: " + String(myHeight) + "m");

    if (recording > 0)
      display.drawString(0, 10, "CMD height: " + String(commanderHeight) + "m");
    else
      display.drawString(0, 10, "Commander: DISCONNECTED");
      
    if (startedShooting > 0){
      display.drawString(0, 20, "Shooting for " + String( (shootingTime - (millis() - startedShooting)) / 1000) + "s");
    }
    if (recording == 2){
      display.drawString(0, 30, "Recording!");
    } else if (recording == 1){
      display.drawString(0, 30, "Woke up video camera");
    }
    display.display();
  }

  delay(1);
  yield();
}
