#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <LOLIN_HP303B.h>

LOLIN_HP303B HP303BPressureSensor;
long lastAltTimeReceived;
long lastAltReceived;

#define WIFI_CHANNEL 1
esp_now_peer_info_t slave;
//uint8_t remoteMac[] = {0x30, 0xAE, 0xA4, 0x27, 0xCE, 0xAD};
uint8_t remoteMac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


const esp_now_peer_info_t *peer = &slave;
int altitude = 0;

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
  if (lastAltReceived > (millis() - 500))
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


void setup()
{
  Serial.begin(115200);
  Serial.print("\r\n\r\n");

  Wire.begin();

  HP303BPressureSensor = LOLIN_HP303B();
  HP303BPressureSensor.begin();
  
  WiFi.mode(WIFI_STA);
  Serial.println( WiFi.softAPmacAddress() );
  WiFi.disconnect();
  if(esp_now_init() == ESP_OK)
  {
    Serial.println("ESP NOW INIT!");
  }
  else
  {
    Serial.println("ESP NOW INIT FAILED....");
  }
  
  

   memcpy( &slave.peer_addr, &broadcastAddress, 6 );
  slave.channel = WIFI_CHANNEL;
  slave.encrypt = 0;
  if( esp_now_add_peer(peer) == ESP_OK)
  {
    Serial.println("Added Peer!");
  }

  esp_now_register_send_cb(OnDataSent);
}

void loop()
{
  delay(250);   
  altitude = (int)getAltitude();
  
   if( esp_now_send(broadcastAddress, (uint8_t*)&altitude, sizeof(altitude)) == ESP_OK)
  {
    Serial.printf("\r\n0xFF Success Sent Value->\t%d", altitude);
  }
  else
  {
    Serial.printf("\r\n0xFF DID NOT SEND....");
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
