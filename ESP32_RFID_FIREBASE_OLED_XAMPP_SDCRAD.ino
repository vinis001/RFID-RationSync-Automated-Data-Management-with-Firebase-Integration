#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <Arduino.h>
#include <U8x8lib.h>
#include <Servo.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SD.h"
#define ON_Board_LED 2 
static const int servoPin = 2;
File dataFile;
Servo servo1;
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#include <FirebaseESP32.h> // for ESP32
WiFiServer server(80);

int readsuccess;
byte readcard[4];
char str[32] = "";
String StrUID;

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE); 
#define SS_PIN 5
#define RST_PIN 4
#define RFID_SS_PIN 5   // RFID CS pin
#define SD_SS_PIN 15    // SD card CS pin
static const int spiClk = 1000000; // 1 MHz
SPIClass spiSD;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];

// Define your Wi-Fi credentials
const char *ssid = "Vini";
const char *password = "vini0000";

// Define your Firebase project credentials
#define FIREBASE_HOST "https://rfid-data-d461e-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyCJFN74ZDRhy0_N-X_y-6MiDmzYxkYwKCs"

// Initialize Firebase
FirebaseData firebaseData;
// Counter to keep track of the card numbers
int cardCounter = 1;

void setup()
{
  pinMode(RFID_SS_PIN, OUTPUT); // Set RFID SS pin as an output
  digitalWrite(RFID_SS_PIN, HIGH); // Deselect RFID module

  pinMode(SD_SS_PIN, OUTPUT); // Set SD card SS pin as an output
  digitalWrite(SD_SS_PIN, HIGH); // Deselect SD card module

  spiSD.begin(); // Initialize the SPI for the SD card module
   dataFile = SD.open("AT64GAIN.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.print(',');
    dataFile.print(',');
    dataFile.println(',');
    dataFile.close();
  } else {
    Serial.println("error opening test.txt");
  }
  servo1.attach(servoPin);
  u8x8.begin();
  u8x8.setPowerSave(0);
  Serial.begin(115200);
  SPI.begin();       // Init SPI bus
  rfid.PCD_Init();   // Init MFRC522
  WiFi.begin(ssid, password);
  // Check if the RFID module is responding
  if (rfid.PCD_PerformSelfTest()) {
    Serial.println("RFID module is connected and initialized.");
  } else {
    Serial.println("RFID module is not responding or not connected.");
  }

  // Initialize the SD card module
  if (SD.begin(SD_SS_PIN)) {
    Serial.println("SD card module is connected and initialized.");
  } else {
    Serial.println("SD card module is not detected or not connected.");
  }
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
    Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connected to WiFi");

  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Serial.println("Please tag a card or keychain to see the UID !");
  Serial.println("");
}

void loop()
{
   //readsuccess = getid();
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check if the PICC is of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K)
  {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {

  Serial.print("THE UID OF THE SCANNED CARD IS : ");

  for (int i = 0; i < 4; i++) {
    readcard[i] = rfid.uid.uidByte[i];
    array_to_string(readcard, 4, str);
    StrUID = str;
  }
    HTTPClient http;

    String UIDresultSend, postData;
    UIDresultSend = StrUID;

    postData = "UIDresult=" + UIDresultSend;

    http.begin("http:// 192.168.182.37/NodeMCU-and-RFID-RC522-IoT-Projects/getUID.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpCode = http.POST(postData);
    String payload = http.getString();

    Serial.println(UIDresultSend);
    Serial.println(httpCode);
    Serial.println(payload);

    http.end();
    delay(1000);
    Serial.println(F("A new card has been detected."));
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
    // Store NUID into nuidPICC array
  String cardNumber = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] < 0x10)
    {
      cardNumber += "0"; // Pad with leading zero if necessary
    }
    cardNumber += String(rfid.uid.uidByte[i], HEX);
  }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

String cardNumber1 = String(cardNumber);  // Convert cardCounter to a String

// Concatenate the prefix and cardNumber into a single String
String path =cardNumber1;

// Now, you can convert path to a const char* if needed
const char* finalPath = path.c_str();

    u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
    
    u8x8.drawString(3,3,finalPath);
    u8x8.refreshDisplay();
    sendCardNumberToFirebase(cardNumber);
    cardCounter++;
    for(int posDegrees = 0; posDegrees <= 180; posDegrees++) {
        servo1.write(posDegrees);
        Serial.println(posDegrees);
        delay(20);
    }

    for(int posDegrees = 180; posDegrees >= 0; posDegrees--) {
        servo1.write(posDegrees);
        Serial.println(posDegrees);
        delay(20);
    }

  
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void printHex(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Send the card number to Firebase with the "CardNoX" path.
 */
void sendCardNumberToFirebase(String cardNumber)
{
  // Define the Firebase path with the card number
  String path = "/rfid_data/CardNo" + String(cardCounter);

  // Send data to Firebase
  if (Firebase.setString(firebaseData, path, cardNumber))
  {
    Serial.println("Card number sent to Firebase successfully");
  }
  else
  {
    Serial.println("Failed to send card number to Firebase");
    Serial.println(firebaseData.errorReason());
  }
}

void array_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
  }
  buffer[len * 2] = '\0';
}
