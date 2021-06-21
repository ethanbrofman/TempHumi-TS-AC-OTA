// Basic includes
#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <EEPROM.h>
// DHT includes
#include <DHT.h>
#include <DHT_U.h>
// Needed for wifi manager library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// Define DHT hardware
#define DHTPIN 2
#define DHTTYPE DHT22
// Millis config
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long startMillis2;
unsigned long currentMillis;
const unsigned long thingspeak_reporting_period = 60000;  //the value is a number of milliseconds
const unsigned long DHT_reading_period = 5000;
// ThingSpeak config
String apiKey = "MKOJFL0OMOXBNKU6";     //  Enter your Write API key from ThingSpeak
const char* thingspeak_server = "api.thingspeak.com";
// Global variables
float humi = 100;
float temp = 100;
float tempf = 100;
int hi_temp = 60;
int lo_temp = 40;
int hi_humi = 80;
int lo_humi = 50;
// Begin DHT and ESP webserver
DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void read_settings() {
  Serial.println("Reading temp settings");
  if (EEPROM.read(1) != 0) {
    hi_temp = EEPROM.read(1);
  }
  if (EEPROM.read(2) != 0) {
    lo_temp = EEPROM.read(2);
  }
  if (EEPROM.read(3) != 0) {
    hi_humi = EEPROM.read(3);
  }
  if (EEPROM.read(4) != 0) {
    lo_humi = EEPROM.read(4);
  }

  Serial.println(hi_temp);
  Serial.println(lo_temp);
  Serial.println(hi_humi);
  Serial.println(lo_humi);
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

  String index_html = R"=====(
    <!DOCTYPE HTML>
    <html>
    <head>
      <title>ESP Temperature Humidity Sensor</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js"></script>
      <script>
        function submitMessage() { 
          document.getElementById("successMessage").innerHTML = "Saved successfully!";
          updateData();
          setTimeout(function() {
            document.getElementById("successMessage").innerHTML = "";
          }, 2000); // <-- time in milliseconds
        }
        function updateData() {
          const xhttp = new XMLHttpRequest();
          xhttp.onload = function() {
            const objects = JSON.parse(this.responseText);
            if (objects.temp != document.getElementById("current_temp").innerHTML && objects.temp != "") {
              document.getElementById("current_temp").innerHTML = objects.temp;
            }
            if (objects.humi != document.getElementById("current_humi").innerHTML && objects.humi != "") {
              document.getElementById("current_humi").innerHTML = objects.humi;
            }
            if (objects.hi_temp != document.getElementById("hi_temp_setting").innerHTML && objects.hi_temp != "") {
              document.getElementById("hi_temp_setting").innerHTML = objects.hi_temp;
              document.getElementById("hi_temp").value = objects.hi_temp;
            }
            if (objects.lo_temp != document.getElementById("lo_temp_setting").innerHTML && objects.lo_temp != "") {
              document.getElementById("lo_temp_setting").innerHTML = objects.lo_temp;
              document.getElementById("lo_temp").value = objects.lo_temp;
            }
            if (objects.hi_humi != document.getElementById("hi_humi_setting").innerHTML && objects.hi_humi != "") {
              document.getElementById("hi_humi_setting").innerHTML = objects.hi_humi;
              document.getElementById("hi_humi").value = objects.hi_humi;
            }
            if (objects.lo_humi != document.getElementById("lo_humi_setting").innerHTML && objects.hi_humi != "") {
              document.getElementById("lo_humi_setting").innerHTML = objects.lo_humi;
              document.getElementById("lo_humi").value = objects.lo_humi;
            }
          }
          xhttp.open("GET", "/data");
          xhttp.send();
        }
        setInterval(function() {
          updateData(); // this will run after every 5 seconds
        }, 5000);
        updateData(); // This will run on page load
      </script>
    </head>
    <body>
      <div>
        <iframe width="450" height="260" style="border: 1px solid #cccccc;" src="https://thingspeak.com/channels/1244891/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&title=Temperature%28F%29&type=line"></iframe>
        <iframe width="450" height="260" style="border: 1px solid #cccccc;" src="https://thingspeak.com/channels/1244891/charts/2?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&title=Humidity%28%25%29&type=line"></iframe>
      </div>
      <div id="successMessage" style="background-color: green;">
      </div>
      <div>
        <p>Temperature(F): <span id="current_temp"></span></p>
        <p>Humidity(%): <span id="current_humi"></span></p>
      </div>
      <form action="" target="hidden-form">
        HI Temp Setting (current value: <span id="hi_temp_setting"></span>): 
        <input id="hi_temp" type="number" name="hi_temp">
        <br>
        LO Temp Setting (current value <span id="lo_temp_setting"></span>): 
        <input id="lo_temp" type="number" name="lo_temp">
        <br>
        HI Humidity Setting (current value: <span id="hi_humi_setting"></span>): 
        <input id="hi_humi" type="number" name="hi_humi">
        <br>
        LO Humidity Setting (current value: <span id="lo_humi_setting"></span>): 
        <input id="lo_humi" type="number" name="lo_humi">
        <br>
        <input type="submit" value="Submit" onclick="submitMessage()">
      </form>
      <iframe style="display:none" name="hidden-form">
      </iframe>
    </body>
    </html>
  )=====";

  server.send(200, "text/html", index_html);
}

void sendData() {
  Serial.println("GET request received /data");
  String json_data = "{\"temp\":\"";
  json_data += String(tempf);
  json_data += "\", \"humi\":\"";
  json_data += String(humi);
  json_data += "\", \"hi_temp\":\"";
  json_data += String(hi_temp);
  json_data += "\", \"lo_temp\":\"";
  json_data += String(lo_temp);
  json_data += "\", \"hi_humi\":\"";
  json_data += String(hi_humi);
  json_data += "\", \"lo_humi\":\"";
  json_data += String(lo_humi);
  json_data += "\"}";
  server.send(200, "application/json", json_data);
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
      hi_temp = server.arg(i).toInt();
      EEPROM.write(1, server.arg(i).toInt());
    }
    if (server.argName(i) == "lo_temp") {
      String arg_val = server.arg(i) + "\n";
      lo_temp = server.arg(i).toInt();
      EEPROM.write(2, server.arg(i).toInt());
    }
    if (server.argName(i) == "hi_humi") {
      String arg_val = server.arg(i) + "\n";
      hi_humi = server.arg(i).toInt();
      EEPROM.write(3, server.arg(i).toInt());
    }
    if (server.argName(i) == "lo_humi") {
      String arg_val = server.arg(i) + "\n";
      lo_humi = server.arg(i).toInt();
      EEPROM.write(4, server.arg(i).toInt());
    }    
  }
}

void one_page() {
  getParams();
  handleRoot();
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
  server.on("/", one_page);
  // server.on("/get", getParams);
  server.on("/data", sendData);
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
  if (currentMillis - startMillis >= thingspeak_reporting_period)  //test whether the period has elapsed
  {
    tell_thingspeak(); 
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
  if (currentMillis - startMillis2 >= DHT_reading_period)  //test whether the period has elapsed
  {
    read_DHT(); 
    startMillis2 = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
  
}
