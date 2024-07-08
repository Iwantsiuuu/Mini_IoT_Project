#include "WiFi.h"
#include <HTTPClient.h>
//----------------------------------------

#define TGS2600_PIN 25
#define TGS2602_PIN 26
#define TGS822_PIN 33

typedef struct dta {
  uint16_t tgs2600;
  uint16_t tgs2602;
  uint16_t tgs822;
} dta_t;

dta_t dataSensor;

// Defining LED PINs on the ESP32 Board.
#define On_Board_LED_PIN 2

// Defines the DHT11 PIN and DHT sensor type.
// #define DHTPIN  4
// #define DHTTYPE DHT11

// Defines the PIN for the Switches.
// #define Switch_1_PIN   13
// #define Switch_2_PIN   12

//----------------------------------------SSID and PASSWORD of your WiFi network.
const char* ssid = "REPLACE_WITH_YOUR_SSID";          //--> Your wifi name
const char* password = "REPLACE_WITH_YOUR_PASSWORD";  //--> Your wifi password
//----------------------------------------

// Google script Web_App_URL.
String Web_App_URL = "https://script.google.com/macros/s/AKfycbxxzZysAN1icTiDN9SqyObtAG-SFI2MvnqVV9xadWdsLrJ5ejVUTOIQd58oNGNc5E5RxQ/exec";

void scan();
uint16_t readData(uint16_t pin);

// String Switch_1_State = "";
// String Switch_2_State = "";

// String Status_Read_Sensor = "";
// float Temp;
// int Humd;

// Initialize DHT as dht11.
// DHT dht11(DHTPIN, DHTTYPE);

//________________________________________________________________________________Getting_DHT11_Sensor_Data()
// Subroutine for getting temperature and humidity data from the DHT11 sensor.
// void Getting_DHT11_Sensor_Data() {
//   // Reading temperature or humidity takes about 250 milliseconds!
//   // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
//   Humd = dht11.readHumidity();
//   // Read temperature as Celsius (the default)
//   Temp = dht11.readTemperature();

//   // Check if any reads failed and exit early (to try again).
//   if (isnan(Humd) || isnan(Temp)) {
//     Serial.println();
//     Serial.println(F("Failed to read from DHT sensor!"));
//     Serial.println();

//     Status_Read_Sensor = "Failed";
//     Temp = 0.00;
//     Humd = 0;
//   } else {
//     Status_Read_Sensor = "Success";
//   }

//   Serial.println();
//   Serial.println("-------------");
//   Serial.print(F("Status_Read_Sensor : "));
//   Serial.print(Status_Read_Sensor);
//   Serial.print(F(" | Humidity : "));
//   Serial.print(Humd);
//   Serial.print(F("% | Temperature : "));
//   Serial.print(Temp);
//   Serial.println(F("°C"));
//   Serial.println("-------------");
// }
//________________________________________________________________________________

//________________________________________________________________________________Read_Switches_State()
// Subroutine to get the state of the Switches.
// void Read_Switches_State() {
//   if (digitalRead(Switch_1_PIN) == LOW) Switch_1_State = "On";
//   if (digitalRead(Switch_1_PIN) == HIGH) Switch_1_State = "Off";

//   if (digitalRead(Switch_2_PIN) == LOW) Switch_2_State = "On";
//   if (digitalRead(Switch_2_PIN) == HIGH) Switch_2_State = "Off";

//   Serial.println();
//   Serial.println("-------------");
//   Serial.print(F("Switch_1 : "));
//   Serial.print(Switch_1_State);
//   Serial.print(F(" | Switch_2 : "));
//   Serial.println(Switch_2_State);
//   Serial.println("-------------");
// }
//________________________________________________________________________________

//________________________________________________________________________________VOID SETUP()
void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println();
  delay(1000);

  // pinMode(Switch_1_PIN, INPUT_PULLUP);
  // pinMode(Switch_2_PIN, INPUT_PULLUP);

  pinMode(On_Board_LED_PIN, OUTPUT);
  pinMode(TGS2600_PIN, INPUT);
  pinMode(TGS2602_PIN, INPUT);
  pinMode(TGS822_PIN, INPUT);

  //----------------------------------------Set Wifi to STA mode
  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  WiFi.mode(WIFI_STA);
  Serial.println("-------------");
  //----------------------------------------

  //----------------------------------------Connect to Wi-Fi (STA).
  Serial.println();
  Serial.println("------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  //:::::::::::::::::: The process of connecting ESP32 with WiFi Hotspot / WiFi Router.
  // The process timeout of connecting ESP32 with WiFi Hotspot / WiFi Router is 20 seconds.
  // If within 20 seconds the ESP32 has not been successfully connected to WiFi, the ESP32 will restart.
  // I made this condition because on my ESP32, there are times when it seems like it can't connect to WiFi, so it needs to be restarted to be able to connect to WiFi.

  int connecting_process_timed_out = 20;  //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(On_Board_LED_PIN, HIGH);
    delay(250);
    digitalWrite(On_Board_LED_PIN, LOW);
    delay(250);
    if (connecting_process_timed_out > 0) connecting_process_timed_out--;
    if (connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }

  digitalWrite(On_Board_LED_PIN, LOW);

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("------------");
  //::::::::::::::::::
  //----------------------------------------

  delay(100);

  Serial.println();
  Serial.println("DHT11 Begin");
  Serial.println();
  delay(1000);

  // dht11.begin();

  delay(2000);
}
//________________________________________________________________________________

//________________________________________________________________________________VOID LOOP()
void loop() {
  // put your main code here, to run repeatedly:

  // Calling the "Getting_DHT11_Sensor_Data()" subroutine.
  // Getting_DHT11_Sensor_Data();

  // Calling the "Read_Switches_State()" subroutine.
  // Read_Switches_State();

  void scan();

  //----------------------------------------Conditions that are executed when WiFi is connected.
  // This condition is the condition for sending or writing data to Google Sheets.
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(On_Board_LED_PIN, HIGH);

    // Create a URL for sending or writing data to Google Sheets.
    String Send_Data_URL = Web_App_URL + "?sts=write";
    Send_Data_URL += "&srs=" + String(dataSensor.tgs2600);
    Send_Data_URL += "&temp=" + String(dataSensor.tgs2602);
    Send_Data_URL += "&humd=" + String(dataSensor.tgs822);
    // Send_Data_URL += "&swtc1=" + Switch_1_State;
    // Send_Data_URL += "&swtc2=" + Switch_2_State;

    Serial.println();
    Serial.println("-------------");
    Serial.println("Send data to Google Spreadsheet...");
    Serial.print("URL : ");
    Serial.println(Send_Data_URL);

    //::::::::::::::::::The process of sending or writing data to Google Sheets.
    // Initialize HTTPClient as "http".
    HTTPClient http;

    // HTTP GET Request.
    http.begin(Send_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Gets the HTTP status code.
    int httpCode = http.GET();
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    // Getting response from google sheets.
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
      Serial.println("Payload : " + payload);
    }

    http.end();
    //::::::::::::::::::

    digitalWrite(On_Board_LED_PIN, LOW);
    Serial.println("-------------");
  }
  //----------------------------------------

  delay(10000);
}

void scan() {
  // put your main code here, to run repeatedly:
  dataSensor.tgs2600 = readData(TGS2600_PIN);
  dataSensor.tgs2602 = readData(TGS2602_PIN);
  dataSensor.tgs822 = readData(TGS822_PIN);
}

uint16_t readData(uint16_t pin) {

  uint16_t value;
  value = analogRead(pin);
  return (value / 1023) * 5;
}