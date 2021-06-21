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
int humi = 100;
int temp = 100;
int tempf = 100;
int hi_temp = 60;
int lo_temp = 40;
int hi_humi = 80;
int lo_humi = 50;
bool ac_relay = false;
bool heat_relay = false;
bool dehumidifier_relay = false;
bool humidifier_relay = false; 
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
  humi = round(dht.readHumidity());
  // Read temperature as Celsius (the default)
  temp = round(dht.readTemperature());
  // Read temperature as Fahrenheit (isFahrenheit = true)
  tempf = round(dht.readTemperature(true));

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
          updateData();
          setTimeout(function() {
            toggleSettings();
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
            if (objects.ac_relay == "1") { 
              document.getElementById("ac_relay").style.display = "inline";
            } else {
              document.getElementById("ac_relay").style.display = "none";
            }
            if (objects.heat_relay == "1") {
              document.getElementById("heat_relay").style.display = "inline";
            } else {
              document.getElementById("heat_relay").style.display = "none";
            }
            if (objects.dehumidifier_relay == "1") {
              document.getElementById("dehumidifier_relay").style.display = "inline";
            } else {
              document.getElementById("dehumidifier_relay").style.display = "none";
            }
            if (objects.humidifier_relay == "1") {
              document.getElementById("humidifier_relay").style.display = "inline";
            } else {
              document.getElementById("humidifier_relay").style.display = "none";
            }

          }
          xhttp.open("GET", "/data");
          xhttp.send();
        }
        setInterval(function() {
          updateData(); // this will run after every 5 seconds
        }, 5000);
        function toggleSettings() {
          if (document.getElementById("settings").style.display == "none") {
            document.getElementById("settings").style.display = "block";
            document.getElementById("settings_button").style.display = "none";
          } else {
            document.getElementById("settings").style.display = "none";
            document.getElementById("settings_button").style.display = "block";
          }
        }
        function toggleStats() {
          if (document.getElementById("stats").style.display == "none") {
            document.getElementById("stats").style.display = "block";
            document.getElementById("stats_button").style.display = "none";
          } else {
            document.getElementById("stats").style.display = "none";
            document.getElementById("stats_button").style.display = "block";
          }
        }
        updateData(); // This will run on page load
      </script>
      <style>
      body {
        font-family: Arial, sans-serif;
      }
      .statusbar {
        padding: 5px 5px 5px 5px;
        margin: 5px 5px 5px 5px;
        background-color: darkgray;
        border-radius: 5px;
        color: white;
      }
      .status-card {
        margin: auto;
        padding: 5px 5px 5px 5px;
        margin: 5px 5px 5px 5px;
        background-color: lightgray;
        border: 1px gray;
        border-radius: 5px;
        text-align: center;
      }
      .btn {
        width: 50%;
        padding: 10px 10px 10px 10px;
        margin: 10px 10px 10px 10px;
        font-size: 20px;
        font-style: bold;
        border-color: lightgray;
        color: gray;
        border-radius: 5px;
      }
      input {
        width: 50%;
        padding: 10px 10px 10px 10px;
        margin: 10px 10px 10px 10px;
        font-size: 20px;
        font-style: bold;
      }
      .btn:hover {
        color: gray;
        border-color: black;
        border-radius: 5px;
      }
      .close-btn {
        font-size: 25px;
        float: right;
        font-style: bold;
      }
      .val_input {
        width: 50px;
      }
      .details {
        font-style: italic;
        font-size: 12px;
        margin: 10px 10px 10px 10px;
        color: gray;
      }
      h1 {
        font-size: 60px;
        margin: 10px 10px 10px 10px;
        background-color: darkgray;
        border-radius: 5px;
        color: white;
      }
      </style>
    </head>
    <body>
      <div>
        <div class="status-card temphumi">
          <span class="details">Temperature</span>
          <h1><span id="current_temp"></span>&#8457;</h1>
          <span class="details">High Setting: <span id="hi_temp_setting"></span>&#8457;</span>
          <span class="details">Low Setting: <span id="lo_temp_setting"></span>&#8457;</span>
        </div>
        <div class="status-card temphumi">
          <span class="details">Humidity</span>
          <h1><span id="current_humi"></span>%</h1>
          <span class="details">High Setting: <span id="hi_humi_setting"></span>%</span>
          <span class="details">Low Setting: <span id="lo_humi_setting"></span>%</span>
        </div>
        <div class="status-card">
          <span class="details">Status</span>
          <p>
            <span id="ac_relay" class="statusbar" style="display:none;">A/C</span>
            <span id="heat_relay" class="statusbar" style="display:none;">HEAT</span>
            <span id="dehumidifier_relay" class="statusbar" style="display:none;">DEHUMIDIFIER</span>
            <span id="humidifier_relay" class="statusbar" style="display:none;">HUMIDIFIER</span>
          </p>
        </div>
        <div class="status-card">
          <div id="settings_button" style="display:block;">
            <button class="btn" onclick="toggleSettings();">Settings</button>
          </div>
          <div id="settings" style="display:none;">
            <button class="close-btn" onclick="toggleSettings();">X</button>
            <span class="details">Settings</span>
            <form action="" target="hidden-form">
              <table style="width:100%;">
                <tr>
                  <td>High Temp Setting</td>
                  <td><input id="hi_temp" type="number" class="val_input" name="hi_temp" maxlength="3" min="0" max="120"></td>
                </tr>
                <tr>
                  <td>Low Temp Setting</td>
                  <td><input id="lo_temp" type="number" class="val_input" name="lo_temp" maxlength="3" min="0" max="120"></td>
                </tr>
                <tr>
                  <td>High Humidity Setting</td>
                  <td><input id="hi_humi" type="number" class="val_input" name="hi_humi" maxlength="3" min="0" max="100"></td>
                </tr>
                <tr>
                  <td>Low Humidity Setting</td> 
                  <td><input id="lo_humi" type="number" class="val_input" name="lo_humi" maxlength="3" min="0" max="100"></td>
                </tr>
              </table>
              <input type="submit" value="Submit" class="btn" onclick="submitMessage()">
              <button class="btn" style="display:none;" onclick="toggleSettings();">Close Settings</button>
              </div>
            </form>
            </div>
          </div>
          <div class="status-card">
            <div id="stats_button" style="display:block;">
              <button class="btn" onclick="toggleStats();">Statistics</button>
            </div>        
            <div id="stats" style="display:none;">
              <button class="close-btn" onclick="toggleStats();">X</button>
              <span class="details">Statistics</span><br>
              <iframe width="450" height="260" style="border: 1px solid #cccccc;" src="https://thingspeak.com/channels/1244891/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&title=Temperature%28F%29&type=line"></iframe>
              <iframe width="450" height="260" style="border: 1px solid #cccccc;" src="https://thingspeak.com/channels/1244891/charts/2?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&title=Humidity%28%25%29&type=line"></iframe>
              <br>
              <button class="btn" style="display:none;" onclick="toggleStats();">Close Statistics</button>
            </div>
          </div>
      </div>
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

  json_data += "\", \"ac_relay\":\"";
  json_data += String(ac_relay);
  json_data += "\", \"heat_relay\":\"";
  json_data += String(heat_relay);
  json_data += "\", \"dehumidifier_relay\":\"";
  json_data += String(dehumidifier_relay);
  json_data += "\", \"humidifier_relay\":\"";
  json_data += String(humidifier_relay);
  
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

void check_relays() {
  if (tempf > hi_temp) {
    ac_relay = true;
  } else {
    ac_relay = false;
  }
  if (tempf < lo_temp) {
    heat_relay = true;
  } else {
    heat_relay = false;
  }
  if (humi > hi_humi) {
    dehumidifier_relay = true;
  } else {
    dehumidifier_relay = false;
  }
  if (humi < lo_humi) {
    humidifier_relay = true;
  } else {
    humidifier_relay = false;
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
  check_relays();
  
}
