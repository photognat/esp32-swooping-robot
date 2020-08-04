#include <ESP8266WiFi.h>
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h


const char* ssid     = "DIRECT-jGC2:DSC-RX10M2";
const char* password = "4VcjMe2J";     // your WPA2 password
 
const char* host = "192.168.122.1";   // fixed IP of camera
const int httpPort = 8080;


char MOVIEMODE[] = "{\"method\":\"setShootMode\",\"params\":[\"movie\"],\"id\":1,\"version\":\"1.0\"}";
char GETVERSIONS[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"getVersions\",\"params\":[]}";
char GETAVAILABLE[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"getAvailableApiList\",\"params\":[]}";
char STARTRECMODE[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startRecMode\",\"params\":[]}";
char STARTLIVEVIEW[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startLiveview\",\"params\":[]}";
//char STARTRECMODE[] = "'{\"method\":\"startRecMode\", \"params\":[], \"id\":1, \"version\":\"1.0\"}'";
char GETEVENT[] = "{\"method\":\"getEvent\",\"params\":[true],\"id\":1,\"version\":\"1.0\"}";
char SETMOVIEMODE[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"setShootMode\",\"params\":[\"movie\"]}";
//char GETEVENT[] = "'{\"method\":\"getEvent\", \"params\":[true], \"id\":1, \"version\":\"1.0\"}'";

char STARTRECORD[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startMovieRec\",\"params\":[]}";
//char STARTRECORD[] = "'{\"method\":\"startMovieRec\", \"params\":[], \"id\":1, \"version\":\"1.0\"}'";
char STOPRECORD[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"stopMovieRec\",\"params\":[]}";

//char STOPRECORD[] = "'{\"method\":\"stopMovieRec\", \"params\":[], \"id\":1, \"version\":\"1.0\"}'";
char ZOOMINSTART[] = "'{\"method\":\"actZoom\", \"params\":[\"in\", \"start\"], \"id\":1, \"version\":\"1.0\"}'";
char ZOOMINSTOP[] = "'{\"method\":\"actZoom\", \"params\":[\"in\", \"stop\"], \"id\":1, \"version\":\"1.0\"}'";
char ZOOMOUTSTART[] = "'{\"method\":\"actZoom\", \"params\":[\"out\", \"start\"], \"id\":1, \"version\":\"1.0\"}'";
char ZOOMOUTSTOP[] = "'{\"method\":\"actZoom\", \"params\":[\"out\", \"stop\"], \"id\":1, \"version\":\"1.0\"}'";
WiFiClient client;


struct cameraStatus {
  bool recording = false;
  int zoomPercent = 0;
  bool connected = false;
  long zoomInUntil = 0;
  long zoomOutUntil = 0;
};
cameraStatus camera;
long lastmillis = 0;
long startedRecording = 0;
long recordTime = 600 * 1000; // 10 minutes

void setup() {
   Serial.begin(115200);
  Serial.println("Starting");
  WiFi.mode(WIFI_OFF);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  delay(3000);
  pinMode(D4, INPUT);
  pinMode(D3, INPUT);
  while (digitalRead(D3) == 1){
    delay(200);
    Serial.println("waiting");
  }
  
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("start wifi!");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
  }
  startRecMode();

   while (digitalRead(D4) == 1){
    delay(200);
    Serial.println("waiting to record");
  }
  startRecord();
}


void httpPost(char* jString) {
  Serial.print("Msg send: ");Serial.println(jString);
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  else {
    Serial.print("connected to ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(httpPort);
  }
 
  // We now create a URI for the request
  String url = "/sony/camera";
 
  Serial.print("Requesting URL: ");
  Serial.println(url);
 
  // This will send the request to the server
  client.print(String("POST " + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n"));
  client.println("Content-Type: application/json; charset=utf-8");
  client.print("Content-Length: ");
  client.println(strlen(jString));
  // End of headers
  client.println();
  // Request body
  client.println(jString);
  Serial.println("wait for data");
  lastmillis = millis();
  while (!client.available() && millis() - lastmillis < 8000) {} // wait 8s max for answer
 
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("----closing connection----");
  Serial.println();
  client.stop();
}

void stopRecord(){
  httpPost(GETEVENT);
  httpPost(STOPRECORD);
  camera.recording = false;
  startedRecording = 0;
}

void startRecMode(){
  if (WiFi.status() == WL_CONNECTED){
    httpPost(GETVERSIONS);
    httpPost(GETEVENT);
    httpPost(STARTRECMODE);
  }
}

void startRecord(){
  while (!camera.recording){
    if (WiFi.status() == WL_CONNECTED){
      if (!camera.recording){

        
        //delay(4000);
        //httpPost(GETAVAILABLE);
        httpPost(GETEVENT);
       // delay(2000);
       // httpPost(STARTLIVEVIEW);
        //delay(8000);
        httpPost(STARTRECORD);
        camera.recording = true;
        startedRecording = millis();
      }
      
    }
  }
}

void loop() {
  
  if (camera.recording && 
    (millis() - startedRecording) > recordTime){
    stopRecord();
  }

  delay(1);
  yield();
}
