// ********************************************************************************
// FilamentBot Firmware
// BY Bin Sun
// Released Under Creative Commons BY-SA License
// 
// Project website: https://www.hackster.io/binsun148/smart-3d-printer-filament-counter-filamentbot-383ac8?team=32053
// Submitted for Amazon DRS Developer Challenge
//*********************************************************************************


/*
   this is a modification to the original ps2 mouse sketch, it requires ps2 library,
   can be found on arduino website.
   an arduino sketch to interface with a   mouse ( optical, ball, ps2 and some usb).
   it measures the distance the mouse travels.
   a button on pin 7 is used to zero the reading.
   one side of the button is connected to pin 7 other to ground.
   it writes the results to a serial display.
*/

/*
   Pin 9 is the mouse data pin, pin 8 is the clock pin
   Feel free to use whatever pins are convenient.
*/

#include <Wire.h>  // Comes with Arduino IDE
#include <ps2.h>
//#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>

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
static String slotId = "0a5038b7-7609-4b81-b87e-3e291f386324";     //unique slot id ex: 0a5038b7-7609-4b81-b87e-3e291f386324 

PS2 mouse(8, 9);
int val = 0 ;// variable for reading the pin
long newmx = 0;
long newmy = 0;
long total = 0;
long target = 100.0; //threshold for placing DRS order, in mm

const int zeroPin = 7; // set zeroPin pin number
boolean direct = true;

int buttonState;
boolean orderState = true;

unsigned long timePress = 0;
unsigned long timePressLimit = 0;
int clicks = 0;

//float cal = 2.5; //calibratino factor to convert mouse movement to milemeter
float cal = 250;

LiquidCrystal lcd(0, 1, 2, 3, 4, 5);

/*
   initialize the mouse. Reset it, and place it into remote
   mode, so we can get the encoder data on demand.
*/

void mouse_init()
{
  mouse.write(0xff);  // reset
  mouse.read();  // ack byte
  mouse.read();  // blank */
  mouse.read();  // blank */
  mouse.write(0xf0);  // remote mode
  mouse.read();  // ack
  delayMicroseconds(100);
}

void setup()
{
  pinMode(zeroPin, INPUT_PULLUP);
  pinMode(led, OUTPUT);

  Serial.begin(115200);

  lcd.clear();
  lcd.begin(16, 2); // set up the LCD's number of columns and rows:

  // Print a message to the LCD.
  Serial.println("FilamentBot");
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
  checkDir();
  mouse_init();
}

void checkDir() {
  lcd.setCursor(0, 0);
  lcd.print("Change Direction?");
  lcd.setCursor(0, 1);
  lcd.print("Hold RESET 4 YES");
  delay(2000);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  boolean resetPressed = false;
  int countdown = 3;
  while (!resetPressed && countdown >= 0) {
    val = digitalRead(zeroPin);
    if (val == LOW) {
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Direct. Changed");
      delay(1000);
      resetPressed = true;
      direct = false;
      lcd.clear();
    }
    lcd.setCursor(0, 2);
    lcd.print("Countdown:   ");
    lcd.setCursor(0, 2);
    lcd.print("Countdown: ");
    lcd.print(countdown);
    countdown--;
    if (countdown < 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
    }
    if (resetPressed) {
      lcd.clear();
      lcd.setCursor(0, 0);
    }
    delay(1000);
  }
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

/*
  get a reading from the mouse and report it back to the
  host via the serial line.
*/
void loop()
{

  char mstat;
  char mx;
  char my;

  /* get a reading from the mouse */
  mouse.write(0xeb);  // give me data!
  mouse.read();      // ignore ack
  mstat = mouse.read();
  mx = mouse.read();
  my = mouse.read();

  //reverse direction
  if (direct) {
    mx = mx * -1;
  }

  total =  total + mx; //calculate total
  newmx = newmx + mx;

  buttonState = digitalRead(zeroPin);

  if (buttonState == LOW) {
    delay(200);
    //    Serial.println("Button Pressed");

    if (clicks == 0) {
      timePress = millis();
      timePressLimit = timePress + 500;
      clicks = 1;
    }

    else if (clicks == 1 && millis() < timePressLimit) {
      //      Serial.println("Button Pressed twice");
      newmx = 0;
      total = 0;
      timePress = 0;
      timePressLimit = 0;
      clicks = 0;
      orderState = true;
    }


    if (clicks == 1 && timePressLimit != 0 && millis() > timePressLimit) {
      //     Serial.println("Button Pressed Once");
      timePress = 0;
      timePressLimit = 0;
      clicks = 0;

      newmx = 0;


    }
  }

   lcd.setCursor(0, 0);
  if(orderState){lcd.print("                ");}else{lcd.print("               D");}
  lcd.setCursor(0, 0);
  lcd.print("U~");
  lcd.print(newmx / (cal));
  lcd.print(" mm");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("T~");
  lcd.print(total / (cal));
  lcd.print(" mm");

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

if (filamentLow())
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
    Serial.println("FilamentBot: Button Pressed!!");
  
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Button Pressed!");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Filament Ordered!!");
    return true;
    delay(1000);
  }
  else 
  {
    return false;
  }
}

bool filamentLow(void)
{
  if(orderState && (((total / (cal)) - target) > 0.0))
  {
    Serial.println("FilamentBot: Filament Running Low!!");
      lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Filament Low!");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Filament Ordered!!");
    orderState = false;
    return true;
    delay(1000);
  }
  else 
  {
    return false;
  }
}

