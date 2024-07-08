/*
PIN RC522 Using ESP32
SDA   = D21
SCL   = D18
MOSI  = D23
MISO  = D19
RQ    = NC
GND   = GND
RST   = D22
VDD   = 3.3V

PIN RC522 Using ESP8266
SDA   = D4
SCL   = D5
MOSI  = D7
MISO  = D6
RQ    = NC
GND   = GND
RST   = D3
VDD   = 3.3V

PIN Fingerprint ESP32
VCC   = 3.3V
GND   = GND
RXD   = D17
TXD   = D16

BUTTON PIN for Index ID = D5
BUTTON PIN for Mode PIN = D15
BUTTON PIN for Solenoid = D25

*/

#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL67v-qnY4E"
#define BLYNK_TEMPLATE_NAME "Secure System"
#define BLYNK_AUTH_TOKEN "iotbHB_X6ucSADNSN-rhSbKq2nVtuvM5"

#include <SPI.h>
#include <WiFi.h>
#include <MFRC522.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Fingerprint.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(2, 3);

#else
#define mySerial Serial2
#endif

#define SS_PIN (21)
#define RST_PIN (22)

#define SOLENOID_PIN (25)
#define PIN_ID (5)    //BTN KUNING
#define PIN_MOD (15)  //BTN HITAM

#define PENGAMBILAN (0)
#define PENGEMBALIAN (1)
#define REGIST_FINGER (2)
#define DELETE_FINGER (3)

#define BUTTON_DEATTACH (1)

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
MFRC522 mfrc522(SS_PIN, RST_PIN);

char auth[] = BLYNK_AUTH_TOKEN;

//Don't forget to changed ssid and pass with your Wifi or hotspot name and password.
char ssid[] = "name your wifi/hotspot";
char pass[] = "Password your wifi/hotspot";

BlynkTimer timer;

String tag;
String word_send;

uint8_t mode = 0, ID = 0, enter_mode = 0;
char buffV1[50];

void setup() {
  Serial.begin(9600);
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522

  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  } else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains ");
    Serial.print(finger.templateCount);
    Serial.println(" templates");
  }
  pinMode(PIN_ID, INPUT_PULLUP);
  pinMode(PIN_MOD, INPUT_PULLUP);
  pinMode(SOLENOID_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(SOLENOID_PIN, HIGH);

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(500L, myDasboard);
  word_send = "Please tap your RFID Card or Finger";
}

void loop() {
  Blynk.run();
  timer.run();

  finger.getTemplateCount();
  if (digitalRead(PIN_MOD) != BUTTON_DEATTACH) {
    mode++;
    Serial.println(mode);
    delay(100);
  }

  if (digitalRead(PIN_ID) != BUTTON_DEATTACH) {
    enter_mode = mode;
    Serial.printf("Mode: %d\n", enter_mode);
    delay(100);
    enter_mode = PENGAMBILAN;
    mode = 0;
  }
  if (enter_mode == PENGAMBILAN) {
    readmfrc522();
    getFingerprintID();
  } else if (enter_mode == PENGEMBALIAN) {
    readmfrc522();
    getFingerprintID();
    delay(100);
    enter_mode = PENGAMBILAN;
    mode = 0;
  } else if (enter_mode == REGIST_FINGER) {
    ID = 0;
    delay(50);

    Serial.print("Mode: ");
    Serial.println(enter_mode);
    Serial.print("Tambah variable\t");
    Serial.println(ID);
    delay(500);

    while (1) {
      if (digitalRead(PIN_ID) != BUTTON_DEATTACH) {
        ID++;
        Serial.print("ID: ");
        Serial.println(ID);
        delay(300);
      }

      if (digitalRead(PIN_MOD) != BUTTON_DEATTACH) {
        Serial.println("Enter ID");
        delay(300);
        break;
      }
    }

    if (ID == 0)
      return;
    while (!getFingerprintEnroll())
      ;
    enter_mode = PENGAMBILAN;
    mode = 0;
    ID = 0;
  } else if (enter_mode == DELETE_FINGER) {
    ID = 0;
    delay(50);

    while (1) {
      Serial.print("Delete ID (while): ");
      Serial.println(ID);
      if (digitalRead(PIN_ID) != BUTTON_DEATTACH) {
        ID++;
        Serial.print("Delete ID (while): ");
        Serial.println(ID);
        delay(300);
      }

      if (digitalRead(PIN_MOD) != BUTTON_DEATTACH) {
        delay(300);
        break;
      }
    }

    Serial.print("Delete ID: ");
    Serial.println(ID);
    delay(100);

    if (ID == 0)
      return;
    while (deleteFingerprint(ID) == -1)
      ;

  } else {
    enter_mode = PENGAMBILAN;
    mode = 0;
    ID = 0;
  }
  delay(50);
}

void readmfrc522() {

  if (!mfrc522.PICC_IsNewCardPresent())
    return;
  if (mfrc522.PICC_ReadCardSerial()) {
    for (byte i = 0; i < 4; i++) {
      tag += mfrc522.uid.uidByte[i];
    }
    Serial.println(tag);
    if (enter_mode == PENGAMBILAN) {
      if (tag == "211771437" || tag == "191835414") {
        // matched_tag_flag = true;

        Serial.println(word_send);
        digitalWrite(SOLENOID_PIN, LOW);
        word_send = "Open the Solenoid(RFID)";
        Blynk.virtualWrite(V0, word_send);
        Serial.println(word_send);
        delay(5000);

      } else {
        // not_matched_tag_flag = true;
        Serial.println("Access Denied!");
      }

      tag = "";

      // matched_tag_flag = true;
      digitalWrite(SOLENOID_PIN, HIGH);
      word_send = "Close the Solenoid";
      Blynk.virtualWrite(V0, word_send);
      Serial.println(word_send);

      delay(1000);

      word_send = "Please tap your RFID Card or Finger";
      Serial.println(word_send);
    } else if (enter_mode == PENGEMBALIAN) {
      if (tag == "211771437") {
        // matched_tag_flag = true;

        Serial.println(word_send);
        digitalWrite(SOLENOID_PIN, LOW);
        word_send = "Open the Solenoid(RFID)";
        Blynk.virtualWrite(V0, word_send);
        Serial.println(word_send);
        delay(5000);

      } else {
        // not_matched_tag_flag = true;
        Serial.println("Access Denied!");
      }

      tag = "";

      // matched_tag_flag = true;
      digitalWrite(SOLENOID_PIN, HIGH);
      word_send = "Close the Solenoid";
      Blynk.virtualWrite(V0, word_send);
      Serial.println(word_send);

      delay(1000);

      word_send = "Please tap your RFID Card or Finger";
      Serial.println(word_send);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void myDasboard() {
  Blynk.virtualWrite(V0, word_send);
  word_send = "";
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      // Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  if (p == FINGERPRINT_OK) {
    Serial.println(word_send);
    digitalWrite(SOLENOID_PIN, LOW);
    word_send = "Open the Solenoid(Finger)";
    Blynk.virtualWrite(V0, word_send);

    delay(5000);
  }
  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  // matched_tag_flag = true;
  digitalWrite(SOLENOID_PIN, HIGH);
  word_send = "Close the Solenoid";
  Blynk.virtualWrite(V0, word_send);
  delay(1000);

  word_send = "Please tap your RFID Card or Finger";

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  Serial.println(word_send);
  digitalWrite(SOLENOID_PIN, LOW);
  word_send = "Open the Solenoid";
  Blynk.virtualWrite(V0, word_send);

  delay(5000);
  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  // matched_tag_flag = true;
  digitalWrite(SOLENOID_PIN, HIGH);
  word_send = "Close the Solenoid";
  Blynk.virtualWrite(V0, word_send);
  delay(1000);

  word_send = "Please tap your RFID Card or Finger";

  return finger.fingerID;
}

uint8_t deleteFingerprint(uint8_t id) {

  uint8_t p = -1;
  p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
    enter_mode = PENGAMBILAN;
    mode = 0;
    ID = 0;

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");

  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");

  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");

  } else {
    Serial.print("Unknown error: 0x");
    Serial.println(p, HEX);
  }
  return p;
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(ID);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(ID);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(ID);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(ID);
  p = finger.storeModel(ID);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
  word_send = "ID Stored";
  Blynk.virtualWrite(V0, word_send);
  delay(500);
  word_send = "Please tap your RFID Card or Finger";
  return true;
}