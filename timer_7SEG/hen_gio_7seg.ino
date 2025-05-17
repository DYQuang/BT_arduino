// ---------------- Chân kết nối ----------------
#define DATA_PIN  5
#define LATCH_PIN 4
#define CLOCK_PIN 6

const byte segmentPins[7] = {13, 12, 11, 10, 9, 8, 7};

// Chân nút
#define BTN_ONOFF 2
#define BTN_RESET 3
#define BTN_H_INC A0
#define BTN_H_DEC A1
#define BTN_M_INC A2
#define BTN_M_DEC A3

// Chân LED cảnh báo (đỏ)
#define ALERT_LED_PIN A5

const byte digitCode[10][7] = {
  {0,0,0,0,0,0,1}, 
  {1,0,0,1,1,1,1}, 
  {0,0,1,0,0,1,0}, 
  {0,0,0,0,1,1,0},
  {1,0,0,1,1,0,0}, 
  {0,1,0,0,1,0,0}, 
  {0,1,0,0,0,0,0}, 
  {0,0,0,1,1,1,1},
  {0,0,0,0,0,0,0},
  {0,0,0,0,1,0,0}
};

byte timeDigits[4] = {0, 0, 0, 0};
int hour = 0, minute = 0;
int second = 0;

bool blinkState = false;  // Dùng để nháy dấu :
bool clockRunning = false;

unsigned long lastBlinkTime = 0;
unsigned long lastSecondUpdate = 0;

const int alertHour = 1;
const int alertMinute = 0;

bool alertTriggered = false;
unsigned long alertBlinkStart = 0;
int alertBlinkCount = 0;
bool alertLedState = false;

// ---------------- Hiển thị ----------------
void displayOneDigit(byte digit, byte position) {
  // Tạo byte điều khiển cho 74HC595: bit 0-3 chọn LED 7 đoạn, bit 4 là LED chỉ thị giây
  byte controlByte = (1 << position);

  // Nếu đang ở vị trí 0 thì chèn LED giây
  if (blinkState) controlByte |= (1 << 4);  // Q4 bật nếu blinkState = true

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, controlByte);
  digitalWrite(LATCH_PIN, HIGH);

  // Hiển thị segment tương ứng
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], digitCode[digit][i]);
  }

  delayMicroseconds(800);
}

void updateTimeDigits() {
  timeDigits[0] = hour / 10;
  timeDigits[1] = hour % 10;
  timeDigits[2] = minute / 10;
  timeDigits[3] = minute % 10;
}

void displayTime() {
  updateTimeDigits();
  for (int i = 0; i < 4; i++) {
    displayOneDigit(timeDigits[i], i);
  }
}

// ---------------- Setup ----------------
void setup() {
  for (int i = 0; i < 7; i++) pinMode(segmentPins[i], OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  pinMode(BTN_ONOFF, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_H_INC, INPUT_PULLUP);
  pinMode(BTN_H_DEC, INPUT_PULLUP);
  pinMode(BTN_M_INC, INPUT_PULLUP);
  pinMode(BTN_M_DEC, INPUT_PULLUP);

  pinMode(ALERT_LED_PIN, OUTPUT);
  digitalWrite(ALERT_LED_PIN, LOW);
}

// ---------------- Hàm phụ ----------------
bool isPressed(int pin) {
  return digitalRead(pin) == LOW;
}

void resetClock() {
  hour = 0;
  minute = 0;
  second = 0;
  clockRunning = true;
  alertTriggered = false;
  alertBlinkCount = 0;
  digitalWrite(ALERT_LED_PIN, LOW);
}

void updateClock() {
  unsigned long now = millis();

  if (now - lastBlinkTime >= 1000) {
    lastBlinkTime = now;

    if (clockRunning) {
      blinkState = !blinkState;

      second++;
      if (second >= 60) {
        second = 0;
        minute++;
        if (minute >= 60) {
          minute = 0;
          hour = (hour + 1) % 24;
        }
      }
    }
  }
}


void handleButtons() {
  static bool prevONOFF = false;
  static bool prevRESET = false;

  if (isPressed(BTN_ONOFF) && !prevONOFF) {
    if (!clockRunning) {
      // Bắt đầu chạy
      clockRunning = true;
      blinkState = true; // Bắt đầu nhấp nháy đèn giây
    } else {
      // Dừng và reset
      clockRunning = false;
      blinkState = false; // Tắt nhấp nháy đèn giây
      hour = 0;
      minute = 0;
      second = 0;
    }
    delay(200);  // chống dội phím
  }
  prevONOFF = isPressed(BTN_ONOFF);

  if (isPressed(BTN_RESET) && !prevRESET) {
    resetClock();
    delay(200);
  }
  prevRESET = isPressed(BTN_RESET);

  if (clockRunning) {
    if (isPressed(BTN_H_INC)) { hour = (hour + 1) % 24; delay(200); }
    if (isPressed(BTN_H_DEC)) { hour = (hour + 23) % 24; delay(200); }
    if (isPressed(BTN_M_INC)) { minute = (minute + 1) % 60; delay(200); }
    if (isPressed(BTN_M_DEC)) { minute = (minute + 59) % 60; delay(200); }
  }
}


void alertProcess() {
  if (!alertTriggered) {
    if (hour == alertHour && minute == alertMinute) {
      alertTriggered = true;
      alertBlinkStart = millis();
      alertBlinkCount = 0;
      alertLedState = false;
      digitalWrite(ALERT_LED_PIN, LOW);
    }
  } else {
    unsigned long now = millis();
    if (now - alertBlinkStart >= 200) {  // Đổi từ 500ms -> 200ms
      alertBlinkStart = now;

      // Đổi trạng thái đèn
      alertLedState = !alertLedState;
      digitalWrite(ALERT_LED_PIN, alertLedState ? HIGH : LOW);

      // Chỉ đếm khi sáng (tránh đếm cả lần tắt)
      if (alertLedState) {
        alertBlinkCount++;
        if (alertBlinkCount >= 5) {
          alertTriggered = false;
          digitalWrite(ALERT_LED_PIN, LOW); // Tắt đèn sau khi xong
        }
      }
    }
  }
}


// ---------------- Loop ----------------
void loop() {
  handleButtons();
  updateClock();
  alertProcess();

  displayTime();
}
