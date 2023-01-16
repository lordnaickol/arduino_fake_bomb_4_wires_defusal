// include the library code:
#include <LiquidCrystal.h>

// Connections to the circuit: LCD screen
const int LCD_RS_PIN = 12;
const int LCD_ENABLE_PIN = 11;
const int LCD_DATA_PIN_D4 = 5;
const int LCD_DATA_PIN_D5 = 4;
const int LCD_DATA_PIN_D6 = 3;
const int LCD_DATA_PIN_D7 = 2;
const int BUTTONTIME = 7;
const int LCD_BACKLIGHT_RED = 8;
const int LCD_BACKLIGHT_GREEN = 9;
const int LCD_BACKLIGHT_BLUE = 10;

LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_DATA_PIN_D4, LCD_DATA_PIN_D5, LCD_DATA_PIN_D6, LCD_DATA_PIN_D7);

const int LCD_ROWS = 2;
const int LCD_COLS = 16;

int state; //0=waiting, 1=Armando, 2=Ready, 3
int x;
bool bLoaded;
bool bFirstLoad;
bool cableError;

const int TOTAL_WIRES = 4;
const int CUTTABLE_WIRES[TOTAL_WIRES] = {A0, A1, A2, A3};
const bool WIRES_TO_CUT[TOTAL_WIRES] = { 1, 0, 1, 0};

unsigned long TOTAL_TIME;
const double TimeHours = 0.5;

unsigned long remainingTime = TOTAL_TIME;
unsigned long lastTimeUpdatedAt = 0;
bool wireStates[TOTAL_WIRES];
byte currentColor = 0;
const int time_multiplier = 50;
int masterKeyPin = 13;

#define COLS 16
#define ROWS 2
#define MIN 0
#define MAX 100

int counter;
int textStartPos = 0;
int percStartPos = 12;

byte customChar1[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte customChar2[] = {
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100
};
byte customChar3[] = {
  B00000,
  B00000,
  B11001,
  B11010,
  B00100,
  B01011,
  B10011,
  B00000
};

void setDefaultBG() {
  if (currentColor == 0) return;
  digitalWrite(LCD_BACKLIGHT_RED, 255);
  digitalWrite(LCD_BACKLIGHT_GREEN, 255);
  digitalWrite(LCD_BACKLIGHT_BLUE, 255);
  currentColor = 0;
}

void setRedBG() {
  if (currentColor == 1) return;
  digitalWrite(LCD_BACKLIGHT_RED, 255);
  digitalWrite(LCD_BACKLIGHT_GREEN, 0);
  digitalWrite(LCD_BACKLIGHT_BLUE, 0);
  currentColor = 1;
}

void setGreenBG() {
  if (currentColor == 2) return;
  digitalWrite(LCD_BACKLIGHT_RED, 0);
  digitalWrite(LCD_BACKLIGHT_GREEN, 255);
  digitalWrite(LCD_BACKLIGHT_BLUE, 0);
  currentColor = 2;
}

void setBlueBG() {
  if (currentColor == 1) return;
  digitalWrite(LCD_BACKLIGHT_RED, 0);
  digitalWrite(LCD_BACKLIGHT_GREEN, 0);
  digitalWrite(LCD_BACKLIGHT_BLUE, 255);
  currentColor = 1;
}


void formatTime(unsigned long t, char str[13]) {
  unsigned long totalSeconds = t / 1000;
  unsigned long ms = t % 1000;
  unsigned long h = totalSeconds / 3600;
  unsigned long m = (totalSeconds - 3600 * h) / 60;
  unsigned long s = totalSeconds - 3600 * h - 60 * m;

  snprintf(str, 13, "%02lu:%02lu:%02lu", h, m, s);
}

void printBar(int val) {
  char lcdBuf[COLS];
  int mark = map(val, MIN, MAX, 0, COLS);
  mark = constrain(mark, 0, COLS);

  memset(lcdBuf, byte(1), mark);
  memset(lcdBuf + mark, ' ', COLS - mark);
  lcd.print(lcdBuf);
}
void clearBar() {
  char lcdBuf[COLS];
  int mark = map(0, MIN, MAX, 0, COLS);
  mark = constrain(mark, 0, COLS);

  memset(lcdBuf, byte(1), mark);
  memset(lcdBuf + mark, ' ', COLS - mark);
  lcd.print(lcdBuf);
}

void fnLoading() {
  if (counter >= MAX) {
    counter = 0;
    lcd.setCursor(percStartPos, 0);
    lcd.print("   ");
    bLoaded = true;
    state = 2;
    exit;
  }
  lcd.display();
  lcd.setCursor(textStartPos, 0);
  lcd.print("Activando..");
  counter = (counter + 1) % (MAX + 1);
  lcd.setCursor(percStartPos, 0);
  lcd.print(counter);
  lcd.setCursor(15, 0);
  lcd.write(3);
  lcd.setCursor(textStartPos, 1);
  printBar(counter);
  delay(100);
}

void fnWaiting() {
  counter = 0;
  lcd.clear();
  lcd.setCursor(textStartPos, 0);
  lcd.print("En espera..");
}

void setup()
{
  Serial.begin(9600); // for debug
  currentColor = -1;

  pinMode(BUTTONTIME, INPUT);
  pinMode(masterKeyPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LCD_BACKLIGHT_RED, OUTPUT);
  pinMode(LCD_BACKLIGHT_GREEN, OUTPUT);
  pinMode(LCD_BACKLIGHT_BLUE, OUTPUT);

  for (int i = 0; i < TOTAL_WIRES; i++) {
    pinMode(CUTTABLE_WIRES[i], INPUT_PULLUP);
    wireStates[i] = LOW; // wires are initially tied to GND
  }

  //setDefaultBG();

  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.clear();

  Serial.println("Ready");

  lcd.createChar(1, customChar1);
  lcd.createChar(2, customChar2);
  lcd.createChar(3, customChar3);

  state = 0;
  fnWaiting();
}

int detectWireStateChange() {
  for (int i = 0; i < TOTAL_WIRES; i++) {
    int newValue = digitalRead(CUTTABLE_WIRES[i]);
    if (newValue != wireStates[i]) {
      wireStates[i] = newValue;
      return i;
    }
  }
  return -1;
}

void displayCurrentState() {
  lcd.setCursor(0, 1);
  int missingWires = 0;
  for (int i = 0; i < TOTAL_WIRES; i++) {
    if (WIRES_TO_CUT[i]) {
      if (wireStates[i]) {
        // Wire was correctly cut
        lcd.print("*");
      } else {
        missingWires++;
      }
    }
  }

  // This is just to erase previously shown asterisks
  for (int i = 0; i < missingWires; i++) {
    lcd.print(" ");
  }
}

void displayTimer() {
  char s[13];
  formatTime(remainingTime, s);
  lcd.setCursor(0, 0);
  lcd.print(s);
}

bool isIncorrectWriteCut() {
  for (int i = 0; i < TOTAL_WIRES; i++) {
    if (wireStates[i] == 1 && WIRES_TO_CUT[i] == 0) {
      return true;
    }
  }
  return false;
}

bool areAllCorrectWiresCut() {
  for (int i = 0; i < TOTAL_WIRES; i++) {
    if (wireStates[i] == 0 && WIRES_TO_CUT[i] == 1) {
      return false;
    }
  }
  return true;
}

void handleWireStateChange(int wireWithNewState) {
  Serial.print("Wire ");
  Serial.print(wireWithNewState);
  if (wireStates[wireWithNewState]) {
    Serial.print(" was cut");
    if (WIRES_TO_CUT[wireWithNewState]) {
      Serial.println(" => correct");
      displayCurrentState();
      setGreenBG();
      delay(1000);
    } else {
      Serial.println(" => INCORRECT");
    }
  } else {
    Serial.println(" reconnected");
  }

  if (isIncorrectWriteCut()) {
    setRedBG();
  } else {
    //setDefaultBG();
  }
}

void updateRemainingTime() {
  if (remainingTime == 0) return;

  unsigned long now = millis();
  if (lastTimeUpdatedAt == now) return;
  unsigned long elapsedTimeSinceLastUpdate = now - lastTimeUpdatedAt;

  if (isIncorrectWriteCut()) {
    // Time goes down X times as fast if incorrect wire is cut
    elapsedTimeSinceLastUpdate *= time_multiplier;
  }

  if (elapsedTimeSinceLastUpdate >= remainingTime) {
    remainingTime = 0;
  } else {
    remainingTime -= elapsedTimeSinceLastUpdate;
  }

  lastTimeUpdatedAt = now;
}

void setLoadedCondition() {

  double tTimeHours = TimeHours;

  TOTAL_TIME = tTimeHours * 3600 * 1000L;
  remainingTime = TOTAL_TIME;
  lcd.clear();
  bFirstLoad = true;
  lastTimeUpdatedAt = millis();;
}
void fnLoaded() {
  if (!bFirstLoad) {
    setLoadedCondition();
  }

  updateRemainingTime();

  if (remainingTime == 0) {
    setRedBG();
    lcd.clear();
    lcd.print("*** BOOM ***");
    //tone(6, 261, 3000);
    while (true);
  }

  displayTimer();

  int wireWithNewState = detectWireStateChange();
  if (wireWithNewState >= 0) {
    handleWireStateChange(wireWithNewState);
  }

  displayCurrentState();

  int masterKeyValue = digitalRead(masterKeyPin);
  if ((areAllCorrectWiresCut() && !isIncorrectWriteCut()) || masterKeyValue == 1)  {
    // Win
    setGreenBG();
    Serial.println("Win conditions");
    Serial.println((String)"All Correct: " + areAllCorrectWiresCut());
    Serial.println((String)"Is incorrect: " + isIncorrectWriteCut());
    Serial.println((String)"Master Key: " + masterKeyValue);
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("DESACTIVADA!");
    while (true);
  }
}
void RGBblink() {
  //si armado y no cable erroneo
  cableError = isIncorrectWriteCut();
  if (bLoaded && !cableError) {
    //amarillo
    blinkRGBLed(230, 230, 0);
  }/* else if (bLoaded && cableError) {
    //rojo
    blinkRGBLed(255, 0, 0);
  }*/
}
void blinkRGBLed(int pRed, int pGreen, int pBlue) {
  digitalWrite(LCD_BACKLIGHT_RED, pRed);
  digitalWrite(LCD_BACKLIGHT_GREEN, pGreen);
  digitalWrite(LCD_BACKLIGHT_BLUE, pBlue);
  delay(500);
  digitalWrite(LCD_BACKLIGHT_RED, 0);
  digitalWrite(LCD_BACKLIGHT_GREEN, 0);
  digitalWrite(LCD_BACKLIGHT_BLUE, 0);
  delay(500);
}

void loop()
{
    RGBblink();
  int a = digitalRead(BUTTONTIME);
  if ((a == 1) & (!bLoaded)) {
    state = 1;
    fnLoading();
  }
  else if (bLoaded) {
    fnLoaded();
  }
  else {
    if (state != 0) {
      fnWaiting();
      state = 0;
    }
  }
}
