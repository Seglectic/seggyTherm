/********************************************************************************
 * 
 *                                 seggyTherm
 *                  
 *                  Displays temperature from a DHT22 sensor
 *                  on SSD1306 128x64 display as well as 
 *                  web interface to display graphs of locally
 *                  logged data.
 * 
 *                  ToDo: optionally connect to remote ESPs or
 *                  report to a centralized server for more nodes.
 *                   
 *                                   - Seglectic Softworks 2020
 *             
 ********************************************************************************/



/****************************************************************************************************************************************************************
 *  
 *                                                          Libraries 📚
 *  
 ****************************************************************************************************************************************************************/ 
#include <Arduino.h>
#include <DHTesp.h>                // Library for DHT22 temp / humidity sensor
#include <Adafruit_SSD1306.h>      // SSD1306 display driver
#include <Wire.h>                  // Required for i²C comms
#include <Adafruit_GFX.h>          // GFX Library for drawing to display
#include <FS.h>                    // SPIFFS File system https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
#include <ESP8266WiFi.h>           // Main WiFi library https://github.com/esp8266/Arduino
#include <WiFiManager.h>           // WiFi Manager for captive library and autoconnect https://github.com/tzapu/WiFiManager
#include <DNSServer.h>             // ESP8266 DNS required for captive portal
#include <ESP8266WebServer.h>      // Main web server library
#include <WiFiUDP.h>               // UDP required for NTP & OTA updates
#include <NTPClient.h>             // Network Time Protocol client
#include <ArduinoOTA.h>            // Allows updating firmware wirelessly https://docs.platformio.org/en/latest/platforms/espressif8266.html#over-the-air-ota-update




/****************************************************************************************************************************************************************
 *  
 *                                                          I/O Setup 📺
 *  
 ****************************************************************************************************************************************************************/ 

#define SCREEN_WIDTH   128                                                    // OLED display width, in pixels
#define SCREEN_HEIGHT  64                                                     // OLED display height, in pixels
#define OLED_RESET     LED_BUILTIN                                            // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);     // Initialize display object as 'display'
DHTesp dht;                                                                   // Initialize temp sensor as 'dht'

String Version = "0.00.000.000.00.000.1a";

int saveTimer = 0;
int saveInterval = 60000;
int lastMillis = 0;

/****************************************************************************************************************************************************************
 *  
 *                                                          NTP Setup ⌚
 *  
 ****************************************************************************************************************************************************************/ 

WiFiUDP ntpUDP;                                                     // Configure NTP Client with -14400 seconds                                     
NTPClient timeClient(ntpUDP,"pool.ntp.org",0,86400000);             // offset for GMT-5 and 24h update interval (8.64e+7ms)
// NTPClient timeClient(ntpUDP,"pool.ntp.org",-14400,60);           // offset for GMT-5 and 60 second update interval



/****************************************************************************************************************************************************************
 *  
 *                                                          Web Server File Handling 📂
 *  
 ****************************************************************************************************************************************************************/ 

ESP8266WebServer server(80);                             //Create web server


 /* * * * * * * * * * * * * * * * * * * * * *
 *             getContentType()         
 *            
 *    Function to return 'Content-Type'
 *    based on file extension for
 *    proper client formatting
 * 
 *  todo: replace this with switchcase
 *  https://www.arduino.cc/reference/en/language/structure/control-structure/switchcase/
 * 
  * * * * * * * * * * * * * * * * * * * * */
String getContentType(String filename) { 
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}


 /* * * * * * * * * * * * * * * * * * * * * *
 *            handleFileRead()
 *             
 *    Sends the right file to the client
 *    and sends the file else returns false
 * 
  * * * * * * * * * * * * * * * * * * * * */

bool handleFileRead(String path) { 
  Serial.println("Serving file: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file (LOCAL LINKS TO FOLDERS MUST END WITH / )
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    server.streamFile(file, contentType);               // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  return false;                                         // If the file doesn't exist, return false
}


/****************************************************************************************************************************************************************
 * 
 *                                                        Web Response Handlers 👋
 *     
 ****************************************************************************************************************************************************************/ 


 /* * * * * * * * * * * * * * * * * * * * *
 *             handleGeneric()
 *             
 *     Example response handler with a
 *  sample parameter check and response

  * * * * * * * * * * * * * * * * * * * * */
void handleGeneric() { 
  String response = "Number of Parameters: ";                     // Create response string to send
  response += server.args();                                      // Get number of parameters
  response += "\n";                                               // New line (<br> for html response)
  for (int i = 0; i < server.args(); i++) {  
    response += "Arg #" + (String)i + " -> ";                     // Include the current iteration value
    response += server.argName(i) + ": ";                         // Get the name of the parameter
    response += server.arg(i) + "\n";                             // Get the value of the parameter
  } 
  if(server.arg("blep")){                                         // Check if parameter exists
    response+="Client sent a blep parameter!!!\n";
  }
  server.send(200, "text/plain", response);                       // Respond to the HTTP request with plaintext
}


void getVersion(){
  server.send(200,"text/plain",Version);
}

// Send temperature and humidity data
void getTemp(){

  float tempC = dht.getTemperature();
  float tempF = tempC*1.8+32;
  float humidity = dht.getHumidity();
  String response = "";
  response += String(tempF) + " ";
  response += String(humidity);

  server.send(200,"text/plain",response);
}






/****************************************************************************************************************************************************************
 *  
 *                                                          Main Setup 🚧
 *  
 ****************************************************************************************************************************************************************/ 

void setup() {
  Serial.begin(115200);

  dht.setup(D4, DHTesp::DHT22);         //Setup dht as a DHT22
  pinMode(D4,INPUT_PULLUP);


  // float tempC = dht.getTemperature();
  // float tempF = tempC*1.8+32;

 /* * * * * * * * * * * *
  *    Display Setup
  * * * * * * * * * * * */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);      // "1:1 pixel scale" 
  display.clearDisplay();
  display.setTextColor(WHITE); //Defaults to black
  //Lil Splash
  display.setCursor(0,0);
  display.setTextSize(2);
  display.println(F("seggyTherm"));
  display.setTextSize(1);
  display.println("\n     Now booting.\n\n");
  display.println(F("Seglectic Softworks")); 
  display.println(F("                -2020"));
  display.display();
  
  display.display();
  // delay(800);
  display.clearDisplay();
  display.display();
       

 /* * * * * * * * * * * *
  *   WifiManager Setup
  * * * * * * * * * * * */   

  WiFiManager wifiManager;                               //Setup WiFi Manager
  wifiManager.autoConnect("segSetup","seglectic");       //Autoconnect; else start portal with this SSID, P/W

  server.on("/generic",handleGeneric);
  server.on("/version",getVersion   );
  server.on("/temp",getTemp         );

  //Serves file to client
  server.onNotFound([]() {                                        // If the client requests any URI
    if (!handleFileRead(server.uri()))                            // send it if it exists
      server.send(404, "text/plain", "404'd");                    // otherwise, respond with a 404 (Not Found) error
  });


 /* * * * * * * * * * * *
  *   Start the things
  * * * * * * * * * * * */   
  server.begin();                                                 // Starts file server
  SPIFFS.begin();                                                 // Starts local file system
  timeClient.begin();                                             // Starts NTP client

Serial.println(timeClient.getFormattedTime());
}


 /* * * * * * * * * * * * * * * * * * * * *
 *             localTemp()
 *             
 *     Displays Local temps to screen
  * * * * * * * * * * * * * * * * * * * * */
void localTemps(){
  display.clearDisplay();
  // Get data from sensors
  float tempC = dht.getTemperature();
  float tempF = tempC*1.8+32;
  float humidity = dht.getHumidity();
  
  // display.fillRect(0,0,128,53,BLACK);           // Clear top area (x,y,w,h,clr)
  display.setTextSize(3);         
  display.setTextColor(WHITE);                  
  display.setCursor(0,17);
 
  if(isnan(tempC)){
    display.setTextSize(2);
    display.setCursor(5,2);
    display.println("SENSOR\n ERROR");
    display.drawRect(70,33,50,31,WHITE);
    display.drawPixel(random(70,120),random(33,64),WHITE);
    delay(5); 
  }else{
    //display.print(int(round(tempF)));
    display.print(int(round(tempF)));
    display.println("F ");
    display.print(int(round(humidity)));
    display.print("%");

    //Print smaller secondary unit display
    display.setCursor(90,17);
    display.setTextSize(2);
    display.print(int(round(tempC)));
    display.print("C");

    int TY = map(tempF,50,100,0,128);
    display.fillRect(0,0,TY,14,WHITE);
  }
  display.display();
}



 /* * * * * * * * * * * * * * * * * * * * *
 *             logData()
 *             
 *     Logs temp (metric) & humidity data
 *  to weather.txt file for reading from browser
  * * * * * * * * * * * * * * * * * * * * */

 bool logData(){
   int m = millis();
   saveTimer+= m-lastMillis;
   lastMillis = m;

   if(saveTimer>saveInterval){
    saveTimer=0;
    File saveFile = SPIFFS.open("/weather.txt","a");                  // Open save file in append mode
    if(!saveFile){                                                    
      Serial.println("failed to open save file");                     // Check if file opened
      return false;
    }else{
      String saveData = String(timeClient.getEpochTime()) +"|"+ String(dht.getTemperature()) + "|" + String(dht.getHumidity());
      saveFile.println( saveData );                                   // Save color in data param
      saveFile.close(); 
      Serial.println("Saved me some data.");
    }
   }else{
     return false;
   }
 }




/****************************************************************************************************************************************************************
 *  
 *                                                          Main Loop ➰
 *  
 ****************************************************************************************************************************************************************/ 

void loop() {
  timeClient.update();
  localTemps();  
  logData();
  server.handleClient();
  
}