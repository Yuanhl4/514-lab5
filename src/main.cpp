#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Network credentials and Firebase configuration
const char* ssid = "UW MPSK";
const char* password = "5Y^y5Ku6tN"; // Replace with your network password
#define DATABASE_URL "https://yuanhl4-514-default-rtdb.firebaseio.com/" // Replace with your Firebase database URL
#define API_KEY "AlzaSyBuVixSW1yto3vd4_b51mqW3pHa4BxlJ64" // Replace with your Firebase API key

// HC-SR04 Pins
const int trigPin = 3;
const int echoPin = 4;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Set the deep sleep and wake-up conditions
const unsigned long DEEP_SLEEP_DURATION = 30e6;
const float DISTANCE_THRESHOLD = 30.0;
const unsigned long DETECTION_PERIOD = 30000;
// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool shouldSleep = false;

// Function prototypes
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);
float measureDistance();
void goToDeepSleep();

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  connectToWiFi();
  initFirebase();

  if (measureDistance() > DISTANCE_THRESHOLD) {
    shouldSleep = true;
  } else {
    unsigned long startMillis = millis();
    while (millis() - startMillis < DETECTION_PERIOD) {
      if (measureDistance() > DISTANCE_THRESHOLD) {
        shouldSleep = true;
        break;
      }
      delay(100);
    }
  }

  if (shouldSleep) {
    Serial.println("No movement detected. Entering deep sleep.");
    goToDeepSleep();
  } else {
    Serial.println("Movement detected. Sending data to Firebase.");
    sendDataToFirebase(measureDistance());
  }
}

void loop() {
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int retries = 0;

  while (WiFi.status() != WL_CONNECTED && retries < 5) {
    delay(1000);
    Serial.print(".");
    retries++;
  }

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi. Going to deep sleep.");
    goToDeepSleep();
  }

  Serial.println("Connected to WiFi");
}

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void sendDataToFirebase(float distance) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.pushFloat(&fbdo, "/sensor_data/distance", distance)) {
      Serial.println("Sent distance to Firebase");
    } else {
      Serial.println("Failed to send distance to Firebase");
    }
  }
}

void goToDeepSleep() {
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);
  esp_deep_sleep_start();
}