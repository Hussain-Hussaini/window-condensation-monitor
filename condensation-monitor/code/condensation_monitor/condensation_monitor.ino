// Window Condensation Monitor
// Arduino Uno R3. Measures air temp and glass temp (two DS18B20 probes) and humidity (DHT11),
// calculates the dew point, and reports SAFE / WARNING / CONDENSING / SENSOR ERROR.
// Needs: OneWire, DallasTemperature (Library Manager), LiquidCrystal (built in),
// and dht_nonblocking.h / dht_nonblocking.cpp in this sketch folder (from the Elegoo UNO kit tutorial files).

#include<OneWire.h>
#include<DallasTemperature.h>
OneWire onewire (3);
DallasTemperature sensors (&onewire);
#include "dht_nonblocking.h"
#define DHT_SENSOR_TYPE DHT_TYPE_11
#include <LiquidCrystal.h>
LiquidCrystal lcd ( 7, 8, 9, 10, 11, 12);




static const int DHT_SENSOR_PIN = 2;
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );






const float WARN_MARGIN = 3.0;


float lastGoodGlass = 0;
bool haveGlass = false;
int badCountGlass = 0;


float lastGoodAir = 0;
bool haveAir = false;
int badCountAir = 0;


float humidityNow = 0;
float dewPointNow = 0;
float marginNow = 0;
int statusNow = 0;


int page = 0;
int lastButton = HIGH;
unsigned long lastPressTime = 0;


bool needRedraw = true;


bool waitingForFirst = true;


unsigned long lastBlink = 0;
bool blinkState = false;




void setLed(int red, int green) {
 analogWrite(5, red);
 analogWrite(6, green);
}


void updateAlert() {
 if (millis() - lastBlink > 500) {
   lastBlink = millis();
   blinkState = !blinkState;
 }


 if (waitingForFirst == true) {
   setLed(0,0);
   digitalWrite(4,LOW);
 } else if (statusNow == 0) {
   digitalWrite(4,LOW);
   setLed(0,255);
  }else if (statusNow == 1) {
   digitalWrite(4,LOW);
   setLed(255,80);
  } else if(statusNow == 2){
   setLed(255,0);
   digitalWrite(4,blinkState);
 } else if (statusNow == 3) {
 if (blinkState == true) {
   setLed(255,0);
 } else {
   setLed(0,0);
 }
 digitalWrite(4,LOW);
 }
 }




void setup() {
// put your setup code here, to run once:
Serial.begin (9600);
sensors.begin();
pinMode(A0, INPUT_PULLUP);
lcd.begin(16,2);
pinMode(4,OUTPUT);
if (waitingForFirst == true ) {
  lcd.setCursor(0,0);
 lcd.print("Starting...");
}


}




static bool measure_environment( float *temperature, float *humidity )
{
static unsigned long measurement_timestamp = millis( );


/* Measure once every four seconds. */
if( millis( ) - measurement_timestamp > 3000ul )
{
if( dht_sensor.measure( temperature, humidity ) == true )
{
  measurement_timestamp = millis( );
  return( true );
}
}


return( false );
}












void loop() {
// put your main code here, to run repeatedly:




float temperature_dht;
float humidity;






/* Measure temperature and humidity.  If the functions returns
 true, then a measurement is available. */
if( measure_environment( &temperature_dht, &humidity ) == true )
{




sensors.requestTemperatures();
float tGlass = sensors.getTempCByIndex(0);
float tAir = sensors.getTempCByIndex(1);


humidityNow = humidity;
needRedraw = true;
waitingForFirst = false;
if (tGlass == DEVICE_DISCONNECTED_C) {
badCountGlass += 1;


} else {
 lastGoodGlass = tGlass;
 haveGlass = true;
 badCountGlass = 0;


}


if (badCountGlass >= 3) {
 Serial.println ("GLASS PROBE ERROR");
 haveGlass = false;
}


if (tAir == DEVICE_DISCONNECTED_C) {
badCountAir += 1;


} else {
 lastGoodAir = tAir;
 haveAir = true;
 badCountAir = 0;


}


if (badCountAir >= 3) {
 Serial.println ("AIR PROBE ERROR");
 haveAir = false;
}










Serial.print("raw Glass= ");
Serial.print(tGlass);
Serial.print(", raw Air= ");
Serial.print(tAir);
Serial.print(", Humidity: ");
Serial.print(humidity,1);
Serial.print(", Probe Glass = ");
Serial.print(lastGoodGlass);
Serial.print(", Probe Air = " );
Serial.print(lastGoodAir);


float RH = humidity;
float dewPoint = 0;
if (haveAir == true) {
float gamma2 = log(RH/ 100.0) + (17.62 * lastGoodAir) / (243.12 + lastGoodAir);
dewPoint = 243.12 * gamma2 / (17.62 - gamma2);
dewPointNow = dewPoint;


Serial.print(", dewPoint: ");
Serial.print(dewPoint);
} else {
Serial.print(", dewPoint: N/A");


}
if (haveAir == true && haveGlass == true){
 float margin = lastGoodGlass - dewPoint;
marginNow = margin;
if (margin <= 0){
 Serial.print(", Margin: ");
 Serial.print(marginNow);
 Serial.println(", Status: CONDENSING");
 statusNow = 2;
} else if (margin <= WARN_MARGIN){
 Serial.print(", Margin: ");
 Serial.print(marginNow);
 Serial.println(", Status: WARNING");
 statusNow = 1;
} else {
 Serial.print(", Margin: ");
 Serial.print(marginNow);
 Serial.println(", Status: SAFE");
 statusNow = 0;
}
} else {
 Serial.println(", Status: SENSOR ERROR");
 statusNow = 3;
}
}


int currentButton = digitalRead(A0);


if (lastButton == HIGH && currentButton == LOW && millis() - lastPressTime > 200) {
 page = page + 1;
if (page > 2) { page = 0; }
lastPressTime = millis();
Serial.print("Page: ");
Serial.println(page);
needRedraw = true;


}
lastButton = currentButton;
updateAlert();
// display screen section
if (waitingForFirst == false) {
if (needRedraw == true) {
if (page == 0) {
 //lcd.clear();
 Serial.println("REDRAW");
 lcd.setCursor(0,0);
 if (haveAir == false) {
 lcd.print ("AIR PROBE ERROR");
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("Humidity ");
 lcd.print(humidityNow, 1);
 lcd.print("%");
 lcd.print("                  ");
 } else {
 lcd.print("Air ");
 lcd.print(lastGoodAir);
 lcd.print("C  ");
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("Humidity ");
 lcd.print(humidityNow, 1);
 lcd.print("%");
 lcd.print("                  ");
 }
 } else if(page == 1) { //page 1 display
 //lcd.clear();
 lcd.setCursor(0,0);
 if (haveGlass == false && haveAir == false) {
 lcd.print ("GLASS PROBE ERR");
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("Dewpoint N/A");
 lcd.print("                  ");
 }else if (haveGlass == false) {
 lcd.print ("GLASS PROBE ERR");
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("Dewpoint ");
 lcd.print(dewPointNow);
 lcd.print("C");
 lcd.print("                  ");
 } else if (haveAir == false) {
 lcd.print("Glass ");
 lcd.print(lastGoodGlass);
 lcd.print("C  ");
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("Dewpoint N/A");
 lcd.print("                  ");
 } else {
 lcd.print("Glass ");
 lcd.print(lastGoodGlass);
 lcd.print("C  ");
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("Dewpoint ");
 lcd.print(dewPointNow);
 lcd.print("C");
 lcd.print("                  ");
}
} else if (page == 2){ //page 2
 //lcd.clear();
 lcd.setCursor(0,0);
 if (statusNow == 3) {
 lcd.print("Sensor Error");
 lcd.print("                  ");
 } else if (statusNow == 2){
 lcd.print("Margin ");
 lcd.print(marginNow, 1);
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("CONDENSING");
 lcd.print("                  ");
} else if (statusNow == 1){
 lcd.print("Margin ");
 lcd.print(marginNow, 1);
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("WARNING");
 lcd.print("                  ");
} else if (statusNow == 0) {
 lcd.print("Margin: ");
 lcd.print(marginNow, 1);
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("SAFE");
 lcd.print("                  ");
}
}
needRedraw = false;
}
}


 }
