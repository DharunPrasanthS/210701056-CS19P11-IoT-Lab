#include <SoftwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Keypad.h>

const char* ssid = "THUNDERBOLT";
const char* password = "balaharinath";
const char* storeDataURL = "http://192.168.137.100/storeData";
const char* alertURL = "http://192.168.137.100/alert";
char dataInformationType[20] = "";
char dataString[128] = "";
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {10, 11, 12, 13};
const char* keyPassword = "4201";
char enteredPassword[5] = {'\0'};

SoftwareSerial mySerial(4, 5);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("Process Started...");
  mySerial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  Serial.println("Connected to THUNDERBOLT Wifi...");
}

void loop() {
  Serial.println("Receiving Serial data...");
  receiveSerialData();
  if (strcmp(dataInformationType, "data") == 0) {
    sendStoreDataHTTPRequest();
  }
  if (strcmp(dataInformationType, "alert") == 0) {
    sendAlertHTTPRequest();
  }
  if (keyPad()) {
    mySerial.println("{\"informationType\":\"key\"}");
  }
  delay(3000);
}

void receiveSerialData() {
  if (mySerial.available() > 0) {
    StaticJsonDocument<128> receiveData;
    char receiveJsonBuffer[128];
    mySerial.readBytesUntil('\n', receiveJsonBuffer, sizeof(receiveJsonBuffer));
    DeserializationError error = deserializeJson(receiveData,receiveJsonBuffer);
    if (error) {
      Serial.println("Failed to parse JSON...");
      Serial.println(error.c_str());
    }
    strcpy(dataInformationType, receiveData["informationType"]);
    strcpy(dataString, receiveJsonBuffer);
  }
}

void sendStoreDataHTTPRequest() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, storeDataURL);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(dataString);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void sendAlertHTTPRequest() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, alertURL);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(dataString);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

boolean keyPad() {
  char key = keypad.getKey();
  if (key) {
    for (int i = 0; i < 4; i++) {
      if (enteredPassword[i] == '\0') {
        enteredPassword[i] = key;
        break;
      }
    }
  }
  if (enteredPassword[3] != '\0') {
    if (strcmp(enteredPassword, keyPassword) == 0) {
      return true;
    } else {
      memset(enteredPassword, '\0', sizeof(enteredPassword));
      return false;
    }
  }
  return false;
}