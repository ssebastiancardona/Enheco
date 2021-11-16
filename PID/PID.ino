/********************************************************
 * PID Basic Example
 * Reading analog input 0 to control analog PWM output 3
 ********************************************************/

#include <PID_v1.h>
//LCD config
#include "max6675.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3f, 16, 2);

//#define PIN_INPUT 8
#define PIN_OUTPUT 3

//Inputs and outputs
//int firing_pin = 3;
int increase_pin = 11;
int decrease_pin = 12;
int zero_cross = 8;
int thermoDO = 9;
int thermoCS = 10;
int thermoCLK = 13;

/////variables delay read temperature//////
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
int temp_read_Delay = 500;

//Start a MAX6675 communication with the selected pins
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

//Variables
int last_CH1_state = 0;
bool zero_cross_detected = false;
//int firing_delay = 7400;
bool pressed_1 = false;
bool pressed_2 = false;

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double Kp = 2, Ki = 5, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

///Variables para envio de datos
//unsigned long currentMillis2 = millis();
unsigned long previousMillis2 = 0;
const long TiempoCiclo = 2500;

/////////////////////////////Estructuras de comunicacion recepcion de datos////////////////////////////////
struct servoMoveMessage
{
  int servoNum;
  int positionGoal;
  float SetpointMaster; // interval;
};

struct servoMoveMessage message;
void recieveStructure(byte *structurePointer, int structureLength)
{
  while (digitalRead(5 == LOW)) //Conectado al pin 15 del mega, controla escucha de puerto serial
  {
    if (Serial.available() < sizeof(message))
      return;
    Serial.readBytes(structurePointer, structureLength);
    Setpoint = message.SetpointMaster;
    //Serial.println(message.SetpointMaster);
  }
}

/////////////////////////////Estructuras de comunicacion envio de datos////////////////////////////////
struct temperaturaMessage
{
  int temperatura1;
};

struct temperaturaMessage message1;
void sendStructure(byte *structurePointer1, int structureLength1)
{//cli();
  //digitalWrite(6, LOW); //Conectado al pin 21 del mega, controla escucha de puerto serial para transmicion
  //message1.temperatura1 = Input;
  //delay(10);
  //digitalWrite(2, LOW);
  Serial.write(structurePointer1, structureLength1);
  delay(10);
  //digitalWrite(2, HIGH);
  digitalWrite(6, HIGH);  
  //sei();
}
//float temperatura1;
void setup()
{
  //Define the pins
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  pinMode(7, OUTPUT); //Pin de activacion de ISR(PCINT2_vect) conectado al A15 del arduino mega
  digitalWrite(7, HIGH);
  pinMode(6, OUTPUT); //Conectado al pin 21 del mega, controla escucha de puerto serial para transmicion
  digitalWrite(6, HIGH);
  pinMode(5, INPUT); //Conectado al pin 15 del mega, controla escucha de puerto serial para recepcion
  pinMode(PIN_OUTPUT, OUTPUT);
  pinMode(zero_cross, INPUT);
  pinMode(increase_pin, INPUT);
  pinMode(decrease_pin, INPUT);
  PCICR |= (1 << PCIE0);    //enable PCMSK0 scan
  //PCICR |= (1 << PCIE1);    //enable PCMSK1 scan
  PCICR |= (1 << PCIE2);    //enable PCMSK2 scan
  PCMSK2 |= (1 << PCINT4);  //set pin 4 (comunication vector interrupt). // conectado al pin 20 del mega
  //PCMSK1 |= (1 << PCINT11); //set pin A3 (comunication vector interrupt).
  PCMSK0 |= (1 << PCINT0);  //Set pin D8 (zero cross input) trigger an interrupt on state change.
  PCMSK0 |= (1 << PCINT3);  //Set pin D11 (increase button) trigger an interrupt on state change.
  PCMSK0 |= (1 << PCINT4);  //Set pin D12 (decrease button) trigger an interrupt on state change.

  lcd.init();      //Start the LC communication
  lcd.backlight(); //Turn on backlight for LCD

  //initialize the variables we're linked to
  Input = thermocouple.readCelsius();
  Setpoint = 25;
   message1.temperatura1 = thermocouple.readCelsius();
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
}
int testtemp;
void loop()
{
  //message1.temperatura1 = Input;
  //Serial.println(temperatura1);
  ///////////////////////Solicitud de estado/////////////////////////////////
  unsigned long currentMillis2 = millis();
  if ((currentMillis2 - previousMillis2 >= TiempoCiclo))
  {
    previousMillis2 = currentMillis2;
    //cli();
    testtemp = int(Input);
    delay(20);
    message1.temperatura1 = testtemp;
    //Serial.println("bucle");
    digitalWrite(6, LOW);
    delay(20);
    sendStructure((byte *)&message1, sizeof(message1));
  }

  currentMillis = millis();
  if (currentMillis - previousMillis >= temp_read_Delay)
  {
    previousMillis += temp_read_Delay; //Increase the previous time for next loop
    Input = thermocouple.readCelsius();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set: ");
    lcd.setCursor(5, 0);
    lcd.print(Setpoint);
    lcd.setCursor(0, 1);
    lcd.print("Real temp: ");
    lcd.setCursor(11, 1);
    lcd.print(Input);
  }
  //Input = analogRead(PIN_INPUT);
  myPID.Compute();
  //digitalWrite(PIN_OUTPUT, Output);

  if (zero_cross_detected)
  {
    delayMicroseconds(Output); //This delay controls the power
    digitalWrite(PIN_OUTPUT, Output);
    delayMicroseconds(100);
    zero_cross_detected = false;
  }
}

ISR(PCINT0_vect)
{
  ///////////////////////////////////////Input from optocoupler
  if (PINB & B00000001)
  { //We make an AND with the state register, We verify if pin D8 is HIGH???
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

  if (PINB & B00001000)
  { //We make an AND with the state register, We verify if pin D11 is HIGH???
    if (!pressed_1)
    {
      Setpoint = Setpoint + 5; //Increase the temperature by 5. Change this with your value if you want.
      delay(20);
      pressed_1 = true;
    }
  }
  else if (pressed_1)
  {
    pressed_1 = false;
  }

  if (PINB & B00010000)
  { //We make an AND with the state register, We verify if pin D12 is HIGH???
    if (!pressed_2)
    {
      Setpoint = Setpoint - 5; //Decrease the temperature by 5. Change this with your value if you want.
      delay(20);
      pressed_2 = true;
    }
  }
  else if (pressed_2)
  {
    pressed_2 = false;
  }
}

ISR(PCINT2_vect) //
{
  //cli();
  recieveStructure((byte *)&message, sizeof(message)); // (vector interrupt) pin 4. Conectado al pin 20 del mega
  //sei();
}
/*
ISR(PCINT1_vect)
{
  sendStructure((byte *)&message1, sizeof(message1)); // (vector interrupt) pin A3. Conectado al pin 19 del mega
}*/