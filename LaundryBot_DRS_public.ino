// ********************************************************************************
// LaundryBot DRS Firmware
// BY Bin Sun
// Released Under Creative Commons BY-SA License
// 
// Project website: https://www.hackster.io/binsun148/liquid-laundry-detergent-drs-sensor-laundrybot-cefa4b?team=37592
// Submitted for Amazon DRS Developer Challenge
//*********************************************************************************


/*
   It requires VL6180X and LcdBarGraph library, can be found on arduino website.
*/

#include <Wire.h>  // Comes with Arduino IDE
#include <LiquidCrystal.h>
#include <VL6180X.h>

// To try different scaling factors, change the following define.
// Valid scaling factors are 1, 2, or 3.
#define SCALING 2

VL6180X sensor;

#include <LiquidCrystal.h>
#include <LcdBarGraph.h>

byte lcdNumCols = 16; // -- number of columns in the LCD
//byte sensorPin = 0; // -- value for this example

#include <WiFi101.h>
#include <WiFiClient.h>

#include "AmazonDRS.h"
AmazonDRS DRS = AmazonDRS();

//WiFi creds ----------------------------------------------------------------------------------
char ssid[] = ""; //  your network SSID (name)
char pass[] = ""; // your network password (use for WPA, or use as key for WEP)
//------------------------------------------------------------------------------------------------------

int status = WL_IDLE_STATUS;

#define slotNumber 1 //This will vary for multi slot devices - dash buttons typically only serve one product/slot

const int dashButton = 14;     //DIO number of the pushbutton pin
static long buttonHigh = 0;    //millis of last button push for switch debouncing
static String slotStatus = ""; //boolean which depicts if slot is available for replenishment
static String slotId = "a171a5fe-9f6e-457f-ba7d-dd0c48e57c98";     //unique slot id ex: 0a5038b7-7609-4b81-b87e-3e291f386324 

long target = 20.0; //threshold for placing DRS order, in mm

int buttonState;
boolean orderState = true;

LiquidCrystal lcd(0, 1, 2, 3, 4, 5); // -- creating LCD instance
LcdBarGraph lbg(&lcd, lcdNumCols, 0, 1);  // -- creating bargraph instance, format is (&lcd, lcdNumCols, start X, start Y). So (&lcd, 16, 0, 1) would set the bargraph length to 16 columns and start the bargraph at column 0 on row 1.

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  sensor.init();
  sensor.configureDefault();

  // Reduce range max convergence time and ALS integration
  // time to 30 ms and 50 ms, respectively, to allow 10 Hz
  // operation (as suggested by Table 6 ("Interleaved mode
  // limits (10 Hz operation)") in the datasheet).
  sensor.writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 30);
  sensor.writeReg16Bit(VL6180X::SYSALS__INTEGRATION_PERIOD, 50);

   // stop continuous mode if already active
  sensor.stopContinuous();
  // in case stopContinuous() triggered a single-shot
  // measurement, wait for it to complete
  delay(300);
  // start interleaved continuous mode with period of 100 ms
  sensor.startInterleavedContinuous(100);
  
  sensor.setScaling(SCALING);
  sensor.setTimeout(500);

  // -- initializing the LCD
  lcd.begin(2, lcdNumCols);
  lcd.clear();

  // Print a message to the LCD.
  Serial.println("LaundryBot");
  lcd.setCursor(0, 0);
  lcd.print("Filament Bot");
  lcd.setCursor(0, 1);
  lcd.print("for Amazon DRS");
  delay(2000);

  if (WiFi.status() == WL_NO_SHIELD) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("NOT PRESENT");
    return; // don't continue
  }
  
  while ( status != WL_CONNECTED) {
    // Connecting message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi network");
    lcd.setCursor(0, 1);
    lcd.print("Connecting ...");

    // Connect to WPA/WPA2 network.
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(1000);
  }

  printWifiStatus(); // Connected status output

  //initialize slots
  DRS.retrieveSubscriptionInfo();  //check slot statuses

  slotStatus = DRS.getSlotStatus(slotNumber);
  slotId = DRS.getSlotId(slotNumber);

  delay(2000);
  lcd.clear();

}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SSID: ");
  lcd.print(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  lcd.setCursor(0, 1);
  lcd.print("IP: ");
  lcd.print(ip);
}

void loop()
{
  Serial.print("(Scaling = ");
  Serial.print(sensor.getScaling());
  Serial.print("x) ");

  float range=sensor.readRangeSingleMillimeters();

  Serial.print("Range: "); Serial.println(range);
  if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }
  Serial.println();
  
  lcd.setCursor(0, 0);
  lcd.print("Detergent Volume");

  if(range <=295) {lbg.drawValue(295-range, 240);}
  else {lbg.drawValue(0, 255);}

  // -- do some delay: frequent draw may cause broken visualization
  delay(50);

if (buttonPushed())
    {
      
       //Check if slot is available, if so replenish
                
        if(slotStatus == "true")   //if the product in slot are available 
        {
            //we have a match! replenish the products associated with that slot!            
            DRS.requestReplenishmentForSlot(slotId);
        }
        else
        {
          Serial.print("Sorry, slot ");
          Serial.print(slotId);
          Serial.println(" is not available at this time");
          lcd.setCursor(0, 0);
          lcd.print("                ");
          lcd.setCursor(0, 0);
          lcd.print("Sorry, slot ");
          lcd.print(slotId);
          lcd.print(" is");
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1);
          lcd.print("not available");
          delay(1000);       
        }
     }

if (soapLow())
    {
      
       //Check if slot is available, if so replenish
                
        if(slotStatus == "true")   //if the product in slot are available 
        {
            //we have a match! replenish the products associated with that slot!            
            DRS.requestReplenishmentForSlot(slotId);
        }
        else
        {
          Serial.print("Sorry, slot ");
          Serial.print(slotId);
          Serial.println(" is not available at this time");
          lcd.setCursor(0, 0);
          lcd.print("                ");
          lcd.setCursor(0, 0);
          lcd.print("Sorry, slot ");
          lcd.print(slotId);
          lcd.print(" is");
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1);
          lcd.print("not available");
          delay(100);      
        }
    }
 //   delay(1000);  
}

bool buttonPushed(void)
{
  int buttonState = digitalRead(dashButton); 
  
  if(buttonState && ((millis() - buttonHigh) > 5000))
  {
    buttonHigh = millis();
    Serial.println("LaundryBot: Button Pressed!!");
  
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Button Pressed!");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Detergent Ordered!!");
    return true;
    delay(1000);
  }
  else 
  {
    return false;
  }
}

bool SoapLow(void)
{
  if(orderState && ((range - target) > 0.0))
  {
    Serial.println("LaundryBot: Soap Running Low!!");
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Detergent Low!");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Detergent Ordered!!");
    orderState = false;
    return true;
    delay(1000);
  }
  else 
  {
    return false;
  }
}

