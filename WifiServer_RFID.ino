#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* ssid = "IOTLAB";
const char* password = "IOTLAB0987";

const char* serverName = "http://192.168.0.172:4000/api/test";

const int max_connection_attempts = 15;
int connection_attempts = 0;

#define RST_PIN 22      // Configurable, see typical pin layout above
#define SS_PIN 5        // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial.println("Brownout detector disabled");
  connectToWiFi();
  SPI.begin();            // Init SPI bus
  mfrc522.PCD_Init();     // Init MFRC522
}

void connectToWiFi()
{
  if (WiFi.status() != WL_CONNECTED) {
    if (connection_attempts < max_connection_attempts) {
      connection_attempts++;
      Serial.print("Connecting to WiFi... (attempt: ");
      Serial.print(connection_attempts);
      Serial.println(')');
      WiFi.begin(ssid, password);
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < max_connection_attempts) {
        delay(1000);
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
      } else {
        Serial.println("Failed to connect to WiFi after multiple attempts. Exiting.");
        while (true) {
          delay(1000);
        }
      }
    } else {
      Serial.println("Failed to connect to WiFi after multiple attempts. Exiting.");
      while (true) {
        delay(1000);
      }
    }
  } else {
    Serial.println("WiFi already connected!");
  }
}



void sendPostRequest(String tagNumber)
{
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"rfid\": \"" + tagNumber + "\", \"weight\": \"weight_number\"}"; // Replace this with your payload data
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String response = http.getString(); // Read the server's response
      Serial.print("Server response: ");
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Error in WiFi connection");
    connectToWiFi();
  }
}

void loop()
{
  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Dump UID in decimal
    String tagNumber = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      tagNumber += String(mfrc522.uid.uidByte[i], DEC);
    }
    Serial.print("Tag Number: ");
    Serial.println(tagNumber);
    sendPostRequest(tagNumber);
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  }
  delay(20); // Wait for 1 second before reading the next RFID tag
}
