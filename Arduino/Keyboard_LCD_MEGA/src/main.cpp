#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>

// LCD I2C (адреса 0x27, 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Клавіатура 4x4
const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {27, 29, 31, 33}; // підключення клавіш
byte colPins[COLS] = {16, 17, 23, 25};

Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

bool placeTaken[4] = {false, false, false, false};
int placePins[4] = {0, 0, 0, 0};
bool waitingForPin = false;
int currentPlace = -1;
String enteredPin = "";

int wrongAttempts = 0;
bool isLocked = false;

#define EEPROM_MAGIC_ADDR 100
#define EEPROM_MAGIC_VALUE 42
#define EEPROM_LOCKED_ADDR 120  // адреса для збереження блокування

void saveToEEPROM() {
  for (int i = 0; i < 4; i++) EEPROM.write(i, placeTaken[i]);
  for (int i = 0; i < 4; i++) EEPROM.put(10 + i * 2, placePins[i]);
}

void loadFromEEPROM() {
  for (int i = 0; i < 4; i++) placeTaken[i] = EEPROM.read(i);
  for (int i = 0; i < 4; i++) EEPROM.get(10 + i * 2, placePins[i]);
}

void initEEPROM() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VALUE) {
    for (int i = 0; i < 4; i++) {
      placeTaken[i] = false;
      placePins[i] = 0;
      EEPROM.write(i, 0);
      EEPROM.put(10 + i * 2, 0);
    }
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.write(EEPROM_LOCKED_ADDR, false); // зберігаємо isLocked = false при ініціалізації
  }
}

void setLockedState(bool locked) {
  isLocked = locked;
  EEPROM.write(EEPROM_LOCKED_ADDR, isLocked); // зберегти у EEPROM
}

bool isPinUsed(int pin);
void showFreePlaces();

void setup() {
  randomSeed(analogRead(A0));
  lcd.init();
  lcd.backlight();
  initEEPROM();
  loadFromEEPROM();
  isLocked = EEPROM.read(EEPROM_LOCKED_ADDR); // зчитати isLocked з EEPROM

  if (isLocked) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PIN admina:");
    lcd.setCursor(0, 1);
    lcd.print("____");
  } else {
    showFreePlaces();
  }
}

void loop() {
  char key = keypad.getKey();

  // Якщо панель заблокована — тільки PIN admina
  if (isLocked) {
    if (key && enteredPin.length() < 4) {
      enteredPin += key;
      lcd.setCursor(0, 1);
      lcd.print(enteredPin);
      for (int i = enteredPin.length(); i < 4; i++) lcd.print("_");
    }

    if (key == '#') {
      if (enteredPin == "CBAD") {
        setLockedState(false);
        wrongAttempts = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Rozblokovane!");
        delay(2000);
        enteredPin = "";
        showFreePlaces();
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Panel");
        lcd.setCursor(0, 1);
        lcd.print("zablokovany");
        delay(2000);
        lcd.clear();
        enteredPin = "";
        lcd.setCursor(0, 0);
        lcd.print("PIN admina:");
        lcd.setCursor(0, 1);
        lcd.print("____");
      }
    }

    if (key == '*') {
      enteredPin = "";
      lcd.setCursor(0, 1);
      lcd.print("____");
    }

    return;
  }

  if (!waitingForPin) {
    if (key == '*') {
      waitingForPin = true;
      currentPlace = -1;
      enteredPin = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Zadajte PIN:");
      lcd.setCursor(0, 1);
      lcd.print("____");
    } else if (key && (key >= '1' && key <= '4')) {
      int place = key - '1';
      if (!placeTaken[place]) {
        int pin;
        do {
          pin = random(1000, 9999);
        } while (isPinUsed(pin));
        placePins[place] = pin;
        placeTaken[place] = true;
        saveToEEPROM();
        currentPlace = place;
        waitingForPin = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(String(place + 1) + ": " + String(pin));
        delay(2000);
        showFreePlaces();
      }
    }
  } else {
    if (key && enteredPin.length() < 4) {
      enteredPin += key;
      lcd.setCursor(0, 1);
      lcd.print(enteredPin);
      for (int i = enteredPin.length(); i < 4; i++) lcd.print("_");
    }

    if (key == '#') {
      if (enteredPin == "CBAD") {
        lcd.clear();
        lcd.setCursor(0, 0);
        for (int i = 0; i < 2; i++) {
          lcd.print(String(i + 1) + ":");
          if (placeTaken[i]) lcd.print(placePins[i]);
          else lcd.print("-");
          lcd.print(" ");
        }
        lcd.setCursor(0, 1);
        for (int i = 2; i < 4; i++) {
          lcd.print(String(i + 1) + ":");
          if (placeTaken[i]) lcd.print(placePins[i]);
          else lcd.print("-");
          lcd.print(" ");
        }
        delay(4000);
        enteredPin = "";
        waitingForPin = false;
        return;
      }

      bool found = false;
      for (int i = 0; i < 4; i++) {
        if (placeTaken[i] && placePins[i] == enteredPin.toInt()) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Vydava sa auto " + String(i + 1));
          delay(2000);
          placeTaken[i] = false;
          placePins[i] = 0;
          saveToEEPROM();
          found = true;
          wrongAttempts = 0;
          break;
        }
      }

      if (!found && enteredPin.length() == 4) {
        wrongAttempts++;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Nespravny PIN!");
        delay(2000);
        if (wrongAttempts >= 3) {
          setLockedState(true);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Zablokovane!");
          delay(2000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PIN admina:");
          lcd.setCursor(0, 1);
          lcd.print("____");
        }
      }

      enteredPin = "";
      waitingForPin = false;
      currentPlace = -1;
      if (!isLocked) showFreePlaces();
    }

    if (key == '*') {
      enteredPin = "";
      lcd.setCursor(0, 1);
      lcd.print("____");
    }
  }
}

bool isPinUsed(int pin) {
  for (int i = 0; i < 4; i++) {
    if (placePins[i] == pin) return true;
  }
  return false;
}

void showFreePlaces() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Volne miesta:");
  lcd.setCursor(0, 1);
  for (int i = 0; i < 4; i++) {
    if (!placeTaken[i]) lcd.print(String(i + 1));
    else lcd.print("-");
    if (i < 3) lcd.print("/");
  }
}
