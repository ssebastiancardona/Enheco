#include <Adafruit_GFX.h>  // Hardware-specific library
#include <TouchScreen.h>   // Hardware-specific library
#include <MCUFRIEND_kbv.h> // Hardware-specific library
//#include <SoftwareSerial.h> // Serial library
#include "max6675.h"

MCUFRIEND_kbv tft; // Hardware-specific library

/////////Definimos los pines a usar con el Max6675/////////
#define thermoDO 50
#define thermoCLK 52
#define thermoCS 53
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

#define thermo1DO 28
#define thermo1CLK 30
#define thermo1CS 26
MAX6675 thermocouple1(thermo1CLK, thermo1CS, thermo1DO);

///Definimos las variables minimas y maximas del touch////////////////////////
#define TS_MINX 129
#define TS_MINY 115
#define TS_MAXX 947
#define TS_MAXY 915

///Definimos los pines que usa la libreria para comunicarse con la pantalla///
#define YP A2 // must be an analog pin, use "An" notation!
#define XM A3 // must be an analog pin, use "An" notation!
#define YM 8  // can be a digital pin
#define XP 9  // can be a digital pin

//////Declaracion de la libreria touch////////////////////////////////////////
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//////Declaracion de la libreria Serial/////////
//SoftwareSerial mySerial(45, 47); // RX, TX

// Assign human-readable names to some common 16-bit color values://///////////
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFFC300
#define GOLD 0xFEA0
#define BROWN 0xA145
#define SILVER 0xC618
#define LIME 0x07E0
#define GRIS 0xA5A5A5

///////////////Estado de la maquina de estados finitos/////////////////////
#define S_HOME 0
#define S_SET_TEMP 1
#define S_SET_TIME 2
#define S_WIFI_MIN 3
#define S_SET_CONF 4
#define M_INICIO 5
#define S_WIFI_MAY 6
#define S_WIFI_MIN_PASS 7
#define S_WIFI_MAY_PASS 8
uint8_t estado = M_INICIO;

////////Variables para anular el delay en funciones de lectura de temperatura////////////
unsigned long currentMillis = millis();
unsigned long previousMillis = 0;
const long interval = 2500;

/////////Variables Generales///////////
int temperatura;
float deseada;

//////////Variables control original////////////////////////
String SetTemp;
String Usr;
String Pass;

////Variables para envio de datos
//unsigned long currentMillis2 = millis();
unsigned long previousMillis2 = 0;
const long TiempoCiclo = 2500;

/////////////////////////////Estructuras de comunicacion recepcion de datos////////////////////////////////
struct temperaturaMessage
{
  int temperatura1;
};

struct temperaturaMessage message1;
void recieveStructure(byte *structurePointer1, int structureLength1)
{
  while (digitalRead(21 == LOW)) //Pin 6 en 328p, pin de activacion pone en escucha puerto serie Arduino mega
  {
    if (Serial3.available() < sizeof(message1))
      return;
    //Serial.println("Entro while receive");
    Serial3.readBytes(structurePointer1, structureLength1);
    temperatura = message1.temperatura1;
    Serial.println(temperatura);
    //INICIO(); if(S_)
    tft.setCursor(10, 45);
    tft.setTextSize(8);
    tft.setTextColor(WHITE, BLACK);
    tft.println(temperatura);
  }
}

/////////////////////////////Estructuras de comunicacion envio de datos////////////////////////////////
struct servoMoveMessage
{
  int servoNum;
  int positionGoal;
  float SetpointMaster;
};

struct servoMoveMessage message;
void sendStructure(byte *structurePointer, int structureLength)
{
  digitalWrite(17, LOW); // Conectado al pin 5 para poner en escucha el puerto serial
  delay(10);
  digitalWrite(20, LOW);                            // Conectado en el pin 4 del mega para disparar ISR(PCINT2_vect) recieveStructure
  Serial3.write(structurePointer, structureLength); //TX3 pertenece al pin 14 en arduino mega
  delay(10);
  digitalWrite(20, HIGH); // Establece pin en alto para cerrar la escucha del puerto serie arduino
  digitalWrite(17, HIGH); // Termina disparo de ISR
}
int pininto = 18;
void setup()
{
  //attachInterrupt(digitalPinToInterrupt(pininto), interrupcion, LOW);
  //PCICR |= (1 << PCIE0);    //enable PCMSK2 scan puerto analogo
  //PCMSK0 |= (1 << PCINT1); // Disparo de ISR PIN A15 conectado al pin 2 del 328p

  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);

  //////////////////Pines para control de envio de datos///////////////////////////
  pinMode(20, OUTPUT); // Conectado en el pin 4 del 328p para disparar ISR(PCINT2_vect) recieveStructure
  digitalWrite(20, HIGH);
  pinMode(19, OUTPUT); // Conectado en el pin A3 del 328p para disparar ISR(PCINT1_vect) sendStructure
  digitalWrite(19, HIGH);

  //////////////////Pines para control de recepcion de datos////////////////////////
  pinMode(21, INPUT); // Controla la escucha del puerto serie del Arduino mega, conectado al pin 6
  //pinMode(16, INPUT);
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH); // Conectado al pin 5 para poner en escucha el puerto serial

  tft.reset();
  tft.begin(0x9487);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  INICIO();
}

void loop()
{
  if (digitalRead(21 == LOW && estado == M_INICIO))
  {
    //delay(5);
    recieveStructure((byte *)&message1, sizeof(message1));
  }
  /*///////////////////////Solicitud de estado/////////////////////////////////
  unsigned long currentMillis2 = millis();
  if ((currentMillis2 - previousMillis2 >= TiempoCiclo))
  {
    previousMillis2 = currentMillis2;
    digitalWrite(19, HIGH); // Conectado en el pin A3 del 328p para disparar ISR(PCINT1_vect) sendStructure 
    //delay(20);      
    //recieveStructure((byte *)&message, sizeof(message)); // ISR(PCINT1_vect) pin A3. Conectado al pin 17 del mega
    digitalWrite(19, LOW);
    //Serial.println("entro");
  }*/

  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > ts.pressureThreshhold)
  {
    p.x = map(p.x, TS_MAXX, TS_MINX, 0, 320);
    p.y = map(p.y, TS_MAXY, TS_MINY, 0, 240);

    switch (estado)
    {              //la instruccion switch es la funcion que permite que unicamente se ejecute el estado que este definido a nivel global
    case M_INICIO: /////////////////////////////////////////INICIO ESTADO M_INICIO////////////////////
      if (p.x > 103 && p.x < 180 && p.y > 77 && p.y < 123)
      { // Transición S_SET_TEMP
        estado = S_SET_TEMP;
        tft.fillScreen(BLACK);
        tft.setCursor(30, 10);
        tft.setTextSize(3);
        tft.setTextColor(GOLD, BLACK);
        tft.println("Set Temperatura");
        TecladoNumerico();
        p.x = 0;
        p.y = 0;
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 90 && p.x < 175 && p.y > 5 && p.y < 47)
      { // Transición Set Tiempo
        estado = S_SET_TIME;
        tft.fillScreen(BLACK);
        tft.setCursor(30, 10);
        tft.setTextSize(3);
        tft.setTextColor(GOLD, BLACK);
        tft.println("Set Tiempo");
        TecladoNumerico();
        p.x = 0;
        p.y = 0;
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 36 && p.x < 80 && p.y > 155 && p.y < 206)
      { // Transición Configurar Wifi
        estado = S_WIFI_MIN;
        p.x = 0;
        p.y = 0;
        tft.fillScreen(BLACK);
        TecladoMinusculas();
        break; //Rompe el case y regresa al loop
      }        //////////////////////////////////////////////// FIN ESTADO INICIO////////////

    case S_SET_TEMP: ////////////////////////////////////////INICIO TECLADO NUMERICO S_SET_TEMP ////
      if (p.x > 5 && p.x < 38 && p.y > 200 && p.y < 238)
      { // Transición HOME
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
        SetTemp = "";
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 138 && p.x < 198 && p.y > 175 && p.y < 190)
      { // Tecla Clear
        p.x = 0;
        p.y = 0;
        //Serial.println("Clear");
        SetTemp = "";
        Borrar();
        //imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 305 && p.y > 109 && p.y < 135)
      { // Numero 7
        p.x = 0;
        p.y = 0;
        //Serial.println("7");
        SetTemp += "7";
        imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 247 && p.x < 270 && p.y > 109 && p.y < 135)
      { // Numero 8
        p.x = 0;
        p.y = 0;
        //Serial.println("8");
        SetTemp += "8";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 211 && p.x < 237 && p.y > 109 && p.y < 135)
      { // Numero 9
        p.x = 0;
        p.y = 0;
        //Serial.println("9");
        SetTemp += "9";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 280 && p.x < 300 && p.y > 73 && p.y < 100)
      { // Numero 4
        p.x = 0;
        p.y = 0;
        //Serial.println("4");
        SetTemp += "4";
        imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 247 && p.x < 270 && p.y > 73 && p.y < 100)
      { // Numero 5
        p.x = 0;
        p.y = 0;
        //Serial.println("5");
        SetTemp += "5";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 213 && p.x < 240 && p.y > 73 && p.y < 100)
      { // Numero 6
        p.x = 0;
        p.y = 0;
        //Serial.println("6");
        SetTemp += "6";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 280 && p.x < 300 && p.y > 40 && p.y < 63)
      { // Numero 1
        p.x = 0;
        p.y = 0;
        //Serial.println("1");
        SetTemp += "1";
        imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 247 && p.x < 270 && p.y > 40 && p.y < 63)
      { // Numero 2
        p.x = 0;
        p.y = 0;
        //Serial.println("2");
        SetTemp += "2";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 213 && p.x < 240 && p.y > 41 && p.y < 63)
      { // Numero 3
        p.x = 0;
        p.y = 0;
        //Serial.println("3");
        SetTemp += "3";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 247 && p.x < 270 && p.y > 5 && p.y < 30)
      { // Numero 0
        p.x = 0;
        p.y = 0;
        //Serial.println("0");
        SetTemp += "0";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 213 && p.x < 240 && p.y > 5 && p.y < 30)
      { // Letra OK
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
        deseada = SetTemp.toInt();
        message.SetpointMaster = deseada;
        sendStructure((byte *)&message, sizeof(message));
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }        ///////////////////////////////////////////////////////FIN TECLADO NUMERICO S_SET_TEMP ////

    case S_SET_TIME: //////////////////////////////////////// INICIO TECLADO NUMERICO S_SET_TIME //////
      if (p.x > 5 && p.x < 38 && p.y > 200 && p.y < 238)
      { // Transición HOME
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
        SetTemp = "";
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 138 && p.x < 198 && p.y > 175 && p.y < 190)
      { // Tecla Clear
        p.x = 0;
        p.y = 0;
        //Serial.println("Clear");
        SetTemp = "";
        Borrar();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 305 && p.y > 109 && p.y < 135)
      { // Numero 7
        p.x = 0;
        p.y = 0;
        //Serial.println("7");
        SetTemp += "7";
        imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 247 && p.x < 270 && p.y > 109 && p.y < 135)
      { // Numero 8
        p.x = 0;
        p.y = 0;
       // Serial.println("8");
        SetTemp += "8";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 211 && p.x < 237 && p.y > 109 && p.y < 135)
      { // Numero 9
        p.x = 0;
        p.y = 0;
       // Serial.println("9");
        SetTemp += "9";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 280 && p.x < 300 && p.y > 73 && p.y < 100)
      { // Numero 4
        p.x = 0;
        p.y = 0;
       // Serial.println("4");
        SetTemp += "4";
        imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 247 && p.x < 270 && p.y > 73 && p.y < 100)
      { // Numero 5
        p.x = 0;
        p.y = 0;
       // Serial.println("5");
        SetTemp += "5";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 213 && p.x < 240 && p.y > 73 && p.y < 100)
      { // Numero 6
        p.x = 0;
        p.y = 0;
      //  Serial.println("6");
        SetTemp += "6";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 280 && p.x < 300 && p.y > 40 && p.y < 63)
      { // Numero 1
        p.x = 0;
        p.y = 0;
       // Serial.println("1");
        SetTemp += "1";
        imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 247 && p.x < 270 && p.y > 40 && p.y < 63)
      { // Numero 2
        p.x = 0;
        p.y = 0;
       // Serial.println("2");
        SetTemp += "2";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 213 && p.x < 240 && p.y > 41 && p.y < 63)
      { // Numero 3
        p.x = 0;
        p.y = 0;
       // Serial.println("3");
        SetTemp += "3";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 247 && p.x < 270 && p.y > 5 && p.y < 30)
      { // Numero 0
        p.x = 0;
        p.y = 0;
       // Serial.println("0");
        SetTemp += "0";
        imprime();
        delay(100);
        break;
      }

      if (p.x > 213 && p.x < 240 && p.y > 5 && p.y < 30)
      { // Letra OK
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
       // Serial.println("0k");
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }        ////////////////////////////////////////////////FIN TECLADO NUMERICO S_SET_TIME ////

    case S_WIFI_MIN: //////////////////////////////////////// INICIO TECLADO MINUSCULAS S_WIFI //////
      if (p.x > 5 && p.x < 38 && p.y > 200 && p.y < 238)
      { // Transición HOME
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 133 && p.x < 284 && p.y > 165 && p.y < 184)
      { // Posiciona Cursor en Pass
        estado = S_WIFI_MIN_PASS;
        p.x = 0;
        p.y = 0;
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 85 && p.x < 149 && p.y > 183 && p.y < 198)
      { // Tecla Clear
        p.x = 0;
        p.y = 0;
        Serial.println("Clear");
        Usr = "";
        BorradoUsr();
        //imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 311 && p.x < 337 && p.y > 112 && p.y < 133)
      { // Numero 1
        p.x = 0;
        p.y = 0;
        Serial.println("1");
        Usr += "1";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 112 && p.y < 133)
      { // Numero 2
        p.x = 0;
        p.y = 0;
        Serial.println("2");
        Usr += "2";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 273 && p.y > 112 && p.y < 133)
      { // Numero 3
        p.x = 0;
        p.y = 0;
        Serial.println("3");
        Usr += "3";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 112 && p.y < 133)
      { // Numero 4
        p.x = 0;
        p.y = 0;
        Serial.println("4");
        Usr += "4";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 181 && p.x < 203 && p.y > 112 && p.y < 133)
      { // Numero 5
        p.x = 0;
        p.y = 0;
        Serial.println("5");
        Usr += "5";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 147 && p.x < 170 && p.y > 112 && p.y < 133)
      { // Numero 6
        p.x = 0;
        p.y = 0;
        Serial.println("6");
        Usr += "6";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 134 && p.y > 112 && p.y < 133)
      { // Numero 7
        p.x = 0;
        p.y = 0;
        Serial.println("7");
        Usr += "7";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 112 && p.y < 133)
      { // Numero 8
        p.x = 0;
        p.y = 0;
        Serial.println("8");
        Usr += "8";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 65 && p.y > 112 && p.y < 133)
      { // Numero 9
        p.x = 0;
        p.y = 0;
        Serial.println("9");
        Usr += "9";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 35 && p.y > 112 && p.y < 133)
      { // Numero 0
        p.x = 0;
        p.y = 0;
        Serial.println("0");
        Usr += "0";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 312 && p.x < 334 && p.y > 77 && p.y < 101)
      { // Letra q
        p.x = 0;
        p.y = 0;
        Serial.println("q");
        Usr += "q";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 77 && p.y < 101)
      { // Letra w
        p.x = 0;
        p.y = 0;
        Serial.println("w");
        Usr += "w";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 270 && p.y > 77 && p.y < 101)
      { // Letra e
        p.x = 0;
        p.y = 0;
        Serial.println("e");
        Usr += "e";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 77 && p.y < 101)
      { // Letra r
        p.x = 0;
        p.y = 0;
        Serial.println("r");
        Usr += "r";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 182 && p.x < 203 && p.y > 77 && p.y < 101)
      { // Letra t
        p.x = 0;
        p.y = 0;
        Serial.println("t");
        Usr += "t";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 148 && p.x < 168 && p.y > 77 && p.y < 101)
      { // Letra y
        p.x = 0;
        p.y = 0;
        Serial.println("y");
        Usr += "y";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 113 && p.x < 135 && p.y > 77 && p.y < 101)
      { // Letra u
        p.x = 0;
        p.y = 0;
        Serial.println("u");
        Usr += "u";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 80 && p.x < 100 && p.y > 77 && p.y < 101)
      { // Letra i
        p.x = 0;
        p.y = 0;
        Serial.println("i");
        Usr += "i";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 77 && p.y < 101)
      { // Letra o
        p.x = 0;
        p.y = 0;
        Serial.println("o");
        Usr += "o";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 14 && p.x < 35 && p.y > 77 && p.y < 101)
      { // Letra p
        p.x = 0;
        p.y = 0;
        Serial.println("p");
        Usr += "p";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 334 && p.y > 40 && p.y < 65)
      { // Letra a
        p.x = 0;
        p.y = 0;
        Serial.println("a");
        Usr += "a";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 300 && p.y > 40 && p.y < 65)
      { // Letra s
        p.x = 0;
        p.y = 0;
        Serial.println("s");
        Usr += "s";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 250 && p.x < 270 && p.y > 40 && p.y < 65)
      { // Letra d
        p.x = 0;
        p.y = 0;
        Serial.println("d");
        Usr += "d";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 40 && p.y < 65)
      { // Letra f
        p.x = 0;
        p.y = 0;
        Serial.println("f");
        Usr += "f";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 180 && p.x < 200 && p.y > 40 && p.y < 65)
      { // Letra g
        p.x = 0;
        p.y = 0;
        Serial.println("g");
        Usr += "g";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 143 && p.x < 170 && p.y > 40 && p.y < 65)
      { // Letra h
        p.x = 0;
        p.y = 0;
        Serial.println("h");
        Usr += "h";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 136 && p.y > 40 && p.y < 65)
      { // Letra j
        p.x = 0;
        p.y = 0;
        Serial.println("j");
        Usr += "j";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 40 && p.y < 65)
      { // Letra k
        p.x = 0;
        p.y = 0;
        Serial.println("k");
        Usr += "k";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 68 && p.y > 40 && p.y < 65)
      { // Letra l
        p.x = 0;
        p.y = 0;
        Serial.println("l");
        Usr += "l";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 36 && p.y > 40 && p.y < 65)
      { // Tecla Mayusculas
        estado = S_WIFI_MAY;
        p.x = 0;
        p.y = 0;
        TecladoMayusculas();
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 336 && p.y > 4 && p.y < 31)
      { // Letra z
        p.x = 0;
        p.y = 0;
        Serial.println("z");
        Usr += "z";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 304 && p.y > 4 && p.y < 31)
      { // Letra x
        p.x = 0;
        p.y = 0;
        Serial.println("x");
        Usr += "x";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 244 && p.x < 267 && p.y > 4 && p.y < 31)
      { // Letra c
        p.x = 0;
        p.y = 0;
        Serial.println("c");
        Usr += "c";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 4 && p.y < 31)
      { // Letra v
        p.x = 0;
        p.y = 0;
        Serial.println("v");
        Usr += "v";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 179 && p.x < 203 && p.y > 4 && p.y < 31)
      { // Letra b
        p.x = 0;
        p.y = 0;
        Serial.println("b");
        Usr += "b";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 146 && p.x < 170 && p.y > 4 && p.y < 31)
      { // Letra n
        p.x = 0;
        p.y = 0;
        Serial.println("n");
        Usr += "n";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 114 && p.x < 136 && p.y > 4 && p.y < 31)
      { // Letra m
        p.x = 0;
        p.y = 0;
        Serial.println("m");
        Usr += "m";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 81 && p.x < 102 && p.y > 4 && p.y < 31)
      { // Signo ,
        p.x = 0;
        p.y = 0;
        Serial.println(",");
        Usr += ",";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 4 && p.y < 31)
      { // Signo .
        p.x = 0;
        p.y = 0;
        Serial.println(".");
        Usr += ".";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

    case S_WIFI_MAY: //////////////////////////////////////// INICIO TECLADO MAYUSCULAS S_WIFI_MAY ///////////////
      if (p.x > 5 && p.x < 38 && p.y > 200 && p.y < 238)
      { // Transición HOME
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 133 && p.x < 284 && p.y > 165 && p.y < 184)
      { // Posiciona Cursor en Pass
        estado = S_WIFI_MIN_PASS;
        p.x = 0;
        p.y = 0;
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 73 && p.x < 134 && p.y > 200 && p.y < 212)
      { // Tecla Clear
        p.x = 0;
        p.y = 0;
        Serial.println("Clear");
        Usr = "";
        //Borrar();
        //imprime();
        BorradoUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 52 && p.x < 115 && p.y > 163 && p.y < 183)
      { // Tecla Enter
        p.x = 0;
        p.y = 0;
        tft.fillScreen(BLACK);
        tft.setCursor(30, 10);
        tft.setTextSize(3);
        tft.setTextColor(GOLD, BLACK);
        tft.println("Estado coneccion");
        //EEPROM.write(0, Usr);                                                                 ///////////////////////////////
        //envioCredenciales();

        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 311 && p.x < 337 && p.y > 112 && p.y < 133)
      { // Numero 1
        p.x = 0;
        p.y = 0;
        Serial.println("1");
        Usr += "1";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 112 && p.y < 133)
      { // Numero 2
        p.x = 0;
        p.y = 0;
        Serial.println("2");
        Usr += "2";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 273 && p.y > 112 && p.y < 133)
      { // Numero 3
        p.x = 0;
        p.y = 0;
        Serial.println("3");
        Usr += "3";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 112 && p.y < 133)
      { // Numero 4
        p.x = 0;
        p.y = 0;
        Serial.println("4");
        Usr += "4";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 181 && p.x < 203 && p.y > 112 && p.y < 133)
      { // Numero 5
        p.x = 0;
        p.y = 0;
        Serial.println("5");
        Usr += "5";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 147 && p.x < 170 && p.y > 112 && p.y < 133)
      { // Numero 6
        p.x = 0;
        p.y = 0;
        Serial.println("6");
        Usr += "6";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 134 && p.y > 112 && p.y < 133)
      { // Numero 7
        p.x = 0;
        p.y = 0;
        Serial.println("7");
        Usr += "7";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 112 && p.y < 133)
      { // Numero 8
        p.x = 0;
        p.y = 0;
        Serial.println("8");
        Usr += "8";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 65 && p.y > 112 && p.y < 133)
      { // Numero 9
        p.x = 0;
        p.y = 0;
        Serial.println("9");
        Usr += "9";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 35 && p.y > 112 && p.y < 133)
      { // Numero 0
        p.x = 0;
        p.y = 0;
        Serial.println("0");
        Usr += "0";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 312 && p.x < 334 && p.y > 77 && p.y < 101)
      { // Letra Q
        p.x = 0;
        p.y = 0;
        Serial.println("Q");
        Usr += "Q";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 77 && p.y < 101)
      { // Letra W
        p.x = 0;
        p.y = 0;
        Serial.println("W");
        Usr += "W";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 270 && p.y > 77 && p.y < 101)
      { // Letra E
        p.x = 0;
        p.y = 0;
        Serial.println("E");
        Usr += "E";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 77 && p.y < 101)
      { // Letra R
        p.x = 0;
        p.y = 0;
        Serial.println("R");
        Usr += "R";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 182 && p.x < 203 && p.y > 77 && p.y < 101)
      { // Letra T
        p.x = 0;
        p.y = 0;
        Serial.println("T");
        Usr += "T";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 148 && p.x < 168 && p.y > 77 && p.y < 101)
      { // Letra Y
        p.x = 0;
        p.y = 0;
        Serial.println("Y");
        Usr += "Y";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 113 && p.x < 135 && p.y > 77 && p.y < 101)
      { // Letra U
        p.x = 0;
        p.y = 0;
        Serial.println("U");
        Usr += "U";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 80 && p.x < 100 && p.y > 77 && p.y < 101)
      { // Letra I
        p.x = 0;
        p.y = 0;
        Serial.println("I");
        Usr += "I";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 77 && p.y < 101)
      { // Letra O
        p.x = 0;
        p.y = 0;
        Serial.println("O");
        Usr += "O";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 14 && p.x < 35 && p.y > 77 && p.y < 101)
      { // Letra P
        p.x = 0;
        p.y = 0;
        Serial.println("P");
        Usr += "P";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 334 && p.y > 40 && p.y < 65)
      { // Letra A
        p.x = 0;
        p.y = 0;
        Serial.println("A");
        Usr += "A";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 300 && p.y > 40 && p.y < 65)
      { // Letra S
        p.x = 0;
        p.y = 0;
        Serial.println("S");
        Usr += "S";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 250 && p.x < 270 && p.y > 40 && p.y < 65)
      { // Letra D
        p.x = 0;
        p.y = 0;
        Serial.println("D");
        Usr += "D";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 40 && p.y < 65)
      { // Letra F
        p.x = 0;
        p.y = 0;
        Serial.println("F");
        Usr += "F";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 180 && p.x < 200 && p.y > 40 && p.y < 65)
      { // Letra G
        p.x = 0;
        p.y = 0;
        Serial.println("G");
        Usr += "G";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 143 && p.x < 170 && p.y > 40 && p.y < 65)
      { // Letra H
        p.x = 0;
        p.y = 0;
        Serial.println("H");
        Usr += "H";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 136 && p.y > 40 && p.y < 65)
      { // Letra J
        p.x = 0;
        p.y = 0;
        Serial.println("J");
        Usr += "J";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 40 && p.y < 65)
      { // Letra K
        p.x = 0;
        p.y = 0;
        Serial.println("K");
        Usr += "K";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 68 && p.y > 40 && p.y < 65)
      { // Letra L
        p.x = 0;
        p.y = 0;
        Serial.println("L");
        Usr += "L";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 36 && p.y > 40 && p.y < 65)
      { // Tecla Mayusculas
        estado = S_WIFI_MIN;
        p.x = 0;
        p.y = 0;
        TecladoMinusculas();
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 336 && p.y > 4 && p.y < 31)
      { // Letra Z
        p.x = 0;
        p.y = 0;
        Serial.println("Z");
        Usr += "Z";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 304 && p.y > 4 && p.y < 31)
      { // Letra X
        p.x = 0;
        p.y = 0;
        Serial.println("X");
        Usr += "X";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 244 && p.x < 267 && p.y > 4 && p.y < 31)
      { // Letra c
        p.x = 0;
        p.y = 0;
        Serial.println("C");
        Usr += "C";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 4 && p.y < 31)
      { // Letra V
        p.x = 0;
        p.y = 0;
        Serial.println("V");
        Usr += "V";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 179 && p.x < 203 && p.y > 4 && p.y < 31)
      { // Letra B
        p.x = 0;
        p.y = 0;
        Serial.println("B");
        Usr += "B";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 146 && p.x < 170 && p.y > 4 && p.y < 31)
      { // Letra N
        p.x = 0;
        p.y = 0;
        Serial.println("N");
        Usr += "N";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 114 && p.x < 136 && p.y > 4 && p.y < 31)
      { // Letra M
        p.x = 0;
        p.y = 0;
        Serial.println("M");
        Usr += "M";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 81 && p.x < 102 && p.y > 4 && p.y < 31)
      { // Signo -
        p.x = 0;
        p.y = 0;
        Serial.println("-");
        Usr += "-";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 4 && p.y < 31)
      { // Signo _
        p.x = 0;
        p.y = 0;
        Serial.println("_");
        Usr += "_";
        PrintUsr();
        delay(100);
        break; //Rompe el case y regresa al loop
      }        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    case S_WIFI_MIN_PASS: //////////////////////////////////////// INICIO TECLADO MINUSCULAS S_WIFI_MIN_PASS //////
      if (p.x > 5 && p.x < 38 && p.y > 200 && p.y < 238)
      { // Transición HOME
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 133 && p.x < 284 && p.y > 200 && p.y < 220)
      { // Posiciona Cursor en Usr
        estado = S_WIFI_MIN;
        p.x = 0;
        p.y = 0;
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 85 && p.x < 149 && p.y > 183 && p.y < 198)
      { // Tecla Clear
        p.x = 0;
        p.y = 0;
        Serial.println("Clear");
        Pass = "";
        BorradoPass();
        //imprime();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 311 && p.x < 337 && p.y > 112 && p.y < 133)
      { // Numero 1
        p.x = 0;
        p.y = 0;
        Serial.println("1");
        Pass += "1";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 112 && p.y < 133)
      { // Numero 2
        p.x = 0;
        p.y = 0;
        Serial.println("2");
        Pass += "2";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 273 && p.y > 112 && p.y < 133)
      { // Numero 3
        p.x = 0;
        p.y = 0;
        Serial.println("3");
        Pass += "3";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 112 && p.y < 133)
      { // Numero 4
        p.x = 0;
        p.y = 0;
        Serial.println("4");
        Pass += "4";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 181 && p.x < 203 && p.y > 112 && p.y < 133)
      { // Numero 5
        p.x = 0;
        p.y = 0;
        Serial.println("5");
        Pass += "5";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 147 && p.x < 170 && p.y > 112 && p.y < 133)
      { // Numero 6
        p.x = 0;
        p.y = 0;
        Serial.println("6");
        Pass += "6";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 134 && p.y > 112 && p.y < 133)
      { // Numero 7
        p.x = 0;
        p.y = 0;
        Serial.println("7");
        Pass += "7";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 112 && p.y < 133)
      { // Numero 8
        p.x = 0;
        p.y = 0;
        Serial.println("8");
        Pass += "8";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 65 && p.y > 112 && p.y < 133)
      { // Numero 9
        p.x = 0;
        p.y = 0;
        Serial.println("9");
        Pass += "9";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 35 && p.y > 112 && p.y < 133)
      { // Numero 0
        p.x = 0;
        p.y = 0;
        Serial.println("0");
        Pass += "0";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 312 && p.x < 334 && p.y > 77 && p.y < 101)
      { // Letra q
        p.x = 0;
        p.y = 0;
        Serial.println("q");
        Pass += "q";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 77 && p.y < 101)
      { // Letra w
        p.x = 0;
        p.y = 0;
        Serial.println("w");
        Pass += "w";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 270 && p.y > 77 && p.y < 101)
      { // Letra e
        p.x = 0;
        p.y = 0;
        Serial.println("e");
        Pass += "e";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 77 && p.y < 101)
      { // Letra r
        p.x = 0;
        p.y = 0;
        Serial.println("r");
        Pass += "r";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 182 && p.x < 203 && p.y > 77 && p.y < 101)
      { // Letra t
        p.x = 0;
        p.y = 0;
        Serial.println("t");
        Pass += "t";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 148 && p.x < 168 && p.y > 77 && p.y < 101)
      { // Letra y
        p.x = 0;
        p.y = 0;
        Serial.println("y");
        Pass += "y";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 113 && p.x < 135 && p.y > 77 && p.y < 101)
      { // Letra u
        p.x = 0;
        p.y = 0;
        Serial.println("u");
        Pass += "u";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 80 && p.x < 100 && p.y > 77 && p.y < 101)
      { // Letra i
        p.x = 0;
        p.y = 0;
        Serial.println("i");
        Pass += "i";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 77 && p.y < 101)
      { // Letra o
        p.x = 0;
        p.y = 0;
        Serial.println("o");
        Pass += "o";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 14 && p.x < 35 && p.y > 77 && p.y < 101)
      { // Letra p
        p.x = 0;
        p.y = 0;
        Serial.println("p");
        Pass += "p";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 334 && p.y > 40 && p.y < 65)
      { // Letra a
        p.x = 0;
        p.y = 0;
        Serial.println("a");
        Pass += "a";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 300 && p.y > 40 && p.y < 65)
      { // Letra s
        p.x = 0;
        p.y = 0;
        Serial.println("s");
        Pass += "s";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 250 && p.x < 270 && p.y > 40 && p.y < 65)
      { // Letra d
        p.x = 0;
        p.y = 0;
        Serial.println("d");
        Pass += "d";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 40 && p.y < 65)
      { // Letra f
        p.x = 0;
        p.y = 0;
        Serial.println("f");
        Pass += "f";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 180 && p.x < 200 && p.y > 40 && p.y < 65)
      { // Letra g
        p.x = 0;
        p.y = 0;
        Serial.println("g");
        Pass += "g";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 143 && p.x < 170 && p.y > 40 && p.y < 65)
      { // Letra h
        p.x = 0;
        p.y = 0;
        Serial.println("h");
        Pass += "h";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 136 && p.y > 40 && p.y < 65)
      { // Letra j
        p.x = 0;
        p.y = 0;
        Serial.println("j");
        Pass += "j";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 40 && p.y < 65)
      { // Letra k
        p.x = 0;
        p.y = 0;
        Serial.println("k");
        Pass += "k";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 68 && p.y > 40 && p.y < 65)
      { // Letra l
        p.x = 0;
        p.y = 0;
        Serial.println("l");
        Pass += "l";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 36 && p.y > 40 && p.y < 65)
      { // Tecla Mayusculas
        estado = S_WIFI_MAY;
        p.x = 0;
        p.y = 0;
        TecladoMayusculas();
        PrintUsr();
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 336 && p.y > 4 && p.y < 31)
      { // Letra z
        p.x = 0;
        p.y = 0;
        Serial.println("z");
        Pass += "z";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 304 && p.y > 4 && p.y < 31)
      { // Letra x
        p.x = 0;
        p.y = 0;
        Serial.println("x");
        Pass += "x";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 244 && p.x < 267 && p.y > 4 && p.y < 31)
      { // Letra c
        p.x = 0;
        p.y = 0;
        Serial.println("c");
        Pass += "c";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 4 && p.y < 31)
      { // Letra v
        p.x = 0;
        p.y = 0;
        Serial.println("v");
        Pass += "v";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 179 && p.x < 203 && p.y > 4 && p.y < 31)
      { // Letra b
        p.x = 0;
        p.y = 0;
        Serial.println("b");
        Pass += "b";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 146 && p.x < 170 && p.y > 4 && p.y < 31)
      { // Letra n
        p.x = 0;
        p.y = 0;
        Serial.println("n");
        Pass += "n";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 114 && p.x < 136 && p.y > 4 && p.y < 31)
      { // Letra m
        p.x = 0;
        p.y = 0;
        Serial.println("m");
        Pass += "m";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 81 && p.x < 102 && p.y > 4 && p.y < 31)
      { // Signo ,
        p.x = 0;
        p.y = 0;
        Serial.println(",");
        Pass += ",";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 4 && p.y < 31)
      { // Signo .
        p.x = 0;
        p.y = 0;
        Serial.println(".");
        Pass += ".";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

    case S_WIFI_MAY_PASS: //////////////////////////////////////// INICIO TECLADO MAYUSCULAS S_WIFI_MAY_PASS ///////////
      if (p.x > 5 && p.x < 38 && p.y > 200 && p.y < 238)
      { // Transición HOME
        estado = M_INICIO;
        p.x = 0;
        p.y = 0;
        tft.fillScreen(BLACK);
        INICIO();
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 133 && p.x < 284 && p.y > 200 && p.y < 220)
      { // Posiciona Cursor en Usr
        estado = S_WIFI_MIN;
        p.x = 0;
        p.y = 0;
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 73 && p.x < 134 && p.y > 200 && p.y < 212)
      { // Tecla Clear
        p.x = 0;
        p.y = 0;
        Serial.println("Clear");
        Pass = "";
        //Borrar();
        //imprime();
        BorradoPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 73 && p.x < 134 && p.y > 160 && p.y < 180)
      { // Tecla Enter
        p.x = 0;
        p.y = 0;

        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 311 && p.x < 337 && p.y > 112 && p.y < 133)
      { // Numero 1
        p.x = 0;
        p.y = 0;
        Serial.println("1");
        Pass += "1";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 112 && p.y < 133)
      { // Numero 2
        p.x = 0;
        p.y = 0;
        Serial.println("2");
        Pass += "2";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 273 && p.y > 112 && p.y < 133)
      { // Numero 3
        p.x = 0;
        p.y = 0;
        Serial.println("3");
        Pass += "3";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 112 && p.y < 133)
      { // Numero 4
        p.x = 0;
        p.y = 0;
        Serial.println("4");
        Pass += "4";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 181 && p.x < 203 && p.y > 112 && p.y < 133)
      { // Numero 5
        p.x = 0;
        p.y = 0;
        Serial.println("5");
        Pass += "5";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 147 && p.x < 170 && p.y > 112 && p.y < 133)
      { // Numero 6
        p.x = 0;
        p.y = 0;
        Serial.println("6");
        Pass += "6";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 134 && p.y > 112 && p.y < 133)
      { // Numero 7
        p.x = 0;
        p.y = 0;
        Serial.println("7");
        Pass += "7";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 112 && p.y < 133)
      { // Numero 8
        p.x = 0;
        p.y = 0;
        Serial.println("8");
        Pass += "8";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 65 && p.y > 112 && p.y < 133)
      { // Numero 9
        p.x = 0;
        p.y = 0;
        Serial.println("9");
        Pass += "9";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 35 && p.y > 112 && p.y < 133)
      { // Numero 0
        p.x = 0;
        p.y = 0;
        Serial.println("0");
        Pass += "0";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 312 && p.x < 334 && p.y > 77 && p.y < 101)
      { // Letra Q
        p.x = 0;
        p.y = 0;
        Serial.println("Q");
        Pass += "Q";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 281 && p.x < 303 && p.y > 77 && p.y < 101)
      { // Letra W
        p.x = 0;
        p.y = 0;
        Serial.println("W");
        Pass += "W";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 249 && p.x < 270 && p.y > 77 && p.y < 101)
      { // Letra E
        p.x = 0;
        p.y = 0;
        Serial.println("E");
        Pass += "E";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 77 && p.y < 101)
      { // Letra R
        p.x = 0;
        p.y = 0;
        Serial.println("R");
        Pass += "R";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 182 && p.x < 203 && p.y > 77 && p.y < 101)
      { // Letra T
        p.x = 0;
        p.y = 0;
        Serial.println("T");
        Pass += "T";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 148 && p.x < 168 && p.y > 77 && p.y < 101)
      { // Letra Y
        p.x = 0;
        p.y = 0;
        Serial.println("Y");
        Pass += "Y";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 113 && p.x < 135 && p.y > 77 && p.y < 101)
      { // Letra U
        p.x = 0;
        p.y = 0;
        Serial.println("U");
        Pass += "U";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 80 && p.x < 100 && p.y > 77 && p.y < 101)
      { // Letra I
        p.x = 0;
        p.y = 0;
        Serial.println("I");
        Pass += "I";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 77 && p.y < 101)
      { // Letra O
        p.x = 0;
        p.y = 0;
        Serial.println("O");
        Pass += "O";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 14 && p.x < 35 && p.y > 77 && p.y < 101)
      { // Letra P
        p.x = 0;
        p.y = 0;
        Serial.println("P");
        Pass += "P";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 334 && p.y > 40 && p.y < 65)
      { // Letra A
        p.x = 0;
        p.y = 0;
        Serial.println("A");
        Pass += "A";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 300 && p.y > 40 && p.y < 65)
      { // Letra S
        p.x = 0;
        p.y = 0;
        Serial.println("S");
        Pass += "S";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 250 && p.x < 270 && p.y > 40 && p.y < 65)
      { // Letra D
        p.x = 0;
        p.y = 0;
        Serial.println("D");
        Pass += "D";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 215 && p.x < 235 && p.y > 40 && p.y < 65)
      { // Letra F
        p.x = 0;
        p.y = 0;
        Serial.println("F");
        Pass += "F";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 180 && p.x < 200 && p.y > 40 && p.y < 65)
      { // Letra G
        p.x = 0;
        p.y = 0;
        Serial.println("G");
        Pass += "G";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 143 && p.x < 170 && p.y > 40 && p.y < 65)
      { // Letra H
        p.x = 0;
        p.y = 0;
        Serial.println("H");
        Pass += "H";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 112 && p.x < 136 && p.y > 40 && p.y < 65)
      { // Letra J
        p.x = 0;
        p.y = 0;
        Serial.println("J");
        Pass += "J";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 78 && p.x < 102 && p.y > 40 && p.y < 65)
      { // Letra K
        p.x = 0;
        p.y = 0;
        Serial.println("K");
        Pass += "K";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 68 && p.y > 40 && p.y < 65)
      { // Letra L
        p.x = 0;
        p.y = 0;
        Serial.println("L");
        Pass += "L";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 13 && p.x < 36 && p.y > 40 && p.y < 65)
      { // Tecla Mayusculas
        estado = S_WIFI_MIN;
        p.x = 0;
        p.y = 0;
        TecladoMinusculas();
        PrintUsr();
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 314 && p.x < 336 && p.y > 4 && p.y < 31)
      { // Letra Z
        p.x = 0;
        p.y = 0;
        Serial.println("Z");
        Pass += "Z";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 280 && p.x < 304 && p.y > 4 && p.y < 31)
      { // Letra X
        p.x = 0;
        p.y = 0;
        Serial.println("X");
        Pass += "X";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 244 && p.x < 267 && p.y > 4 && p.y < 31)
      { // Letra c
        p.x = 0;
        p.y = 0;
        Serial.println("C");
        Pass += "C";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 214 && p.x < 235 && p.y > 4 && p.y < 31)
      { // Letra V
        p.x = 0;
        p.y = 0;
        Serial.println("V");
        Pass += "V";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 179 && p.x < 203 && p.y > 4 && p.y < 31)
      { // Letra B
        p.x = 0;
        p.y = 0;
        Serial.println("B");
        Pass += "B";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 146 && p.x < 170 && p.y > 4 && p.y < 31)
      { // Letra N
        p.x = 0;
        p.y = 0;
        Serial.println("N");
        Pass += "N";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 114 && p.x < 136 && p.y > 4 && p.y < 31)
      { // Letra M
        p.x = 0;
        p.y = 0;
        Serial.println("M");
        Pass += "M";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 81 && p.x < 102 && p.y > 4 && p.y < 31)
      { // Signo -
        p.x = 0;
        p.y = 0;
        Serial.println("-");
        Pass += "-";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }

      if (p.x > 45 && p.x < 67 && p.y > 4 && p.y < 31)
      { // Signo _
        p.x = 0;
        p.y = 0;
        Serial.println("_");
        Pass += "_";
        PrintPass();
        delay(100);
        break; //Rompe el case y regresa al loop
      }        /////////////////////////////////////////////////////
      ////////////////////////////////////////////////// FIN TECLADO MAYUSCULAS S_WIFI_MAY_PASS ///////////////////
    }
  }
}

/////////////Control Termocupla/////////////////////////////

float readThermocouple()
{ /*
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis >= interval))
  {
    previousMillis = currentMillis;
    return thermocouple.readCelsius();
  }*/
}

float readThermocouple1()
{ /*
  unsigned long currentMillis2 = millis();
  if ((currentMillis2 - previousMillis2 >= interva2))
  {
    previousMillis2 = currentMillis2;
    return thermocouple1.readCelsius();
  }*/
}

///////////////////////////////IMPRESION DE TECLADO EN PANTALLA///////////////////////////////////////////////

void BorradoUsr()
{
  tft.fillRect(81, 21, 218, 33, BLACK);
}

void BorradoPass()
{
  tft.fillRect(81, 71, 218, 33, BLACK);
}
void PrintUsr()
{
  ///////////Imprime Caracteres ///////////

  tft.fillRect(81, 21, 218, 33, BLACK);
  // tft.fillRect(37, 50, 110, 44, BLACK);//47
  tft.setCursor(83, 26); //50
  tft.setTextSize(3);
  tft.setTextColor(WHITE, BLACK);
  tft.println(Usr);
}

void PrintPass()
{
  ///////////Imprime Caracteres ///////////

  tft.fillRect(81, 71, 218, 33, BLACK);
  // tft.fillRect(37, 50, 110, 44, BLACK);//47
  tft.setCursor(83, 74); //50
  tft.setTextSize(3);
  tft.setTextColor(WHITE, BLACK);
  tft.println(Pass);
}

void INICIO()
{
  tft.setCursor(7, 10);
  tft.setTextSize(3);
  tft.setTextColor(GOLD, BLACK);
  tft.println("Temperatura Actual");

  tft.setCursor(10, 45);
  tft.setTextSize(8);
  tft.setTextColor(WHITE, BLACK);
  tft.println(temperatura);

  tft.setCursor(7, 115);
  tft.setTextSize(3);
  tft.setTextColor(GOLD, BLACK);
  tft.println("Temperatura Deseada");

  tft.setCursor(10, 153);
  tft.setTextSize(8);
  tft.setTextColor(WHITE, BLACK);
  tft.println(deseada);

  tft.drawRect(241, 151, 101, 61, WHITE);
  tft.fillRect(240, 150, 100, 60, SILVER);
  tft.setCursor(245, 165);
  tft.setTextSize(5);
  tft.setTextColor(WHITE, SILVER);
  tft.println("Set");

  tft.setCursor(7, 225);
  tft.setTextSize(3);
  tft.setTextColor(GOLD, BLACK);
  tft.println("Tiempo de Proceso");

  tft.setCursor(10, 260);
  tft.setTextSize(7);
  tft.setTextColor(WHITE, BLACK);
  tft.println("06:43");

  tft.drawRect(241, 256, 101, 56, WHITE);
  tft.fillRect(240, 255, 100, 55, SILVER);
  tft.setCursor(245, 267);
  tft.setTextSize(5);
  tft.setTextColor(WHITE, SILVER);
  tft.println("Set");

  tft.drawRect(241, 256, 101, 56, WHITE);
  tft.fillRect(240, 255, 100, 55, SILVER);
  tft.setCursor(270, 267);
  tft.setTextSize(5);
  tft.setTextColor(WHITE, SILVER);
  tft.println("Go");
}

void TecladoMinusculas()
{
  ///////////Usr & Pass///////////
  tft.drawRect(80, 20, 220, 35, WHITE);
  tft.setCursor(5, 30);
  tft.setTextSize(3);
  tft.setTextColor(WHITE, BLACK);
  tft.println("Usr");

  tft.drawRect(80, 70, 220, 35, WHITE);
  tft.setCursor(5, 80);
  tft.setTextSize(3);
  tft.setTextColor(WHITE, BLACK);
  tft.println("Pass");

  ///////////Tecla Retroceso///////////

  tft.drawRect(430, 10, 45, 45, WHITE);
  tft.fillRect(432, 12, 40, 40, WHITE);
  tft.setCursor(435, 20);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("<-");

  ///////////Tecla Borrado/////////////

  tft.drawRect(315, 25, 95, 30, WHITE);
  tft.fillRect(317, 27, 90, 25, WHITE);
  tft.setCursor(318, 29);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("Clear");

  //////////Tecla Enter////////////////

  tft.drawRect(315, 75, 95, 30, WHITE);
  tft.fillRect(317, 77, 90, 25, WHITE);
  tft.setCursor(318, 79);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("Enter");

  ////////////Primera Linea////////////

  tft.drawRect(5, 130, 45, 45, WHITE);
  tft.fillRect(7, 132, 40, 40, WHITE);
  tft.setCursor(20, 142);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("1");

  tft.drawRect(52, 130, 45, 45, WHITE);
  tft.fillRect(54, 132, 40, 40, WHITE);
  tft.setCursor(67, 142);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("2");

  tft.drawRect(99, 130, 45, 45, WHITE);
  tft.fillRect(101, 132, 40, 40, WHITE);
  tft.setCursor(112, 142); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("3");

  tft.drawRect(146, 130, 45, 45, WHITE);
  tft.fillRect(148, 132, 40, 40, WHITE);
  tft.setCursor(160, 142); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("4");

  tft.drawRect(193, 130, 45, 45, WHITE); //47
  tft.fillRect(195, 132, 40, 40, WHITE); //47
  tft.setCursor(207, 142);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("5");

  tft.drawRect(240, 130, 45, 45, WHITE); //47
  tft.fillRect(242, 132, 40, 40, WHITE); //47
  tft.setCursor(255, 142);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("6");

  tft.drawRect(287, 130, 45, 45, WHITE); //47
  tft.fillRect(289, 132, 40, 40, WHITE); //47
  tft.setCursor(302, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("7");

  tft.drawRect(334, 130, 45, 45, WHITE); //47
  tft.fillRect(336, 132, 40, 40, WHITE); //47
  tft.setCursor(349, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("8");

  tft.drawRect(381, 130, 45, 45, WHITE); //47
  tft.fillRect(383, 132, 40, 40, WHITE); //47
  tft.setCursor(396, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("9");

  tft.drawRect(428, 130, 45, 45, WHITE); //47
  tft.fillRect(430, 132, 40, 40, WHITE); //47
  tft.setCursor(443, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("0");

  ////////////Segunda Linea////////////

  tft.drawRect(5, 177, 45, 45, WHITE);
  tft.fillRect(7, 179, 40, 40, WHITE);
  tft.setCursor(20, 185);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("q");

  tft.drawRect(52, 177, 45, 45, WHITE); //47
  tft.fillRect(54, 179, 40, 40, WHITE); //47
  tft.setCursor(67, 185);               //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("w");

  tft.drawRect(99, 177, 45, 45, WHITE);
  tft.fillRect(101, 179, 40, 40, WHITE);
  tft.setCursor(112, 185); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("e");

  tft.drawRect(146, 177, 45, 45, WHITE);
  tft.fillRect(148, 179, 40, 40, WHITE);
  tft.setCursor(160, 185); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("r");

  tft.drawRect(193, 177, 45, 45, WHITE); //47
  tft.fillRect(195, 179, 40, 40, WHITE); //47
  tft.setCursor(207, 185);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("t");

  tft.drawRect(240, 177, 45, 45, WHITE); //47
  tft.fillRect(242, 179, 40, 40, WHITE); //47
  tft.setCursor(255, 185);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("y");

  tft.drawRect(287, 177, 45, 45, WHITE); //47
  tft.fillRect(289, 179, 40, 40, WHITE); //47
  tft.setCursor(302, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("u");

  tft.drawRect(334, 177, 45, 45, WHITE); //47
  tft.fillRect(336, 179, 40, 40, WHITE); //47
  tft.setCursor(349, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("i");

  tft.drawRect(381, 177, 45, 45, WHITE); //47
  tft.fillRect(383, 179, 40, 40, WHITE); //47
  tft.setCursor(396, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("o");

  tft.drawRect(428, 177, 45, 45, WHITE); //47
  tft.fillRect(430, 179, 40, 40, WHITE); //47
  tft.setCursor(443, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("p");

  ///////////Tercera Linea////////////

  tft.drawRect(5, 224, 45, 45, WHITE);
  tft.fillRect(7, 226, 40, 40, WHITE);
  tft.setCursor(20, 232);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("a");

  tft.drawRect(52, 224, 45, 45, WHITE); //47
  tft.fillRect(54, 226, 40, 40, WHITE); //47
  tft.setCursor(67, 232);               //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("s");

  tft.drawRect(99, 224, 45, 45, WHITE);
  tft.fillRect(101, 226, 40, 40, WHITE);
  tft.setCursor(112, 232); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("d");

  tft.drawRect(146, 224, 45, 45, WHITE);
  tft.fillRect(148, 226, 40, 40, WHITE);
  tft.setCursor(160, 232); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("f");

  tft.drawRect(193, 224, 45, 45, WHITE); //47
  tft.fillRect(195, 226, 40, 40, WHITE); //47
  tft.setCursor(207, 232);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("g");

  tft.drawRect(240, 224, 45, 45, WHITE); //47
  tft.fillRect(242, 226, 40, 40, WHITE); //47
  tft.setCursor(255, 232);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("h");

  tft.drawRect(287, 224, 45, 45, WHITE); //47
  tft.fillRect(289, 226, 40, 40, WHITE); //47
  tft.setCursor(302, 232);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("j");

  tft.drawRect(334, 224, 45, 45, WHITE); //47
  tft.fillRect(336, 226, 40, 40, WHITE); //47
  tft.setCursor(349, 232);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("k");

  tft.drawRect(381, 224, 45, 45, WHITE); //47
  tft.fillRect(383, 226, 40, 40, WHITE); //47
  tft.setCursor(396, 232);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("l");

  tft.drawRect(428, 224, 45, 45, WHITE); //47
  tft.fillRect(430, 226, 40, 40, WHITE); //47
  tft.setCursor(443, 232);               //47
  tft.setTextSize(2);
  tft.setTextColor(BLACK, WHITE);
  tft.println("B/M");

  ///////////Cuarta Linea//////////////

  tft.drawRect(5, 271, 45, 45, WHITE);
  tft.fillRect(7, 273, 40, 40, WHITE);
  tft.setCursor(20, 279);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("z");

  tft.drawRect(52, 271, 45, 45, WHITE); //47
  tft.fillRect(54, 273, 40, 40, WHITE); //47
  tft.setCursor(67, 279);               //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("x");

  tft.drawRect(99, 271, 45, 45, WHITE);
  tft.fillRect(101, 273, 40, 40, WHITE);
  tft.setCursor(112, 279); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("c");

  tft.drawRect(146, 271, 45, 45, WHITE);
  tft.fillRect(148, 273, 40, 40, WHITE);
  tft.setCursor(160, 279); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("v");

  tft.drawRect(193, 271, 45, 45, WHITE); //47
  tft.fillRect(195, 273, 40, 40, WHITE); //47
  tft.setCursor(207, 279);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("b");

  tft.drawRect(240, 271, 45, 45, WHITE); //47
  tft.fillRect(242, 273, 40, 40, WHITE); //47
  tft.setCursor(255, 279);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("n");

  tft.drawRect(287, 271, 45, 45, WHITE); //47
  tft.fillRect(289, 273, 40, 40, WHITE); //47
  tft.setCursor(302, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("m");

  tft.drawRect(334, 271, 45, 45, WHITE); //47
  tft.fillRect(336, 273, 40, 40, WHITE); //47
  tft.setCursor(349, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println(",");

  tft.drawRect(381, 271, 45, 45, WHITE); //47
  tft.fillRect(383, 273, 40, 40, WHITE); //47
  tft.setCursor(396, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println(".");

  tft.drawRect(428, 271, 45, 45, WHITE); //47
  tft.fillRect(430, 273, 40, 40, WHITE); //47
  tft.setCursor(443, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("_");
}

void TecladoMayusculas()
{
  ///////////Tecla Retroceso///////////

  tft.drawRect(430, 10, 45, 45, WHITE); //47
  tft.fillRect(432, 12, 40, 40, WHITE); //47
  tft.setCursor(435, 20);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("<-");

  ////////////Primera Linea////////////

  tft.drawRect(5, 130, 45, 45, WHITE);
  tft.fillRect(7, 132, 40, 40, WHITE);
  tft.setCursor(20, 142);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("1");

  tft.drawRect(52, 130, 45, 45, WHITE); //47
  tft.fillRect(54, 132, 40, 40, WHITE); //47
  tft.setCursor(67, 142);               //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("2");

  tft.drawRect(99, 130, 45, 45, WHITE);
  tft.fillRect(101, 132, 40, 40, WHITE);
  tft.setCursor(112, 142); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("3");

  tft.drawRect(146, 130, 45, 45, WHITE);
  tft.fillRect(148, 132, 40, 40, WHITE);
  tft.setCursor(160, 142); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("4");

  tft.drawRect(193, 130, 45, 45, WHITE); //47
  tft.fillRect(195, 132, 40, 40, WHITE); //47
  tft.setCursor(207, 142);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("5");

  tft.drawRect(240, 130, 45, 45, WHITE); //47
  tft.fillRect(242, 132, 40, 40, WHITE); //47
  tft.setCursor(255, 142);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("6");

  tft.drawRect(287, 130, 45, 45, WHITE); //47
  tft.fillRect(289, 132, 40, 40, WHITE); //47
  tft.setCursor(302, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("7");

  tft.drawRect(334, 130, 45, 45, WHITE); //47
  tft.fillRect(336, 132, 40, 40, WHITE); //47
  tft.setCursor(349, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("8");

  tft.drawRect(381, 130, 45, 45, WHITE); //47
  tft.fillRect(383, 132, 40, 40, WHITE); //47
  tft.setCursor(396, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("9");

  tft.drawRect(428, 130, 45, 45, WHITE); //47
  tft.fillRect(430, 132, 40, 40, WHITE); //47
  tft.setCursor(443, 142);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("0");

  ////////////Segunda Linea////////////

  tft.drawRect(5, 177, 45, 45, WHITE);
  tft.fillRect(7, 179, 40, 40, WHITE);
  tft.setCursor(20, 185);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("Q");

  tft.drawRect(52, 177, 45, 45, WHITE); //47
  tft.fillRect(54, 179, 40, 40, WHITE); //47
  tft.setCursor(67, 185);               //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("W");

  tft.drawRect(99, 177, 45, 45, WHITE);
  tft.fillRect(101, 179, 40, 40, WHITE);
  tft.setCursor(112, 185); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("E");

  tft.drawRect(146, 177, 45, 45, WHITE);
  tft.fillRect(148, 179, 40, 40, WHITE);
  tft.setCursor(160, 185); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("R");

  tft.drawRect(193, 177, 45, 45, WHITE); //47
  tft.fillRect(195, 179, 40, 40, WHITE); //47
  tft.setCursor(207, 185);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("T");

  tft.drawRect(240, 177, 45, 45, WHITE); //47
  tft.fillRect(242, 179, 40, 40, WHITE); //47
  tft.setCursor(255, 185);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("Y");

  tft.drawRect(287, 177, 45, 45, WHITE); //47
  tft.fillRect(289, 179, 40, 40, WHITE); //47
  tft.setCursor(302, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("U");

  tft.drawRect(334, 177, 45, 45, WHITE); //47
  tft.fillRect(336, 179, 40, 40, WHITE); //47
  tft.setCursor(349, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("I");

  tft.drawRect(381, 177, 45, 45, WHITE); //47
  tft.fillRect(383, 179, 40, 40, WHITE); //47
  tft.setCursor(396, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("O");

  tft.drawRect(428, 177, 45, 45, WHITE); //47
  tft.fillRect(430, 179, 40, 40, WHITE); //47
  tft.setCursor(443, 185);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("P");

  ///////////Tercera Linea////////////

  tft.drawRect(5, 224, 45, 45, WHITE);
  tft.fillRect(7, 226, 40, 40, WHITE);
  tft.setCursor(20, 232);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("A");

  tft.drawRect(52, 224, 45, 45, WHITE); //47
  tft.fillRect(54, 226, 40, 40, WHITE); //47
  tft.setCursor(67, 232);               //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("S");

  tft.drawRect(99, 224, 45, 45, WHITE);
  tft.fillRect(101, 226, 40, 40, WHITE);
  tft.setCursor(112, 232); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("D");

  tft.drawRect(146, 224, 45, 45, WHITE);
  tft.fillRect(148, 226, 40, 40, WHITE);
  tft.setCursor(160, 232); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("F");

  tft.drawRect(193, 224, 45, 45, WHITE); //47
  tft.fillRect(195, 226, 40, 40, WHITE); //47
  tft.setCursor(207, 232);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("G");

  tft.drawRect(240, 224, 45, 45, WHITE); //47
  tft.fillRect(242, 226, 40, 40, WHITE); //47
  tft.setCursor(255, 232);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("H");

  tft.drawRect(287, 224, 45, 45, WHITE); //47
  tft.fillRect(289, 226, 40, 40, WHITE); //47
  tft.setCursor(302, 232);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("J");

  tft.drawRect(334, 224, 45, 45, WHITE); //47
  tft.fillRect(336, 226, 40, 40, WHITE); //47
  tft.setCursor(349, 232);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("K");

  tft.drawRect(381, 224, 45, 45, WHITE); //47
  tft.fillRect(383, 226, 40, 40, WHITE); //47
  tft.setCursor(396, 232);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("L");

  tft.drawRect(428, 224, 45, 45, WHITE); //47
  tft.fillRect(430, 226, 40, 40, WHITE); //47
  tft.setCursor(443, 232);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("");

  ///////////Cuarta Linea//////////////

  tft.drawRect(5, 271, 45, 45, WHITE);
  tft.fillRect(7, 273, 40, 40, WHITE);
  tft.setCursor(20, 279);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("Z");

  tft.drawRect(52, 271, 45, 45, WHITE); //47
  tft.fillRect(54, 273, 40, 40, WHITE); //47
  tft.setCursor(67, 279);               //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("X");

  tft.drawRect(99, 271, 45, 45, WHITE);
  tft.fillRect(101, 273, 40, 40, WHITE);
  tft.setCursor(112, 279); //45
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("C");

  tft.drawRect(146, 271, 45, 45, WHITE);
  tft.fillRect(148, 273, 40, 40, WHITE);
  tft.setCursor(160, 279); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("V");

  tft.drawRect(193, 271, 45, 45, WHITE); //47
  tft.fillRect(195, 273, 40, 40, WHITE); //47
  tft.setCursor(207, 279);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("B");

  tft.drawRect(240, 271, 45, 45, WHITE); //47
  tft.fillRect(242, 273, 40, 40, WHITE); //47
  tft.setCursor(255, 279);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("N");

  tft.drawRect(287, 271, 45, 45, WHITE); //47
  tft.fillRect(289, 273, 40, 40, WHITE); //47
  tft.setCursor(302, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("M");

  tft.drawRect(334, 271, 45, 45, WHITE); //47
  tft.fillRect(336, 273, 40, 40, WHITE); //47
  tft.setCursor(349, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("-");

  tft.drawRect(381, 271, 45, 45, WHITE); //47
  tft.fillRect(383, 273, 40, 40, WHITE); //47
  tft.setCursor(396, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("_");

  tft.drawRect(428, 271, 45, 45, WHITE); //47
  tft.fillRect(430, 273, 40, 40, WHITE); //47
  tft.setCursor(443, 279);               //47
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("_");
}

void Borrar()
{
  tft.fillRect(41, 56, 150, 52, BLACK); //47
}
void imprime()
{
  ///////////Imprime Caracteres ///////////

  tft.drawRect(40, 55, 155, 55, WHITE);
  // tft.fillRect(37, 50, 110, 44, BLACK);//47
  tft.setCursor(45, 60); //50
  tft.setTextSize(6);
  tft.setTextColor(WHITE, BLACK);
  tft.println(SetTemp);
}

void TecladoNumerico()
{
  ///////////Imprime Caracteres ///////////

  tft.drawRect(40, 55, 155, 55, WHITE);
  //tft.fillRect(32, 62, 40, 40, WHITE);
  tft.setCursor(45, 60);
  tft.setTextSize(6);
  tft.setTextColor(WHITE, BLACK);
  tft.println(SetTemp);

  ///////////Tecla Borrado///////////

  tft.drawRect(200, 65, 95, 30, WHITE);
  tft.fillRect(202, 67, 90, 25, WHITE);
  tft.setCursor(203, 69);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("Clear");

  ///////////Tecla Retroceso///////////

  tft.drawRect(430, 10, 45, 45, WHITE);
  tft.fillRect(432, 12, 40, 40, WHITE);
  tft.setCursor(435, 20);
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("<-");

  ////////////Primera Linea////////////

  tft.drawRect(52, 130, 45, 45, WHITE);
  tft.fillRect(54, 132, 40, 40, WHITE);
  tft.setCursor(66, 142); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("7");

  tft.drawRect(99, 130, 45, 45, WHITE);  //47
  tft.fillRect(101, 132, 40, 40, WHITE); //47
  tft.setCursor(113, 142);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("8");

  tft.drawRect(146, 130, 45, 45, WHITE); //47
  tft.fillRect(148, 132, 40, 40, WHITE); //47
  tft.setCursor(161, 142);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("9");

  ////////////Segunda Linea////////////

  tft.drawRect(52, 177, 45, 45, WHITE);
  tft.fillRect(54, 179, 40, 40, WHITE);
  tft.setCursor(66, 185); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("4");

  tft.drawRect(99, 177, 45, 45, WHITE);  //47
  tft.fillRect(101, 179, 40, 40, WHITE); //47
  tft.setCursor(113, 185);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("5");

  tft.drawRect(146, 177, 45, 45, WHITE); //47
  tft.fillRect(148, 179, 40, 40, WHITE); //47
  tft.setCursor(161, 185);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("6");

  ///////////Tercera Linea////////////

  tft.drawRect(52, 224, 45, 45, WHITE);
  tft.fillRect(54, 226, 40, 40, WHITE);
  tft.setCursor(66, 232); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("1");

  tft.drawRect(99, 224, 45, 45, WHITE);  //47
  tft.fillRect(101, 226, 40, 40, WHITE); //47
  tft.setCursor(113, 232);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("2");

  tft.drawRect(146, 224, 45, 45, WHITE); //47
  tft.fillRect(148, 226, 40, 40, WHITE); //47
  tft.setCursor(161, 232);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("3");

  ///////////Cuarta Linea//////////////

  tft.drawRect(99, 271, 45, 45, WHITE);  //47
  tft.fillRect(101, 273, 40, 40, WHITE); //47
  tft.setCursor(113, 279);               //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("0");

  tft.drawRect(146, 271, 45, 45, WHITE);
  tft.fillRect(148, 273, 40, 40, WHITE);
  tft.setCursor(152, 279); //50
  tft.setTextSize(3);
  tft.setTextColor(BLACK, WHITE);
  tft.println("Ok");
}
/*
ISR(PCINT0_vect)
{
  cli();
  Serial.println("disparo");
  recieveStructure((byte *)&message, sizeof(message));
  //pin A15 (vector interrupt) conectado al pin 2 del 328p.
  //sei();
}*/
/*
void interrupcion()
{
  cli();
  Serial.println("disparo");
  recieveStructure((byte *)&message, sizeof(message));
  //pin A15 (vector interrupt) conectado al pin 2 del 328p.
  sei();
}*/