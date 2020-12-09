#include <Arduino.h>
#include <FS.h>
  
#include <DHT.h>
#include <DHT_U.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define DHTPIN 2
#define DHTTYPE DHT22

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long reading_period = 60000;  //the value is a number of milliseconds

float humi = 100;
float temp = 100;
float tempf = 100;
float hi_temp = 60;
float lo_temp = 40;
float hi_humi = 80;
float lo_humi = 50;

String apiKey = "MKOJFL0OMOXBNKU6";     //  Enter your Write API key from ThingSpeak
const char* thingspeak_server = "api.thingspeak.com";

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);

  if(var == "hi_temp"){
    return readFile(SPIFFS, "/hi_temp.txt");
  }
  else if(var == "lo_temp"){
    return readFile(SPIFFS, "/lo_temp.txt");
  }
  else if(var == "hi_humi"){
    return readFile(SPIFFS, "/hi_humi.txt");
  }
  else if(var == "lo_humi"){
    return readFile(SPIFFS, "/lo_humi.txt");
  }
  return String();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

 ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  dht.begin();
  startMillis = millis();  //initial start time
  server.on("/", handleRoot);
  server.on("/get", getParams);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  read_DHT();
}

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  server.handleClient();
  MDNS.update();
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= reading_period)  //test whether the period has elapsed
  {
    if (read_DHT()) {
      tell_thingspeak();
    } 
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
  
}

void read_settings() {
  hi_temp = readFile(SPIFFS, "/hi_temp.txt").toFloat();
  lo_temp = readFile(SPIFFS, "/lo_temp.txt").toFloat();
  hi_humi = readFile(SPIFFS, "/hi_humi.txt").toFloat();
  lo_humi = readFile(SPIFFS, "/lo_humi.txt").toFloat();

}

bool read_DHT() {
  // Wait a few seconds between measurements.

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humi = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  tempf = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humi) || isnan(temp) || isnan(tempf)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return false;
  }

  Serial.println("TempF: "+String(tempf));
  Serial.println("Humid: "+String(humi));
  return true;
}

void tell_thingspeak() {
  WiFiClient client;

  if (client.connect(thingspeak_server,80))   //   "184.106.153.149" or api.thingspeak.com
  {  
    String sendData = apiKey+"&field1="+String(tempf)+"&field2="+String(humi)+"\r\n\r\n"; 
       
    // Serial.println(sendData);
    Serial.println("Connecting to Thingspeak.");

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(sendData.length());
    client.print("\n\n");
    client.print(sendData);

    // Serial.print("Temperature: ");
    // Serial.print(tempf);
    // Serial.print("deg F. Humidity: ");
    // Serial.print(humi);
  }
      
  client.stop();

  Serial.println("Sent.");  
}

void handleRoot() {

  read_settings();

  String index_html = "<!DOCTYPE HTML><html><head><title>ESP Temperature Humidity Sensor</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  index_html += "<script>function submitMessage() { alert(\"Saved value to ESP\"); setTimeout(function(){ document.location.reload(false); }, 500);}</script></head><body>";
  index_html += "<div><iframe width=\"450\" height=\"260\" style=\"border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/1244891/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&title=Temperature%28F%29&type=line\"></iframe>";
  index_html += "<iframe width=\"450\" height=\"260\" style=\"border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/1244891/charts/2?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&title=Humidity%28%25%29&type=line\"></iframe></div>";
  index_html += "<div><p>Temperature(F):" + String(tempf) + "</p><p>Humidity(%): " + String(humi) + "</p></div>";
  index_html += "<form action=\"/get\" target=\"hidden-form\">HI Temp Setting (current value: " + String(hi_temp) + "): <input type=\"number \" name=\"hi_temp\">";
  index_html += "<input type=\"submit\" value=\"Submit\" onclick=\"submitMessage()\"></form><br>";
  index_html += "<form action=\"/get\" target=\"hidden-form\">LO Temp Setting (current value " + String(lo_temp) + "): <input type=\"number \" name=\"lo_temp\">";
  index_html += "<input type=\"submit\" value=\"Submit\" onclick=\"submitMessage()\"></form><br>";
  index_html += "<form action=\"/get\" target=\"hidden-form\">HI Humidity Setting (current value " + String(hi_humi) + "): <input type=\"number \" name=\"hi_humi\">";
  index_html += "<input type=\"submit\" value=\"Submit\" onclick=\"submitMessage()\"></form><br>";
  index_html += "<form action=\"/get\" target=\"hidden-form\">LO Humidity Setting (current value " + String(lo_humi) + "): <input type=\"number \" name=\"lo_humi\">";
  index_html += "<input type=\"submit\" value=\"Submit\" onclick=\"submitMessage()\"></form>";
  index_html += "<iframe style=\"display:none\" name=\"hidden-form\"></iframe></body></html>";

  server.send(200, "text/html", index_html);
}

void handleNotFound() {
  String message = "Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void getParams() {
  Serial.println("GET request received with argument");
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.println(server.argName(i) + ": " + server.arg(i) + "\n");
    if (server.argName(i) == "hi_temp") {
      String arg_val = server.arg(i) + "\n";
      writeFile(SPIFFS, "/hi_temp.txt", arg_val.c_str());
    }
    if (server.argName(i) == "lo_temp") {
      String arg_val = server.arg(i) + "\n";
      writeFile(SPIFFS, "/lo_temp.txt", arg_val.c_str());
    }
    if (server.argName(i) == "hi_humi") {
      String arg_val = server.arg(i) + "\n";
      writeFile(SPIFFS, "/hi_humi.txt", arg_val.c_str());
    }
    if (server.argName(i) == "lo_humi") {
      String arg_val = server.arg(i) + "\n";
      writeFile(SPIFFS, "/lo_humi.txt", arg_val.c_str());
    }    
  }
      
}
