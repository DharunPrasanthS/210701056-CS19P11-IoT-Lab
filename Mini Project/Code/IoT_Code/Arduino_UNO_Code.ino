#include <Arduino.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Initialize LCD and SoftwareSerial
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(2, 3);

// Define sensor pins and other constants
const int trigPin = 7;
const int echoPin = 6;
const int mqPin = A0;
const int ledPin = 12;
const int buzzerPin = 11;
int distance = 0;
int gasValue = 0;
bool oilStatus=false;
bool gasStatus=false;
bool keyPadClicked = true;

void setup() {
    // Initialize serial communication and LCD
    Serial.begin(9600);
    delay(2000);
    Serial.println("Process Started...");
    mySerial.begin(9600);
    lcd.init();
    lcd.backlight();
    
    // Set pin modes
    pinMode(ledPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(mqPin, INPUT);
}

void loop() {
    // Check oil and gas sensor status
    oilStatus = OilSensor();
    gasStatus = GasSensor();
    bool overallStatus = oilStatus && gasStatus;
    if (overallStatus) {
        // If both sensors are okay, display good status
        ledGood();
        buzzerGood();
        lcdGood();
    } else {
        // If any sensor detects a problem, display bad status, sound alarm, and send alert
        ledBad();
        buzzerBad();
        lcdBad();
        sendAlert();
        while(keyPadClicked){
            receiveKeyPad();
            delay(2000);
        }
    }
    // Send sensor data periodically
    sendStoreData();
    delay(3000);
}

// Function to check oil sensor status
bool OilSensor() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;
    Serial.print(distance);
    Serial.println(" cm");
    return distance <= 8;
}

// Function to check gas sensor status
bool GasSensor() {
    gasValue = analogRead(mqPin);
    Serial.print(gasValue);
    Serial.println(" units");
    return gasValue <= 1500;
}

// Functions to control LED and buzzer for good and bad status
void ledGood() {
    digitalWrite(ledPin,HIGH);
    delay(750);
    digitalWrite(ledPin,LOW);
    delay(750);
    digitalWrite(ledPin,HIGH);
    delay(750);
    digitalWrite(ledPin,LOW);
    delay(750);
    digitalWrite(ledPin,HIGH);
}

void ledBad() {
    digitalWrite(ledPin,HIGH);
    delay(150);
    digitalWrite(ledPin,LOW);
    delay(150);
    digitalWrite(ledPin,HIGH);
    delay(150);
    digitalWrite(ledPin,LOW);
    delay(150);
    digitalWrite(ledPin,HIGH);
}

void buzzerGood() {
    digitalWrite(buzzerPin,LOW);
}

void buzzerBad() {
    digitalWrite(buzzerPin,HIGH);
}

// Functions to control LCD display for good, bad, and invalid status
void lcdGood() {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Status: GOOD");
}

void lcdBad() {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Status: BAD");
}

// Function to send sensor data over serial
void sendStoreData() {
    StaticJsonDocument<128> sendData;
    sendData["informationType"] = "data";
    sendData["oil"]["oilValue"] = distance;
    sendData["oil"]["oilStatus"] = oilStatus;
    sendData["gas"]["gasValue"] = gasValue;
    sendData["gas"]["gasStatus"] = gasStatus;
    sendData["timestamp"] = 0;
    String sendJsonString;
    serializeJson(sendData, sendJsonString);
    mySerial.println(sendJsonString);
    Serial.println(sendJsonString);
}

// Function to send alert over serial
void sendAlert() {
  if(OilSensor()){
    mySerial.println("{\"informationType\":\"alert\",\"sectorNo\":1}");
    Serial.println("{\"informationType\":\"alert\",\"sectorNo\":1}");
  }
  else{
    mySerial.println("{\"informationType\":\"alert\",\"sectorNo\":2}");
    Serial.println("{\"informationType\":\"alert\",\"sectorNo\":2}");
  }
}

// Function to receive keypad input over serial
void receiveKeyPad() {
    if (mySerial.available() > 0) {
        StaticJsonDocument<128> receiveData;
        char receiveJsonBuffer[128];
        mySerial.readBytesUntil('\n', receiveJsonBuffer, sizeof(receiveJsonBuffer));
        DeserializationError error = deserializeJson(receiveData, receiveJsonBuffer);
        if (error) {
            Serial.println("Failed to parse JSON...");
            Serial.println(error.c_str());
        }
        Serial.println(receiveJsonBuffer);
        if (strcmp(receiveData["informationType"], "key") == 0) {
            keyPadClicked = false;
        } else {
            keyPadClicked = true;
        }
    }
}