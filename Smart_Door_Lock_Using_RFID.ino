/*
  PROJECT: Smart RFID Door Lock
  BOARD: NodeMCU ESP8266
  RFID: RC522
  MOTOR: SG90 Servo
  PLATFORM: KiwisIoT
*/

#include <ESP8266WiFi.h>
#include <KiwisIoT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// =================================
// WIFI CONFIGURATION
// =================================

const char* ssid = "YOUR_WIFI_NAME";
const char* pass = "YOUR_WIFI_PASSWORD";

// KiwisIoT Topic ID
const char* topic = "YOUR_DASHBOARD_TOPIC_ID";

KiwisIoT kiwisiot(ssid, pass, topic);

// =================================
// RFID PIN CONFIGURATION
// =================================

#define SS_PIN  D2
#define RST_PIN D1

MFRC522 rfid(SS_PIN, RST_PIN);

// =================================
// SERVO CONFIGURATION
// =================================

#define SERVO_PIN D0

Servo doorServo;

// Adjust these angles according to
// your physical lock mechanism.

const int LOCK_ANGLE = 0;
const int UNLOCK_ANGLE = 90;

const unsigned long UNLOCK_TIME = 5000;

bool doorUnlocked = false;

unsigned long unlockStartTime = 0;
unsigned long lastCardTime = 0;

const unsigned long CARD_COOLDOWN = 2000;

// =================================
// AUTHORIZED RFID CARD UID
// =================================

// Replace this with your actual card UID.
// Scan the card and check Serial Monitor.

String authorizedUID = "A1B2C3D4";

// =================================
// FUNCTION: READ RFID UID
// =================================

String readCardUID() {

  String cardUID = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      cardUID += "0";
    }

    cardUID += String(
      rfid.uid.uidByte[i], HEX
    );
  }

  cardUID.toUpperCase();

  return cardUID;
}

// =================================
// FUNCTION: SEND CLOUD STATUS
// =================================

void sendStatus(String status) {

  kiwisiot.send("0", status);

  Serial.print("KiwisIoT Status: ");
  Serial.println(status);
}

// =================================
// FUNCTION: UNLOCK DOOR
// =================================

void unlockDoor() {

  doorServo.write(UNLOCK_ANGLE);

  doorUnlocked = true;

  unlockStartTime = millis();

  Serial.println("ACCESS GRANTED");
  Serial.println("Door Unlocked");

  sendStatus("UNLOCKED");
}

// =================================
// FUNCTION: LOCK DOOR
// =================================

void lockDoor() {

  doorServo.write(LOCK_ANGLE);

  doorUnlocked = false;

  Serial.println("Door Locked");

  sendStatus("LOCKED");
}

// =================================
// SETUP
// =================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("SMART RFID DOOR LOCK");
  Serial.println("====================");

  // Initialize servo
  doorServo.attach(SERVO_PIN);

  doorServo.write(LOCK_ANGLE);

  // Initialize SPI and RFID
  SPI.begin();

  rfid.PCD_Init();

  Serial.println("RFID Reader Initialized");

  // Initialize KiwisIoT
  Serial.println("Connecting to KiwisIoT...");

  kiwisiot.begin();

  Serial.println("KiwisIoT Initialized");

  sendStatus("LOCKED");

  Serial.println("System Ready");
  Serial.println("Scan Your RFID Card");
}

// =================================
// MAIN LOOP
// =================================

void loop() {

  // Maintain cloud connection
  kiwisiot.run();

  // Automatically lock after 5 seconds
  if (doorUnlocked &&
      millis() - unlockStartTime >= UNLOCK_TIME) {

    lockDoor();
  }

  // Check for a new RFID card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Prevent repeated reads
  if (millis() - lastCardTime < CARD_COOLDOWN) {

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return;
  }

  lastCardTime = millis();

  // Read card UID
  String scannedUID = readCardUID();

  Serial.println();
  Serial.print("Scanned Card UID: ");

  Serial.println(scannedUID);

  // Check authorization
  if (scannedUID == authorizedUID) {

    Serial.println("Authorized Card");

    unlockDoor();

    sendStatus("ACCESS GRANTED");

  } else {

    Serial.println("Unauthorized Card");

    Serial.println("Access Denied");

    sendStatus("ACCESS DENIED");
  }

  // Stop RFID communication
  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();
}