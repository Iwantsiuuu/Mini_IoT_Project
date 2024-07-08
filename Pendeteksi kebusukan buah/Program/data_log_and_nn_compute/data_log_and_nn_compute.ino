// https://docs.google.com/spreadsheets/d/1o3_NIqMc8vNBYaXiEh0fg9Ox9uWHBVv6g7TmUSt7B08/edit?pli=1#gid=0

// Example Arduino/ESP8266 code to upload data to Google Sheets
// Follow setup instructions found here:
// https://github.com/StorageB/Google-Sheets-Logging
// 
// email: StorageUnitB@gmail.com


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"

// Enter network credentials:
const char* ssid     = "lina";
const char* password = "B4tam2022#";

// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbxg71SGQVjapVHDcv6w6Pll2Xp1wbIuKx7K0iBpi-63lG2VreDuFE3wyImwtr-7JkPQAw";

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

// Google Sheets setup (do not edit)
const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

// Declare variables that will be published to Google Sheets
int value0 = 0;
int value1 = 0;
int value2 = 0;

const uint8_t inputNode = 4;
const uint8_t hiddenNode = 4;
const uint8_t outputNode = 1;

const float weight_1[hiddenNode][inputNode] = {
  { 0.1650, 0.5009, 0.8083, -0.7957 },
  { 2.8969, -2.9054, 0.1244, 0.0356 },
  { 3.0390, -0.8884, -0.3069, -0.2094 },
  { -2.9047, 1.2746, -0.5498, -0.4430 }
};

const float bias_1[] = {
  -2.5863,
  -0.2289,
  0.0085,
  0.0860
};

const float weight_2[] = {
  -0.4546,
  7.7966,
  3.9379,
  -3.7011
};

const float bias_2 = -0.7282;

const float xOffset[inputNode] = { 13.0, 14.0, 21.0, 11.0 };
const float gain[inputNode] = { 0.00851, 0.00847, 0.00897, 0.00866 };
const float yMin[inputNode] = { -1.0 };

float input_data_testing[inputNode] = { 26.0, 204.0, 88.0, 242.0 };
float output = 0.0;
float hiddenLayer[hiddenNode];
float norm_input[inputNode];
float accumtion;
int x, y, i, j;

void nn_compute();
void send_to_spreadsheet(int data);

void setup() {

  Serial.begin(9600);        
  delay(10);
  Serial.println('\n');
  
  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
       flag = true;
       Serial.println("Connected");
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    return;
  }
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
}

void loop() {

  // create some fake data to publish
  nn_compute();
  send_to_spreadsheet(output);
  // a delay of several seconds is required before publishing again    
  delay(1000);
}

void send_to_spreadsheet(int data){
    static bool flag = false;
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected()){
      client->connect(host, httpsPort);
    }
  }
  else{
    Serial.println("Error creating client object!");
  }
  
  // Create json object string to send to Google Sheets
  payload = payload_base + "\"" + data + "\"}";
  
  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  if(client->POST(url, host, payload)){ 
    // do stuff here if publish was successful
  }
  else{
    // do stuff here if publish was not successful
    Serial.println("Error while connecting");
  }
}

void nn_compute() {
  for (i = 0; i < hiddenNode; i++) {
    norm_input[i] = input_data_testing[i] - xOffset[i];
    norm_input[i] = norm_input[i] * gain[i];
    norm_input[i] = norm_input[i] + yMin[i];
  }

  for (i = 0; i < hiddenNode; i++) {
    hiddenLayer[i] = 0;
  }

  for (i = 0; i < hiddenNode; i++) {
    for (j = 0; j < hiddenNode; j++) {
      hiddenLayer[i] += norm_input[j] * weight_1[i][j];
    }
    hiddenLayer[i] = (2.0 / (1.0 + exp(-(2 * hiddenLayer[i] + bias_1[i])))) - 1;
  }

  accumtion = 0;
  for (i = 0; i < hiddenNode; i++) {
    accumtion += hiddenLayer[i] * weight_2[i];
  }
  output = 1.0 / (1.0 + exp(-(accumtion + bias_2)));
  Serial.println(output);
}