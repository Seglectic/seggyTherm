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




/********************************************************************************
 *  
 *                                 Libraries 📚
 *  
 ********************************************************************************/
#include <Arduino.h>             // Arduino library
#include <Wire.h>                // Required for i²C comms
#include <Adafruit_SSD1306.h>    // SSD1306 display driver
#include <Adafruit_GFX.h>        // GFX Library for drawing to display
#include "DHTesp.h"              // Library for DHT22 temp / humidity sensor



/********************************************************************************
 * 
 *                                 I/O Setup 📺
 *     
 ********************************************************************************/

#define SCREEN_WIDTH   128                                                    // OLED display width, in pixels
#define SCREEN_HEIGHT  64                                                     // OLED display height, in pixels
#define OLED_RESET     LED_BUILTIN                                            // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);     // Initialize display object as 'display'
DHTesp dht;                                                                   // Initialize temp sensor as 'dht'


// Setup button code
const int buttonPin = D5;
bool buttonFlag = 0;                                                          //Has button been pushed? Queue action if so
int buttonCounter = 0;

// Button Interrupt Service Routine
void ICACHE_RAM_ATTR buttonISR() {                                            // ICACHE_RAM_ATTR caches this function into 
  buttonFlag = true;                                                          // ESP RAM at runtime. Keep compact.
}

//Check for button input
bool handleButton(){
  if(digitalRead(buttonPin)==LOW){
    return false;
  }else{
    return true;
  }
}

// Determines what will appear on screen: 0 Local temp sensor | 1 Remote temp sensor | 2 Server / wifi stats? |
int displayMode = 0;


/********************************************************************************
 *  
 *                                 Main Setup 🚧
 *  
 ********************************************************************************/

void setup() {
  Serial.begin(115200);

  dht.setup(D4, DHTesp::DHT22);         //Setup dht as a DHT22
  pinMode(buttonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(buttonPin),buttonISR,RISING); // Setup button interrupt 

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
  delay(500);
  
// iono
  // display.clearDisplay();
  // display.setCursor(0,0);
  // display.setTextSize(1); 
  // display.println("WiFi Connected.");
  // display.println("\n");
  // display.setTextSize(1); 
  
  display.display();
  // delay(800);
  display.clearDisplay();
  display.display();
       
}

void localTemps(){

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
    delay(500); 
  }


}





/********************************************************************************
 * 
 *                                 Main Loop ➰
 *  
 ********************************************************************************/

void loop() {

  display.clearDisplay();

  if( displayMode==0 ){ localTemps(); }
  if( displayMode==1 ){ display.clearDisplay(); display.display(); }
  
  //Bottom Line
  // display.drawLine(0,5,128,53,WHITE);          //Draw fancy line above bottom
  // display.fillRect(0,55,128,9,WHITE);           //Fill bottom line with white 
  // display.setTextSize(1);         
  // display.setTextColor(BLACK);                  //Invert font for bottom line
  // display.setCursor(1,56);                       //Display raw value from Sensor
  // display.setCursor(64,56);
  // Serial.println(handleButton());




  
   

  // Was the button pushed? Cycle mode if so.
  if(buttonFlag){
    if(displayMode<1){
      displayMode++;
    }else{
      displayMode=0;
    }
    buttonFlag=false;
  }

  display.display();
}