#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Пины основных каналов ввода и индикации
const int BUTTON_PIN = 2;   // Канал 1: Механическая кнопка (Pin 2 -> GND)
const int BUZZER_PIN = 8;   // Активный зуммер (Pin 8 -> GND)
const int SOUND_PIN  = A1;  // Канал 2: Датчик звука (AO -> A1)
const int PHOTO_PIN  = A0;  // Канал 3: Фоторезистор (делитель -> A0)

// Пины модуля энкодера KY-040
const int ENC_CLK_PIN = 3;  // Выход А (CLK) -> Pin 3 (PD3 / PCINT19)
const int ENC_DT_PIN  = 4;  // Выход B (DT)  -> Pin 4 (PD4 / PCINT20)
const int ENC_SW_PIN  = 5;  // Кнопка (SW)   -> Pin 5

const int SOUND_THRESHOLD = 120;
const int SOUND_HOLD_TIME = 100;
const int LIGHT_THRESHOLD = 600;

unsigned long pressStartTime = 0;
unsigned long releaseTime = 0;
unsigned long lastSoundTime = 0;
bool isPressed = false;

String currentMorse = "";
String decodedWord = "";

enum Mode { WAIT, MORSE, CAESAR, CUSTOM };
Mode currentMode = WAIT;
Mode lastActiveMode = MORSE; // Последний выбранный рабочий режим

// Переменные для обработки энкодера KY-040 (автомат состояний кода Грея)
const int8_t ENC_STATES[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};
volatile uint8_t encAB = 0x03;
volatile int8_t encSubStep = 0;
volatile int8_t encDelta = 0;

// Переменные антидребезга и удержания кнопки энкодера (SW)
bool encBtnPressed = false;
bool encBtnLongHandled = false;
unsigned long encBtnPressTime = 0;
unsigned long lastEncRotateTime = 0;
const unsigned long ENC_DEBOUNCE_MS = 40;
const unsigned long ENC_LONG_PRESS_MS = 700;

struct MorseChar {
    char letter;
    const char* code;
};

// Полная таблица Морзе со служебными командами
MorseChar morseTable[] = {
    {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
    {'E', "."},    {'F', "..-."}, {'G', "--."},  {'H', "...."},
    {'I', ".."},   {'J', ".---"}, {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},   {'N', "-."},   {'O', "---"},  {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {' ', ".-.-"},    // Команда: Пробел
    {'\b', "......"}, // Команда: Удаление
    {'*', "----"}     // Команда: Смена режима (выход в WAIT)
};

// Прототипы функций
bool checkSignal();
char applyCaesar(char c);
char applyCustom(char c);
char decodeMorse(String code);
void updateDisplay();
void handleEncoder();
void shortBeep(unsigned int durationMs, unsigned int freqHz = 1500);

// Прерывание по изменению состояния пинов D3 (CLK) и D4 (DT)
ISR(PCINT2_vect) {
  uint8_t clk = (PIND >> PD3) & 0x01;
  uint8_t dt  = (PIND >> PD4) & 0x01;
  uint8_t cur = (clk << 1) | dt;
  encAB = ((encAB << 2) | cur) & 0x0F;
  encSubStep += ENC_STATES[encAB];

  if (cur == 0x03) { // Положение фиксации (щелчок)
    if (encSubStep >= 2) {
      encDelta++;
    } else if (encSubStep <= -2) {
      encDelta--;
    }
    encSubStep = 0;
  }
}

void shortBeep(unsigned int durationMs, unsigned int freqHz) {
  tone(BUZZER_PIN, freqHz);
  delay(durationMs);
  noTone(BUZZER_PIN);
}

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Инициализация пинов энкодера KY-040
  pinMode(ENC_CLK_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);

  // Начальное состояние линий CLK и DT и разрешение прерываний PCINT19 (D3) и PCINT20 (D4)
  encAB = ((digitalRead(ENC_CLK_PIN) << 1) | digitalRead(ENC_DT_PIN)) & 0x03;
  noInterrupts();
  PCICR  |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT19) | (1 << PCINT20);
  interrupts();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Crypto Decoder");
  lcd.setCursor(0, 1);
  lcd.print("Prefix / Encoder");
  delay(2000);
  lcd.clear();
  updateDisplay();
}

bool checkSignal() {
  if (digitalRead(BUTTON_PIN) == LOW) return true;
  if (analogRead(PHOTO_PIN) > LIGHT_THRESHOLD) return true;
  if (analogRead(SOUND_PIN) > SOUND_THRESHOLD) {
    lastSoundTime = millis();
    return true;
  }
  if (millis() - lastSoundTime < SOUND_HOLD_TIME) return true;
  return false;
}

char applyCaesar(char c) {
  if (c >= 'A' && c <= 'Z') return ((c - 'A' + 3) % 26) + 'A';
  return c;
}

char applyCustom(char c) {
  if (c >= 'A' && c <= 'Z') return 'Z' - (c - 'A');
  return c;
}

// Обработка вращения и нажатия кнопки энкодера KY-040
void handleEncoder() {
  int8_t step = 0;
  noInterrupts();
  if (encDelta != 0) {
    step = encDelta;
    encDelta = 0;
  }
  interrupts();

  // 1. Вращение энкодера: переключение режимов (MORSE <-> CAESAR <-> CUSTOM)
  if (step != 0) {
    const Mode workingModes[3] = { MORSE, CAESAR, CUSTOM };
    int idx = 0;
    Mode refMode = (currentMode == WAIT) ? lastActiveMode : currentMode;
    for (int i = 0; i < 3; i++) {
      if (workingModes[i] == refMode) {
        idx = i;
        break;
      }
    }

    if (currentMode == WAIT) {
      // Из режима WAIT первый щелчок по часовой включает MORSE (или последний активный), против — CUSTOM
      if (step > 0) idx = 0;
      else idx = 2;
    } else {
      if (step > 0) idx = (idx + 1) % 3;
      else idx = (idx + 2) % 3;
    }

    currentMode = workingModes[idx];
    lastActiveMode = currentMode;
    currentMorse = ""; // Очищаем незавершенный ввод Морзе при смене режима
    lastEncRotateTime = millis();
    shortBeep(30, 1800);
    updateDisplay();
  }

  // 2. Нажатие кнопки энкодера (SW):
  // - Короткое нажатие: выбор режима (из WAIT) или сброс в режим WAIT (если режим уже активен)
  // - Долгое нажатие (>700 мс): полный сброс введенного слова и переход в WAIT
  bool swLow = (digitalRead(ENC_SW_PIN) == LOW);
  unsigned long now = millis();

  if (swLow && !encBtnPressed) {
    encBtnPressed = true;
    encBtnLongHandled = false;
    encBtnPressTime = now;
  } else if (swLow && encBtnPressed && !encBtnLongHandled) {
    if (now - encBtnPressTime >= ENC_LONG_PRESS_MS) {
      encBtnLongHandled = true;
      currentMode = WAIT;
      currentMorse = "";
      decodedWord = "";
      Serial.println("{\"mode\":\"COMMAND\",\"char\":\"CLEAR\"}");
      shortBeep(60, 1200);
      delay(50);
      shortBeep(80, 800);
      updateDisplay();
    }
  } else if (!swLow && encBtnPressed) {
    unsigned long pressDur = now - encBtnPressTime;
    encBtnPressed = false;

    if (!encBtnLongHandled && pressDur >= ENC_DEBOUNCE_MS) {
      if (currentMode == WAIT) {
        // Вход в выбранный рабочий режим
        currentMode = lastActiveMode;
        currentMorse = "";
        shortBeep(45, 1500);
      } else if (now - lastEncRotateTime < 2000) {
        // Если нажали кнопку сразу после вращения — подтверждаем выбранный режим
        lastEncRotateTime = 0;
        currentMorse = "";
        shortBeep(45, 1500);
      } else {
        // Сброс активного режима обратно в WAIT
        currentMode = WAIT;
        currentMorse = "";
        shortBeep(45, 900);
      }
      updateDisplay();
    }
  }
}

void loop() {
  // 0. ОБРАБОТКА ЭНКОДЕРА KY-040
  handleEncoder();

  // 1. ПРИЕМ ДАННЫХ ОТ СЕРВЕРА (из POST-запросов)
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();

    if (incomingChar != '\n' && incomingChar != '\r') {
      if (incomingChar == '\b') {
        if (decodedWord.length() > 0) decodedWord.remove(decodedWord.length() - 1);
      } else {
        decodedWord += incomingChar;
      }
      updateDisplay();
    }
  }

  // 2. ОБРАБОТКА ДАТЧИКОВ (Кнопка, Фоторезистор, Датчик звука)
  bool signalActive = checkSignal();
  unsigned long currentTime = millis();

  if (signalActive && !isPressed) {
    isPressed = true;
    pressStartTime = currentTime;
    tone(BUZZER_PIN, 1000); // Генерация частоты 1000 Гц (работает и для пассивного, и для активного зуммера)
    delay(30);
  }

  if (!signalActive && isPressed) {
    isPressed = false;
    unsigned long pressDuration = currentTime - pressStartTime;
    noTone(BUZZER_PIN);
    releaseTime = currentTime;

    if (pressDuration < 300) currentMorse += ".";
    else currentMorse += "-";

    updateDisplay();
  }

  if (!isPressed && currentMorse.length() > 0 && (currentTime - releaseTime > 700)) {
    if (currentMode == WAIT) {
      if (currentMorse == "...") {
        currentMode = MORSE;
        lastActiveMode = MORSE;
      } else if (currentMorse == "---") {
        currentMode = CAESAR;
        lastActiveMode = CAESAR;
      } else if (currentMorse == ".-.") {
        currentMode = CUSTOM;
        lastActiveMode = CUSTOM;
      }

      currentMorse = "";
      updateDisplay();
      return;
    }

    char decodedLetter = decodeMorse(currentMorse);

    if (decodedLetter == '*') {
      currentMode = WAIT;
    } else if (decodedLetter == ' ') {
      decodedWord += " ";
      Serial.println("{\"mode\":\"COMMAND\",\"char\":\"SPACE\"}");
    } else if (decodedLetter == '\b') {
      if (decodedWord.length() > 0) decodedWord.remove(decodedWord.length() - 1);
      Serial.println("{\"mode\":\"COMMAND\",\"char\":\"BACKSPACE\"}");
    } else if (decodedLetter != '?') {
      char finalChar = decodedLetter;
      String modeStr = "MORSE";

      if (currentMode == CAESAR) {
        finalChar = applyCaesar(finalChar);
        modeStr = "CAESAR";
      } else if (currentMode == CUSTOM) {
        finalChar = applyCustom(finalChar);
        modeStr = "CUSTOM";
      }

      decodedWord += finalChar;

      String jsonPayload = "{\"mode\":\"" + modeStr + "\",\"char\":\"" + String(finalChar) + "\"}";
      Serial.println(jsonPayload);
    }

    currentMorse = "";
    updateDisplay();
  }
}

char decodeMorse(String code) {
  int tableSize = sizeof(morseTable) / sizeof(morseTable[0]);
  for (int i = 0; i < tableSize; i++) {
    if (String(morseTable[i].code) == code) return morseTable[i].letter;
  }
  return '?';
}

void updateDisplay() {
  lcd.setCursor(0, 0);

  if (currentMode == WAIT) lcd.print("MODE: WAIT      ");
  else if (currentMode == MORSE) lcd.print("MORSE:          ");
  else if (currentMode == CAESAR) lcd.print("CAESAR:         ");
  else if (currentMode == CUSTOM) lcd.print("CUSTOM:         ");

  lcd.setCursor(8, 0);
  lcd.print(currentMorse);
  lcd.print("        ");

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  if (decodedWord.length() > 16) {
    lcd.print(decodedWord.substring(decodedWord.length() - 16));
  } else {
    lcd.print(decodedWord);
  }
}
