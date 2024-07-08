/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6cKNnoV5Q"
#define BLYNK_TEMPLATE_NAME "IoT2"
#define BLYNK_AUTH_TOKEN "yYlGWVAa9WbJDC_elrd4AaINECZLmiBU"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "lina";
char pass[] = "B4tam2022#";

// Set up LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Set up buttons
const int navigationButtonPin = D7;  // Button 1 for menu navigation
const int testButtonPin = D8;        // Button 2 for manual test and reset alarm

// Set up buzzer
const int buzzerPin = D3;

// Set up LEDs
const int ledBluePin = D4;    // Blue LED for system ready and alarm
const int ledGreenPin = D5;   // Green LED for low noise
const int ledYellowPin = D6;  // Yellow LED for mid noise
const int ledRedPin = D0;     // Red LED for high noise and trigger alarm

// const int playPin = MOSI;
// const int pin_a_mux = 10;
// const int pin_b_mux = 9;

// Set up voice sensor
const int voiceSensorPin = A0;

// Define menu items
const char* menuItems[] = {
  "Menu Utama",
  "Melihat Nilai ADC",
  "Melihat Nilai Voltage",
  "Melihat dB/Desible",
  "Menihat IP Address",
  "Melihat Kekuatan Signal Wifi",
  "Melihat Status wifi Connection",
  "Melihat Nama Wifi Ssid Dan Passwordnya",
  "Melihat Status System ON/OFF",
  "Melihat Status Alarm",
  "Menampilkan Notif Alarm",
  "DII"
};

const int sampleWindow = 50;  // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
char buff_[16];
int db;
uint8_t logicMux = 0;
uint16_t ADC_VAL = 0;
const char* BUZZER_STATUS = "";

enum MenuState {
  MENU_MAIN,
  MENU_DESIBLE,                 //
  MENU_SSID,                    //
  MENU_ADC,                     //
  MENU_VOLTAGE,                 //
  MENU_WIFI_SIGNAL_STRENGTH,    //
  MENU_WIFI_CONNECTION_STATUS,  //
  MENU_WIFI_PASSWORD,           //
  MENU_SYSTEM_STATUS,
  MENU_ALARM_STATUS,            //
  MENU_ALARM_NOTIFICATION,
  MENU_DII
};

MenuState currentMenu = MENU_MAIN;

void setup() {
  Serial.begin(115200);

  // Initialize LCD display
  lcd.init();
  lcd.backlight();

  // Initialize buttons
  pinMode(navigationButtonPin, INPUT);
  pinMode(testButtonPin, INPUT);

  // Initialize buttons
  digitalWrite(navigationButtonPin, LOW);
  digitalWrite(testButtonPin, LOW);

  // Initialize buzzer
  pinMode(buzzerPin, OUTPUT);

  // Initialize LEDs
  pinMode(ledBluePin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledYellowPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);

  // pinMode(playPin, OUTPUT);
  // pinMode(pin_a_mux, OUTPUT);
  // pinMode(pin_b_mux, OUTPUT);

  // Initialize voice sensor
  pinMode(voiceSensorPin, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize the Blynk connection
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print("SOUND DESIBLE");
  lcd.setCursor(3, 1);
  lcd.print("DETECTION");
}

void loop() {
  Blynk.run();
  uint32_t prev_millis_send_to_blynk;
  uint32_t prev_millis;

  if (millis() < prev_millis)
    prev_millis = 0;

  if (millis() < prev_millis_send_to_blynk)
    prev_millis_send_to_blynk = 0;

  if (millis() - prev_millis > 10) {
    prev_millis = millis();
    desible_read();
    case_menu();
    monitoring();
  }

  if (millis() - prev_millis_send_to_blynk > 1000) {

    prev_millis_send_to_blynk = millis();
    // Serial.printf("Wifi connection: %d\r\n", WiFi.RSSI());

    // logicMux++;
    // if (logicMux == 1) logicMux_sound(LOW, LOW);
    // else if (logicMux == 2) logicMux_sound(LOW, HIGH);
    // else if (logicMux == 3) logicMux_sound(HIGH, LOW);
    // else if (logicMux == 4) {
    //   logicMux_sound(HIGH, HIGH);
    //   logicMux = 0;
    // }

    send_data_to_blynk();
    // Update LEDs based on voice sensor value
    if (db < 60) {
      Serial.println("Desible level Normal");
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledYellowPin, LOW);
      digitalWrite(ledRedPin, LOW);
      BUZZER_STATUS = "OFF";
      // digitalWrite(playPin, LOW);
    } else if (db >= 60 && db <= 80) {
      Serial.println("Desible level Medium");
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledYellowPin, HIGH);
      digitalWrite(ledRedPin, LOW);
      BUZZER_STATUS = "OFF";
      // digitalWrite(playPin, LOW);
    } else {
      Serial.println("Desible level HIGH");
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledYellowPin, LOW);
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(buzzerPin, HIGH);
      BUZZER_STATUS = "ON";
      // digitalWrite(playPin, HIGH);
    }
  }
}

void desible_read() {
  unsigned long startMillis = millis();  // Start of sample window
  float peakToPeak = 0;                  // peak-to-peak level

  unsigned int signalMax = 0;     //minimum value
  unsigned int signalMin = 4095;  //maximum value

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(voiceSensorPin);  //get reading from microphone
    ADC_VAL = sample;
    if (sample < 4095)  // toss out spurious readings
    {
      if (sample > signalMax) {
        signalMax = sample;  // save just the max levels
      } else if (sample < signalMin) {
        signalMin = sample;  // save just the min levels*
      }
    }
  }

  peakToPeak = signalMax - signalMin;       // max - min = peak-peak amplitude
  db = map(peakToPeak, 20, 900, 49.5, 90);  //calibrate for deciBels
  // Serial.printf("Voice: %d\r\n", db);
}

void monitoring() {
  char buff_[16];
  // Handle button presses
  if (digitalRead(navigationButtonPin) == HIGH) {
    currentMenu = (MenuState)((currentMenu + 1) % (sizeof(menuItems) / sizeof(menuItems[0])));
    Serial.printf("Current menu: %d\n", currentMenu);
  }
  if (digitalRead(testButtonPin) == HIGH) {
    // Manual test and reset alarm
    digitalWrite(buzzerPin, HIGH);
    BUZZER_STATUS = "ON";
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    BUZZER_STATUS = "OFF";
  }
}

void send_data_to_blynk() {
  // Send data to Blynk server
  Blynk.virtualWrite(V0, db);
  Blynk.virtualWrite(V1, WiFi.RSSI());    // WiFi signal strength
  Blynk.virtualWrite(V2, WiFi.status());  // WiFi connection status
  Blynk.virtualWrite(V3, WiFi.SSID());    // WiFi SSID
  Blynk.virtualWrite(V4, WiFi.psk());     // WiFi password

  // Send notification to Blynk app
  if (db > 80) {
    Blynk.virtualWrite(V8, "Alert!");
  }
}

void case_menu() {
  switch (currentMenu) {
    case MENU_ALARM_STATUS:
      lcd.clear();

      sprintf(buff_, "STATUS: %s", BUZZER_STATUS);

      lcd.setCursor(0, 0);
      lcd.print("BUZZER STATUS");
      lcd.setCursor(0, 1);
      lcd.print(buff_);
      break;

    case MENU_VOLTAGE:
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("VOLTAGE VALUE");
      lcd.setCursor(0, 1);
      lcd.print("Value: 5 V");
      break;

    case MENU_ADC:
      lcd.clear();

      sprintf(buff_, "Value: %d ", ADC_VAL);

      lcd.setCursor(0, 0);
      lcd.print("ADC VALUE");
      lcd.setCursor(0, 1);
      lcd.print(buff_);
      break;

    case MENU_DESIBLE:
      lcd.clear();

      sprintf(buff_, "Value: %d db", db);

      lcd.setCursor(0, 0);
      lcd.print("DESIBLE VALUE");
      lcd.setCursor(0, 1);
      lcd.print(buff_);
      break;

    case MENU_WIFI_SIGNAL_STRENGTH:
      lcd.clear();

      sprintf(buff_, "Value: %d db", WiFi.RSSI());

      lcd.setCursor(0, 0);
      lcd.print("WiFi Signal");
      lcd.setCursor(0, 1);
      lcd.print(buff_);
      break;

    case MENU_WIFI_CONNECTION_STATUS:
      lcd.clear();
      sprintf(buff_, "Status: %d ", WiFi.status());

      lcd.setCursor(0, 0);
      lcd.print("WiFi Status");
      lcd.setCursor(0, 1);
      lcd.print(buff_);
      break;

    case MENU_WIFI_PASSWORD:
      lcd.clear();

      sprintf(buff_, "%s", WiFi.psk());

      lcd.setCursor(0, 0);
      lcd.print("WiFi PASSWORD");
      lcd.setCursor(0, 1);
      lcd.print(buff_);
      break;

    case MENU_SSID:
      lcd.clear();
      sprintf(buff_, "%s", WiFi.SSID());

      lcd.setCursor(0, 0);
      lcd.print("WiFi SSID (NAME)");
      lcd.setCursor(0, 1);
      lcd.print(buff_);
      break;

    default:
      break;
  }
}

// void logicMux_sound(uint8_t logic_pin_a_mux, uint8_t logic_pin_b_mux) {
//   digitalWrite(pin_a_mux, logic_pin_a_mux);
//   digitalWrite(pin_b_mux, logic_pin_b_mux);
// }