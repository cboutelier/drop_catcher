#include <Arduino.h>

// Define direct port access.
#define dwon(port, pin) (port |= _BV(pin))
#define dwoff(port, pin) (port &= ~(_BV(pin)))

#define SICK_OUTPUT GPIO_NUM_15
#define TRIGGER_OUTPUT GPIO_NUM_32

// timestamp of the first drop detected, used to prevent bounces.
unsigned long firstDropTimestamp = 0;
bool dropDetected = false;

// put function declarations here:
int myFunction(int, int);

void doInterrupt()
{
  if (firstDropTimestamp == 0)
  {
    Serial.println("Drop detected");
    firstDropTimestamp = millis();
    dropDetected = true;
  }
}

void setup()
{
  Serial.begin(9600);
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  // Declare the GPIO_22( pin 39) as output.
  pinMode(TRIGGER_OUTPUT, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(SICK_OUTPUT), doInterrupt, RISING);
}

void loop()
{
  // put your main code here, to run repeatedly:
  // Hello world
  if (dropDetected)
  {
    //delayMicroseconds(500);
    delay(35);
    digitalWrite(TRIGGER_OUTPUT, HIGH);
    delay(50);
    digitalWrite(TRIGGER_OUTPUT, LOW);
    dropDetected = false;
    delay(3000);
    firstDropTimestamp =  0;
    Serial.println("Ready for next drop");
  }
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}