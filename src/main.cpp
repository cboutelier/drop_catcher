#include <Arduino.h>
#include <Nextion.h>
#include <Preferences.h>

// 41 tx
// 40 rx

// Define direct port access.
#define dwon(port, pin) (port |= _BV(pin))
#define dwoff(port, pin) (port &= ~(_BV(pin)))

#define SICK_OUTPUT GPIO_NUM_15
#define TRIGGER_OUTPUT GPIO_NUM_32

Preferences preferences;

// timestamp of the first drop detected, used to prevent bounces.
unsigned long firstDropTimestamp = 0;
bool dropDetected = false;

// Nextion tutorial variables
uint32_t next, myInt = 0;
uint32_t triggerPulseWidth = 10;

// Trigger flash or camera
bool triggerFlash = true;
uint32_t chuteHeight = 0;
uint32_t sleepDelay = 0;

#define ledPin 2

// GUI buttons and texts
NexPage homePage = NexPage(0, 0, "home");
NexPage settingsPage = NexPage(0, 0, "settings");

// Home page
NexButton homeSettingsButton = NexButton(0, 1, "home_settings");

// Settings page
NexButton settingsBackButton = NexButton(1, 2, "settings_back");
NexNumber triggerPulseWidthField = NexNumber(1, 4, "trigger_width");
NexNumber heightField = NexNumber(1, 9, "distance_mm");
NexNumber sleepMinField = NexNumber(1, 7, "sleep_min");
NexCheckbox triggerField = NexCheckbox(1,10,"c0");
NexButton settingsValidateButton = NexButton(1, 5, "validate");

NexTouch *nex_listen_list[] = {
    &homeSettingsButton,
    &settingsBackButton,
    &heightField,
    &sleepMinField,
    &settingsValidateButton,
    &triggerPulseWidthField, NULL};

long lastAction = 0;

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

  preferences.begin("drop", false);
  preferences.putUInt("TRIGGER_MS", triggerPulseWidth);

  preferences.putUInt("CHUTE_MM", chuteHeight);
  preferences.putBool("TRIGGER_FLASH", triggerFlash);
  preferences.putUInt("SLEEP", sleepDelay);

  preferences.end();

  homePage.show();
  lastAction = millis();
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
  preferences.begin("drop", true);
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

  Serial.println(sleepDelay);
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

  // Declare the GPIO_22( pin 39) as output.
  pinMode(TRIGGER_OUTPUT, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(SICK_OUTPUT), doInterrupt, RISING);

  homeSettingsButton.attachPush(p0_b0_Push, &homeSettingsButton);
    
  settingsValidateButton.attachPop(handleSettingsChanges, &settingsValidateButton);

  next = millis();
  lastAction = millis();
  sendCommand("thup=1");
}

void loop()
{

  nexLoop(nex_listen_list);

  if (dropDetected)
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
  }
}
