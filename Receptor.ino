#include <Wire.h> 
#include <LiquidCrystal_I2C.h>    //If you don't have the LiquidCrystal_I2C library, download it and install it
LiquidCrystal_I2C lcd(0x27,16,2);  //sometimes the adress is not 0x3f. Change to 0x27 if it dosn't work.

#include "max6675.h"
#include <SPI.h>

#define thermoCS   10
#define thermoDO   12
#define thermoCLK  13
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

////Variables lectura termocupla
unsigned long currentMillis = millis();
unsigned long previousMillis = 0;
const long interval = 2500;

////Variables Envio De Datos
//unsigned long currentMillis1 = millis();
//unsigned long previousMillis1 = 0;
const long TiempoCiclo = 2500;

struct servoMoveMessage
{
  volatile int servoNum;
  volatile int positionGoal;
  volatile float interval;
};

struct servoMoveMessage message;

byte *structurePointer;
int structureLength;

void recieveStructure(byte *structurePointer, int structureLength)
{
  //recieveStructure((byte *)&message, sizeof(message));
  if (Serial.available() < sizeof(message))
    return;
  Serial.readBytes(structurePointer, structureLength);
  //Serial.println("message.servoNum");
  Serial.println(message.positionGoal);
  Serial.println(message.interval);
}  

void sendStructure(byte *structurePointer, int structureLength)
{
  digitalWrite(21, LOW);

  digitalWrite(20, LOW);
  delay(10);
  digitalWrite(20, HIGH);

  Serial3.write(structurePointer, structureLength);

  digitalWrite(21, HIGH);
}

//Inputs and outputs
int firing_pin = 3;
int zero_cross = 8;
int pinControl = 7;

//Variables
int last_CH1_state = 0;
bool zero_cross_detected = false;
int firing_delay = 7400;

//////////////////////////////////////////////////////
int maximum_firing_delay = 7400;
/*Later in the code you will se that the maximum delay after the zero detection
 * is 7400. Why? Well, we know that the 220V AC voltage has a frequency of around 50-60HZ so
 * the period is between 20ms and 16ms, depending on the country. We control the firing
 * delay each half period so each 10ms or 8 ms. To amke sure we wont pass thsoe 10ms, I've made tests
 * and the 7400us or 7.4ms was a good value. Measure your frequency and chande that value later */
//////////////////////////////////////////////////////

void setup()
{
  lcd.init();
  lcd.backlight();

  Serial.begin(9600);

  pinMode(pinControl, INPUT_PULLUP);
  
  pinMode(firing_pin, OUTPUT); //Este es el pin 3
  pinMode(zero_cross, INPUT);  //this is the pin number 8
  PCICR |= (1 << PCIE0);       //enable PCMSK0 scan
  PCMSK0 |= (1 << PCINT0);     //Set pin D8 (zero cross input) trigger an interrupt on state change.
}

void loop()
{
  float temperature_read = readThermocouple(); 
  lcd.setCursor(0,0);
  lcd.print("TEMPERATURE");
  lcd.setCursor(7,1);  
  lcd.print(temperature_read);    
  delay(300);
  envioDatos();
}

float readThermocouple()
{ 
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis >= interval))
  {
    previousMillis = currentMillis;
    return thermocouple.readCelsius();
  }
}

void envioDatos()
{ 
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis >= TiempoCiclo))
  {
    previousMillis1 = currentMillis;
    sendStructure((byte *)&message, sizeof(message));
  }
}

ISR(PCINT0_vect)
{
  ///////////////////////////////////////Input from optocoupler
  if (PINB & B00000001)
  {                                        //We make an AND with the state register, We verify if pin D8 is HIGH???
    while (digitalRead(pinControl) == LOW) //PINB & B00000001)
    {
      //Serial.println("while0");
      recieveStructure((byte *)&message, sizeof(message));
      delayMicroseconds(500);
      //Serial.println("while0");
    }

    if (last_CH1_state == 0)
    {                             //If the last state was 0, then we have a state change...
      zero_cross_detected = true; //We have detected a state change! We need both falling and rising edges
    }
  }
  else if (last_CH1_state == 1)
  {                             //If pin 8 is LOW and the last state was HIGH then we have a state change
    zero_cross_detected = true; //We haev detected a state change!  We need both falling and rising edges.
    last_CH1_state = 0;         //Store the current state into the last state for the next loop
  }
}
//End of interruption vector for pins on port B: D8-D13