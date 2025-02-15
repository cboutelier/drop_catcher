#include <Arduino.h>
#include <Nextion.h>
#include <Preferences.h>

// 41 tx
// 40 rx

// Define direct port access.
#define dwon(port, pin) (port |= _BV(pin))
#define dwoff(port, pin) (port &= ~(_BV(pin)))

#define SICK_OUTPUT GPIO_NUM_21
#define TRIGGER_OUTPUT GPIO_NUM_18
#define VALVE_OUTPUT GPIO_NUM_5

Preferences preferences;

// timestamp of the first drop detected, used to prevent bounces.
unsigned long firstDropTimestamp = 0;
bool dropDetected = false;

uint32_t next = 0;
uint32_t triggerPulseWidth = 10;
uint32_t valvePulseWidth = 2;

// Trigger flash or camera
bool triggerFlash = true;
uint32_t chuteHeight = 0;
uint32_t sleepDelay = 0;

bool countDownStarted = false;
uint32_t countDownStart = 10;
uint32_t countDownCurrent = countDownStart;

#define ledPin 2

// GUI buttons and texts
NexPage homePage = NexPage(0, 0, "home");
NexPage settingsPage = NexPage(0, 0, "settings");

// Home page
NexButton homeSettingsButton = NexButton(0, 1, "home_settings");

// Settings page
NexButton settingsBackButton = NexButton(1, 2, "settings_back");
NexNumber triggerPulseWidthField = NexNumber(1, 4, "trigger_width");
NexNumber valvePulseWidthField = NexNumber(1, 11, "valve_width");
NexNumber heightField = NexNumber(1, 9, "distance_mm");
NexNumber sleepMinField = NexNumber(1, 7, "sleep_min");
NexCheckbox triggerField = NexCheckbox(1, 10, "c0");
NexNumber countDownField = NexNumber(1, 15, "countdown");
NexButton settingsValidateButton = NexButton(1, 5, "settings_ok");

NexTouch *nex_listen_list[] = {
    &homeSettingsButton,
    &settingsBackButton,
    &heightField,
    &sleepMinField,
    &settingsValidateButton,
    &triggerPulseWidthField,
    &valvePulseWidthField,
    &countDownField,
    NULL};

long lastAction = 0;

//void releaseDrops(uint32_t pulseWidth, uint32_t triggerDelay, uint8_t numberofDrops, uint32_t interval);
void releaseDrop(uint32_t pulsewidth);

void p0_b0_Push(void *ptr)
{
  Serial.println("Going to settings page");
  // Sending the values

  // Display the page
  settingsPage.show();
  triggerPulseWidthField.setValue(triggerPulseWidth);
  sleepMinField.setValue(sleepDelay);
  heightField.setValue(chuteHeight);
  triggerField.setValue(triggerFlash);
  countDownField.setValue(countDownStart);
}

void handleSettingsChanges(void *ptr)
{

  uint32_t *pointerToPulse = &triggerPulseWidth;
  if (!triggerPulseWidthField.getValue(pointerToPulse))
  {
    Serial.println("Could not get the trigger pulse width");
  }
  if (!heightField.getValue(&chuteHeight))
  {
    Serial.println("Could not get the chute height");
  }
  if (!sleepMinField.getValue(&sleepDelay))
  {
    Serial.println("Could not get the sleeping delay");
  }
  else
  {
    Serial.print("Sleeping delay:");
    Serial.println(sleepDelay);
  }

  if (!valvePulseWidthField.getValue(&valvePulseWidth))
  {
    Serial.println("Could not get the valve pulse width");
  }
  countDownField.getValue(&countDownStart);

  preferences.begin("drop", false);
  preferences.putUInt("TRIGGER_MS", triggerPulseWidth);

  preferences.putUInt("CHUTE_MM", chuteHeight);
  preferences.putBool("TRIGGER_FLASH", triggerFlash);
  preferences.putUInt("SLEEP", sleepDelay);
  preferences.putUInt("VALVE_MS", valvePulseWidth);
  preferences.putUInt("COUNT_DOWN", countDownStart);

  preferences.end();

  homePage.show();
  lastAction = millis();

  if (!triggerFlash)
  {
    attachInterrupt(digitalPinToInterrupt(SICK_OUTPUT), doInterrupt, RISING);
  }

  digitalWrite(VALVE_OUTPUT, HIGH);
  delay(valvePulseWidth);
  digitalWrite(VALVE_OUTPUT, LOW);
}

void doInterrupt()
{
  if (firstDropTimestamp == 0)
  {
    Serial.println("Drop detected");
    firstDropTimestamp = millis();
    dropDetected = true;
  }
}

void loadValues()
{
  Serial.println("------------------------------");
  bool hasPref = preferences.begin("drop", false);
  if (hasPref)
  {
    triggerPulseWidth = preferences.getUInt("TRIGGER_MS", 50);

    Serial.print("Trigger width:");
    Serial.println(triggerPulseWidth);

    chuteHeight = preferences.getUInt("CHUTE_MM", 600);
    Serial.print("Chute height:");
    Serial.println(chuteHeight);

    sleepDelay = preferences.getUInt("SLEEP", 5);
    Serial.print("Sleep delay:");
    Serial.println(sleepDelay);

    triggerFlash = preferences.getBool("TRIGGER_FLASH", true);
    Serial.print("Trigger flash:");
    Serial.println(triggerFlash);

    valvePulseWidth = preferences.getUInt("VALVE_MS", 1);
    countDownStart = preferences.getUInt("COUNT_DOWN", 7);
    Serial.print("Count down:");
    Serial.println(countDownStart);
  }
  else
  {
    Serial.println("no pref");
  }
  if (sleepDelay == 0)
  {
    sleepDelay = 3;
  }
  Serial.println("------------------------------");
  preferences.end();
}

void setup()
{
  Serial.begin(9600);
  delay(1000);
  loadValues();

  nexInit();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(VALVE_OUTPUT, OUTPUT);
  digitalWrite(VALVE_OUTPUT, LOW);

  pinMode(TRIGGER_OUTPUT, OUTPUT);

  homeSettingsButton.attachPush(p0_b0_Push, &homeSettingsButton);

  settingsValidateButton.attachPop(handleSettingsChanges, &settingsValidateButton);

  next = millis();
  lastAction = millis();
  sendCommand("thup=1");
}

void OnCountDownEvent()
{
  if (countDownCurrent > 0)
  {
    Serial.println(countDownCurrent);
    countDownCurrent--;
    sleep(1);
  }
  else
  {
    Serial.println("Ding!");
    countDownStarted = false;
    releaseDrop(valvePulseWidth);
  }
}

void loop()
{

  nexLoop(nex_listen_list);

  if (countDownStarted)
  {
    OnCountDownEvent();
  }

  if (Serial.available() > 0)
  {
    // read the incoming byte:
    int v = Serial.read();
    Serial.print("I received: ");
    Serial.println(v, DEC);
    if (v == 115)
    {
      countDownStarted = true;
      Serial.println("Count down started");
    }
  }

  /* if (dropDetected)
   {
     // delayMicroseconds(500);
     delay(35);
     digitalWrite(TRIGGER_OUTPUT, HIGH);
     delay(triggerPulseWidth);
     digitalWrite(TRIGGER_OUTPUT, LOW);
     dropDetected = false;
     delay(3000);
     firstDropTimestamp = 0;
     Serial.println("Ready for next drop");
   }

   if (lastAction + 1000 * 60 * sleepDelay < millis())
   {
     sendCommand("sleep=1");
     lastAction = millis();
   }*/
}

void releaseDrop(uint32_t pulsewidth)
{
  digitalWrite(TRIGGER_OUTPUT, HIGH);
  delay(triggerPulseWidth);
  digitalWrite(TRIGGER_OUTPUT, LOW);
}