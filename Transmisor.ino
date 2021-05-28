////Variables lectura termocupla
unsigned long currentMillis = millis();
unsigned long previousMillis = 0;
const long TiempoCiclo = 2500;

struct servoMoveMessage
{
  volatile int servoNum;
  volatile int positionGoal;
  volatile float interval;
};

struct servoMoveMessage message;

void sendStructure(byte *structurePointer, int structureLength)
{
  digitalWrite(15, LOW);

  digitalWrite(20, LOW);
  delay(10);
  digitalWrite(20, HIGH);

  Serial3.write(structurePointer, structureLength);

  digitalWrite(15, HIGH);
}

void recieveStructure(byte *structurePointer, int structureLength)
{
  if (Serial.available() < sizeof(message))
    return;
  Serial.readBytes(structurePointer, structureLength);
  //Serial.println("message.servoNum");
  Serial.println(message.positionGoal);
  Serial.println(message.interval);
}

void setup()
{
  Serial.begin(9600);
  Serial3.begin(9600);

  pinMode(20, OUTPUT);    //Trigger Control pin
  digitalWrite(20, HIGH);

  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  message.servoNum = 10;
  message.positionGoal = 1200;
  message.interval = 2.5;

  sendStructure((byte *)&message, sizeof(message));
}

void loop()
{
  sendStructure((byte *)&message, sizeof(message));
  delay(250);
}

void envioDatos()
{ 
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis >= TiempoCiclo))
  {
    previousMillis = currentMillis;    
  }
}
