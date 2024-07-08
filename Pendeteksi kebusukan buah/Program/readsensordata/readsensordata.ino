#include <WiFi.h>

#define TGS2600_PIN 25
#define TGS2602_PIN 26
#define TGS822_PIN 33

typedef struct dta {
  uint16_t tgs2600;
  uint16_t tgs2602;
  uint16_t tgs822;
} dta_t;

dta_t dataSensor;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(TGS2600_PIN, INPUT);
  pinMode(TGS2602_PIN, INPUT);
  pinMode(TGS822_PIN, INPUT);

}

void loop() {

  scan();

  delay(15000);
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