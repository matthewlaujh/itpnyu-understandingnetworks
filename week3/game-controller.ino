#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "network_secrets.h"

#define BUTTON_A_PIN A0
#define BUTTON_B_PIN A1

#define switchesNeopixelA_pin 5
#define switchesNeopixelA_numPixels 2
#define switchesNeopixelB_pin 6
#define switchesNeopixelB_numPixels 2
#define boardNeopixelA_pin 9
#define boardNeopixelA_numPixels 2
#define boardNeopixelB_pin 10
#define boardNeopixelB_numPixels 2

WiFiClient client;
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char server[] = SECRET_SERVER;
const int serverPort = 8080; 
const char yourName[] = "matthew";
bool lastConnectState = false;

bool isConnected = false;
unsigned long lastTimeSent = 0;
const int sendInterval = 10;

Adafruit_NeoPixel switchesNeopixelA = Adafruit_NeoPixel(switchesNeopixelA_numPixels, switchesNeopixelA_pin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel switchesNeopixelB = Adafruit_NeoPixel(switchesNeopixelB_numPixels, switchesNeopixelB_pin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel boardNeopixelA = Adafruit_NeoPixel(boardNeopixelA_numPixels, boardNeopixelA_pin, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel boardNeopixelB = Adafruit_NeoPixel(boardNeopixelB_numPixels, boardNeopixelB_pin, NEO_GRBW + NEO_KHZ800);

int lastButtonAState = HIGH;
int lastButtonBState = HIGH;
int buttonAState;
int buttonBState;
int selectedLetterIndex = 0;
unsigned long lastDebounceTimeA = 0;
unsigned long lastDebounceTimeB = 0;
unsigned long debounceDelay = 10;
bool buttonAPressed = false;
bool buttonBPressed = false;
unsigned long bothButtonsPressStart = 0;
bool bothButtonsPressed = false;
const unsigned long disconnectReconnectDelay = 500;

const char letters[] = {'w', 's', 'a', 'd'};

// Color definitions
const int COLOR_W_RGB = 0x00FFFF;  // Teal for RGB LEDs
const int COLOR_A_RGB = 0xFF8000;  // Orange for RGB LEDs
const int COLOR_S_RGB = 0x800080;  // Purple for RGB LEDs
const int COLOR_D_RGB = 0xFFFF00;  // Yellow for RGB LEDs

// RGBW color components
const int COLOR_W_R = 0, COLOR_W_G = 255, COLOR_W_B = 255, COLOR_W_W = 0;  // Teal
const int COLOR_A_R = 255, COLOR_A_G = 128, COLOR_A_B = 0, COLOR_A_W = 0;  // Orange
const int COLOR_S_R = 128, COLOR_S_G = 0, COLOR_S_B = 128, COLOR_S_W = 0;  // Purple
const int COLOR_D_R = 255, COLOR_D_G = 255, COLOR_D_B = 0, COLOR_D_W = 0;  // Yellow

unsigned long lastFlashTime = 0;
const unsigned long flashInterval = 250; // 500ms on, 500ms off
bool isLedOn = false;
bool isPrinted = false;
unsigned long printStartTime = 0;
const unsigned long printDuration = 2000; // 1 second solid after printing

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  initializeNeoPixels();
  connectToWiFi();
  if (!client.connected()) {
    Serial.println("connecting");
    client.connect(server, serverPort);
  } else {  // else disconnect:
  Serial.println("disconnecting");
    client.print("x");
    client.stop();
  }
}

void loop() {
  handleButtons();

  if (client.connected() && (millis() - lastTimeSent > sendInterval)) {
  handleButtonA();
  handleButtonB();
  updateNeoPixels();
  }
    // if there's incoming data from the client, print it serially:
  if (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

// if connection state has changed:
  if (lastConnectState != client.connected()) {
    if (lastConnectState == false) {
      // send your name, and change to name display ("i");
      client.print("=");
      client.println(yourName);
    }
    lastConnectState = client.connected();
  }
}

void initializeNeoPixels() {
  switchesNeopixelA.begin();
  switchesNeopixelA.setBrightness(255);
  switchesNeopixelA.show();

  switchesNeopixelB.begin();
  switchesNeopixelB.setBrightness(255);
  switchesNeopixelB.show();

  boardNeopixelA.begin();
  boardNeopixelA.setBrightness(255);
  boardNeopixelA.show();

  boardNeopixelB.begin();
  boardNeopixelB.setBrightness(255);
  boardNeopixelB.show();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (true) {
    switch (WiFi.status()) {
      case WL_CONNECTED:
        Serial.println("WiFi is connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return;
        break;
    }
  }
}

void handleButtons() {
  int readingA = digitalRead(BUTTON_A_PIN);
  int readingB = digitalRead(BUTTON_B_PIN);

  if (readingA == LOW && readingB == LOW) {
    if (!bothButtonsPressed) {
      bothButtonsPressed = true;
      bothButtonsPressStart = millis();
    } else if (millis() - bothButtonsPressStart >= disconnectReconnectDelay) {
      if (client.connected()) {
        disconnectClient();
      } else {
        reconnectClient();
      }
      bothButtonsPressed = false; // Reset the flag to prevent multiple triggers
    }
  } else {
    bothButtonsPressed = false;
    handleButtonA();
    handleButtonB();
  }
}


void disconnectClient() {
  if (client.connected()) {
    Serial.println("Disconnecting...");
    client.print("x");
    client.stop();
    clearAllPixels();
    lastConnectState = false;
  }
}

void reconnectClient() {
  if (!client.connected()) {
    Serial.println("Reconnecting...");
    if (client.connect(server, serverPort)) {
      Serial.println("Reconnected!");
      client.print("=");
      client.println(yourName);
      setAllPixelsWhite();
      lastConnectState = true;
    } else {
      Serial.println("Reconnection failed!");
    }
  }
}

void setAllPixelsWhite() {
  switchesNeopixelA.fill(0xFFFFFF);
  switchesNeopixelB.fill(0xFFFFFF);
  boardNeopixelA.fill(boardNeopixelA.Color(0, 0, 0, 255)); // White for RGBW
  boardNeopixelB.fill(boardNeopixelB.Color(0, 0, 0, 255)); // White for RGBW
  showAllPixels();
}

void handleButtonA() {
  int reading = digitalRead(BUTTON_A_PIN);

  if (reading != lastButtonAState) {
    lastDebounceTimeA = millis();
  }

  if ((millis() - lastDebounceTimeA) > debounceDelay) {
    if (reading != buttonAState) {
      buttonAState = reading;

      if (buttonAState == LOW && !buttonAPressed) {
        buttonAPressed = true;
        selectedLetterIndex = (selectedLetterIndex + 1) % 4;
        Serial.print("Selected letter: ");
        Serial.println(letters[selectedLetterIndex]);
        isPrinted = false;
        isLedOn = true;
        lastFlashTime = millis();
      } else if (buttonAState == HIGH) {
        buttonAPressed = false;
      }
    }
  }

  lastButtonAState = reading;
}

void handleButtonB() {
  int reading = digitalRead(BUTTON_B_PIN);

  if (reading != lastButtonBState) {
    lastDebounceTimeB = millis();
  }

  if ((millis() - lastDebounceTimeB) > debounceDelay) {
    if (reading != buttonBState) {
      buttonBState = reading;

      if (buttonBState == LOW && !buttonBPressed) {
        buttonBPressed = true;
        client.print(letters[selectedLetterIndex]);
        Serial.print("Printed letter: ");
        Serial.println(letters[selectedLetterIndex]);
        isPrinted = true;
        printStartTime = millis();
      } else if (buttonBState == HIGH) {
        buttonBPressed = false;
      }
    }
  }

  lastButtonBState = reading;
}

void updateNeoPixels() {
  if (isPrinted && (millis() - printStartTime < printDuration)) {
    setSelectedPixels(true);
  } else if (!isPrinted) {
    if (millis() - lastFlashTime >= flashInterval) {
      isLedOn = !isLedOn;
      lastFlashTime = millis();
    }
    setSelectedPixels(isLedOn);
  } else {
    clearAllPixels();
  }
}

void setSelectedPixels(bool on) {
  clearAllPixels();
  if (!on) return;

  int colorRGB;
  int r, g, b, w;

  switch(letters[selectedLetterIndex]) {
    case 'w':
      colorRGB = COLOR_W_RGB;
      r = COLOR_W_R; g = COLOR_W_G; b = COLOR_W_B; w = COLOR_W_W;
      break;
    case 'a':
      colorRGB = COLOR_A_RGB;
      r = COLOR_A_R; g = COLOR_A_G; b = COLOR_A_B; w = COLOR_A_W;
      break;
    case 's':
      colorRGB = COLOR_S_RGB;
      r = COLOR_S_R; g = COLOR_S_G; b = COLOR_S_B; w = COLOR_S_W;
      break;
    case 'd':
      colorRGB = COLOR_D_RGB;
      r = COLOR_D_R; g = COLOR_D_G; b = COLOR_D_B; w = COLOR_D_W;
      break;
  }

  // Set colors for the appropriate pixels
  switch(letters[selectedLetterIndex]) {
    case 'w':
      switchesNeopixelA.setPixelColor(0, colorRGB);
      boardNeopixelA.setPixelColor(0, r, g, b, w);
      switchesNeopixelB.setPixelColor(0, colorRGB);
      boardNeopixelB.setPixelColor(0, r, g, b, w);
      break;
    case 'a':
      switchesNeopixelA.setPixelColor(0, colorRGB);
      boardNeopixelA.setPixelColor(0, r, g, b, w);
      switchesNeopixelB.setPixelColor(1, colorRGB);
      boardNeopixelB.setPixelColor(1, r, g, b, w);
      break;
    case 's':
      switchesNeopixelA.setPixelColor(1, colorRGB);
      boardNeopixelA.setPixelColor(1, r, g, b, w);
      switchesNeopixelB.setPixelColor(1, colorRGB);
      boardNeopixelB.setPixelColor(1, r, g, b, w);
      break;
    case 'd':
      switchesNeopixelB.setPixelColor(0, colorRGB);
      boardNeopixelB.setPixelColor(0, r, g, b, w);
      switchesNeopixelA.setPixelColor(1, colorRGB);
      boardNeopixelA.setPixelColor(1, r, g, b, w);
      break;
  }

  showAllPixels();
}

void clearAllPixels() {
  switchesNeopixelA.clear();
  switchesNeopixelB.clear();
  boardNeopixelA.clear();
  boardNeopixelB.clear();
  showAllPixels();
}

void showAllPixels() {
  switchesNeopixelA.show();
  switchesNeopixelB.show();
  boardNeopixelA.show();
  boardNeopixelB.show();
}