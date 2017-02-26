#include <Wire.h>
#include <VL6180X.h>

// To try different scaling factors, change the following define.
// Valid scaling factors are 1, 2, or 3.
#define SCALING 2

VL6180X sensor;

#include <LiquidCrystal.h>
#include <LcdBarGraph.h>

byte lcdNumCols = 16; // -- number of columns in the LCD
//byte sensorPin = 0; // -- value for this example

//LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // -- creating LCD instance
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);
LcdBarGraph lbg(&lcd, lcdNumCols, 0, 1);  // -- creating bargraph instance, format is (&lcd, lcdNumCols, start X, start Y). So (&lcd, 16, 0, 1) would set the bargraph length to 16 columns and start the bargraph at column 0 on row 1.

void setup(){
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

  sensor.setTimeout(500);

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
  // -- do some delay some time I've got broken visualization
  //delay(100);

}

void loop()
{
  Serial.print("(Scaling = ");
  Serial.print(sensor.getScaling());
  Serial.print("x) ");

  float range=sensor.readRangeSingleMillimeters();

  //Serial.print(sensor.readRangeSingleMillimeters());
  //if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }
    Serial.print("Range: "); Serial.println(range);
  if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }
  Serial.println();
  
     lcd.setCursor(0, 0);
    lcd.print("Detergent Volume");
 //       lcd.clear();
  // -- draw bar graph from the analog value readed
  //lbg.drawValue( analogRead(sensorPin), 1024);
  if(range <=295) {lbg.drawValue(295-range, 240);}
  else {lbg.drawValue(0, 255);}
  //lbg.drawValue(295-range, 255);
  // -- do some delay: frequent draw may cause broken visualization
  delay(50);
}
