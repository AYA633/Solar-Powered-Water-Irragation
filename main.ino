#include <SoftwareSerial.h>

#define SOIL_MOISTURE_PIN A0
#define TRIG_PIN 2
#define ECHO_PIN 3
#define RELAY_PIN 4
#define GSM_TX 7
#define GSM_RX 8

SoftwareSerial gsmSerial(GSM_RX, GSM_TX);

// Array of mobile numbers
const char* phoneNumbers[] = {
  "+639946891826",
  "+639683314905",
  "+639913610922",
  "+639655546571",
  "+639922796971",
  "+639940491334",
};

const int phoneCount = sizeof(phoneNumbers) / sizeof(phoneNumbers[0]);

bool alreadyWatered = false;

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);

  Serial.println("System Initialized.");

  initializeGSM();
}

void loop() {
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  long distance = getDistance();

  Serial.print("Soil Moisture Value: ");
  Serial.println(soilMoistureValue);

  Serial.print("Distance: ");
  if (distance != -1) {
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("Out of range or read error.");
  }

  // Soil moisture logic - Adjusted threshold to 200
  if (soilMoistureValue < 400 && !alreadyWatered) {
    Serial.println("Soil is dry. Activating water pump and sending SMS...");
    sendSMS("Soil moisture is low! Watering the plants.");
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);
    digitalWrite(RELAY_PIN, LOW);
    alreadyWatered = true;
    Serial.println("Watering done.");
  } else if (soilMoistureValue > 400) {
    alreadyWatered = false;
  }

  // Obstacle detection logic
  if (distance != -1 && distance < 40) {
    Serial.println("Object detected within 40 cm. Sending SMS...");
    sendSMS("Object detected within 40 cm!");
  }

  readSMS(); // Check for incoming SMS

  Serial.println("Waiting for next reading...\n");
  delay(5000); // 5 second delay before next loop
}

long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) {
    return -1; // Error or no echo
  }
  return duration * 0.034 / 2;
}

void initializeGSM() {
  gsmSerial.println("AT");
  delay(1000);

  gsmSerial.println("AT+CMGF=1"); // SMS text mode
  delay(1000);

  gsmSerial.println("AT+CNMI=1,2,0,0,0"); // New SMS shown directly
  delay(1000);

  Serial.println("GSM Module Initialized.");
}

void sendSMS(String message) {
  for (int i = 0; i < phoneCount; i++) {
    gsmSerial.print("AT+CMGS=\"");
    gsmSerial.print(phoneNumbers[i]);
    gsmSerial.println("\"");
    delay(1000); // Wait for > prompt

    gsmSerial.println(message);
    delay(500); // Let it prepare message
    gsmSerial.write(26); // CTRL+Z to send
    delay(3000); // Wait for sending

    if (gsmSerial.available()) {
      String response = gsmSerial.readString();
      Serial.print("Response: ");
      Serial.println(response);

      if (response.indexOf("OK") != -1) {
        Serial.print("SMS sent to: ");
        Serial.println(phoneNumbers[i]);
      } else {
        Serial.print("Failed to send SMS to: ");
        Serial.println(phoneNumbers[i]);
      }
    } else {
      Serial.println("No response from GSM module for: " + String(phoneNumbers[i]));
    }
  }
}

void readSMS() {
  gsmSerial.println("AT+CMGL=\"REC UNREAD\""); // Read unread messages
  delay(1000);

  while (gsmSerial.available()) {
    String response = gsmSerial.readString();
    Serial.println("Received SMS:");
    Serial.println(response);

    if (response.indexOf("WATER") != -1) {
      Serial.println("Manual watering command received.");
      digitalWrite(RELAY_PIN, HIGH);
      delay(5000);
      digitalWrite(RELAY_PIN, LOW);
      sendSMS("Manual watering activated via SMS.");
    }
  }
}
