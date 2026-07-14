/************************************************************
   SMART DOOR SECURITY SYSTEM USING ESP32 + BLYNK
 ************************************************************/
// ---------------- BLYNK ----------------
#define BLYNK_TEMPLATE_ID "Your_Template_ID"
#define BLYNK_TEMPLATE_NAME "ESP32 Smart door security system"
#define BLYNK_AUTH_TOKEN "Your_Auth_Token"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ---------------- WIFI ----------------
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// ---------------- LIBRARIES ----------------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- RFID ----------------
#define SS_PIN   5
#define RST_PIN  4

MFRC522 rfid(SS_PIN, RST_PIN);

// ---------------- SERVO ----------------
Servo doorServo;

// ---------------- PIN DEFINITIONS ----------------
#define SERVO_PIN   12
#define PIR_PIN     13
#define GREEN_LED   14
#define RED_LED     27 //27
#define BUZZER      26
#define DOORBELL_PIN   18      

// ---------------- PASSWORD ----------------
String correctPassword = "2580";
String enteredPassword = "";

int wrongAttempts = 0;
// ---------- Visitor Detection ----------
bool visitorDetected = false;
bool userInteracted = false;

unsigned long visitorStartTime = 0;
const unsigned long WAIT_TIME = 60000;   // 60 seconds

// ---------------- RFID UID ----------------
// Replace with your RFID card UID
String authorizedUID = "55 66 77 88";

// ---------------- KEYPAD ----------------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {32, 33, 25, 15};
byte colPins[COLS] = {2, 16, 17, 0};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ======================================================
// BLYNK BUTTON
// ======================================================

BLYNK_WRITE(V0) {

  int buttonState = param.asInt();

  if (buttonState == 1) {

    openDoor();
  }
}
void checkDoorbell();

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  // ---------- BLYNK ----------
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // ---------- LCD ----------
  lcd.init();
  lcd.backlight();

  // ---------- PIN MODES ----------
  pinMode(PIR_PIN, INPUT);
  pinMode(DOORBELL_PIN, INPUT_PULLUP);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(RED_LED, LOW);   
  digitalWrite(GREEN_LED, LOW);

  // ---------- SERVO ----------
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);

  // ---------- RFID ----------
  SPI.begin();
  rfid.PCD_Init();

  // ---------- WELCOME ----------
  visitorDetected = false;
  userInteracted = false;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SMART DOOR");
  
  lcd.setCursor(0,1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("System Ready");
  delay(1500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Waiting...");

  Serial.println("==================================");
  Serial.println("Smart Door Security System Ready");
  Serial.println("Waiting for Visitor...");
  Serial.println("==================================");
}

// ======================================================
// LOOP
// ======================================================

void loop() {

  Blynk.run();

  // ----------------------------
  // Detect Visitor Using PIR
  // ----------------------------
  if (!visitorDetected && digitalRead(PIR_PIN) == HIGH) {

    visitorDetected = true;
    userInteracted = false;
    visitorStartTime = millis();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Visitor Found");
    lcd.setCursor(0, 1);
    lcd.print("Waiting...");

    Serial.println("Visitor Detected");
  }

  // ----------------------------
  // Wait for User Interaction
  // ----------------------------
  if (visitorDetected) {

    checkDoorbell();
    checkKeypad();
    checkRFID();

    // 30 seconds expired?
    if ((millis() - visitorStartTime >= WAIT_TIME) &&
        !userInteracted) {

      Blynk.logEvent(
        "visitor_alert",
        "Someone is waiting at your door."
      );

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Owner Notified");

      Serial.println("Notification Sent");

      visitorDetected = false;

      delay(2000);

      lcd.clear();
      lcd.print("Waiting...");
    }

    // User interacted
    if (userInteracted) {

      visitorDetected = false;

      Serial.println("User Interaction Detected");
    }
  }
}

// ======================================================
// DOORBELL FUNCTION
// ======================================================

void checkDoorbell() {

  if (digitalRead(DOORBELL_PIN) == LOW) {

    userInteracted = true;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Doorbell");
    lcd.setCursor(0,1);
    lcd.print("Ringing...");

    Serial.println("Doorbell Pressed");

    delay(1000);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Choose Access");
    lcd.setCursor(0,1);
    lcd.print("RFID/Password");
  }
}

// ======================================================
// KEYPAD FUNCTION
// ======================================================

void checkKeypad() {

  char key = keypad.getKey();

  if (key) {
    userInteracted = true;

    Serial.println(key);

    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Password:");

    lcd.setCursor(0,1);
    lcd.print(enteredPassword);
    lcd.print("*");

    delay(300);

    // ---------- ENTER ----------
    if (key == '#') {

      if (enteredPassword == correctPassword) {
        userInteracted = true;
        accessGranted();
      }
      else {
        userInteracted = true;
        accessDenied();
      }

      enteredPassword = "";
    }

    // ---------- CLEAR ----------
    else if (key == '*') {

      enteredPassword = "";
    }

    else {

      enteredPassword += key;
    }
  }
}

// ======================================================
// RFID FUNCTION
// ======================================================

void checkRFID() {

  if (rfid.PICC_IsNewCardPresent() &&
      rfid.PICC_ReadCardSerial()) {
        userInteracted = true;

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("RFID Scanned");

      Serial.println("RFID Card Detected");

      String cardUID = "";

    for (byte i = 0; i < rfid.uid.size; i++) {

      cardUID += String(rfid.uid.uidByte[i], HEX);

      if (i != rfid.uid.size - 1) {

        cardUID += " ";
      }
    }

    cardUID.toUpperCase();

    Serial.println(cardUID);

    if (cardUID == authorizedUID) {
      Serial.println("Authorized RFID");
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Authorized");
      lcd.setCursor(0,1);
      lcd.print("Access Granted");
      
      accessGranted();
    }
    else {
      
      Serial.println("Unauthorized RFID");
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Invalid RFID");
      
      Blynk.logEvent(
        "intruder_alert",
        "Unauthorized RFID card detected!"
      );
      accessDenied();
    }

    rfid.PICC_HaltA();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Waiting...");
  }
}

// ======================================================
// ACCESS GRANTED
// ======================================================

void accessGranted() {

  wrongAttempts = 0;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ACCESS GRANTED");

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  tone(BUZZER, 1000, 200);

  delay(500);

  openDoor();

  digitalWrite(GREEN_LED, LOW);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Waiting...");
}

// ======================================================
// ACCESS DENIED
// ======================================================

void accessDenied() {

  wrongAttempts++;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ACCESS DENIED");

  digitalWrite(RED_LED, HIGH);

  tone(BUZZER, 500);

  delay(1500);

  noTone(BUZZER);

  digitalWrite(RED_LED, LOW);

  // Send notification only for repeated failures
  if (wrongAttempts >= 3) {

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("INTRUDER ALERT");

    Blynk.logEvent(
      "intruder_alert",
      "Three failed authentication attempts!"
    );

    for(int i=0;i<5;i++){

      digitalWrite(RED_LED,HIGH);
      tone(BUZZER,1000);

      delay(250);

      digitalWrite(RED_LED,LOW);
      noTone(BUZZER);

      delay(250);
    }

    wrongAttempts = 0;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Waiting...");
}

// ======================================================
// OPEN DOOR
// ======================================================

void openDoor() {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Door Opening");

  doorServo.write(90);

  delay(5000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Door Closing");

  doorServo.write(0);

  delay(1000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Door Locked");

  delay(1500);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Waiting...");
}
