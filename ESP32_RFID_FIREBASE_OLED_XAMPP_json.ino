#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <U8x8lib.h>
#include <Servo.h>
#include <HTTPClient.h>
#define ON_Board_LED 2 
static const int servoPin = 2;
Servo servo1;
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#include <FirebaseESP32.h> // for ESP32
WiFiServer server(80);
const char* serverAddress = "192.168.197.37"; // e.g., "192.168.1.8" or "localhost"
const int serverPort = 80;
const int bufferSize = 128; // Adjust the buffer size as needed
char dataa[bufferSize];     // Character array to store data
int bytesRead = 0;    
int readsuccess;
byte readcard[4];
char str[32] = "";
String StrUID;

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE); 
#define SS_PIN 5
#define RST_PIN 4

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
  servo1.attach(servoPin);
  u8x8.begin();
  u8x8.setPowerSave(0);
  Serial.begin(115200);
  SPI.begin();       // Init SPI bus
  rfid.PCD_Init();   // Init MFRC522
  WiFi.begin(ssid, password);

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
const String ID = String(cardNumber);
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
    
    while(1){
        if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;

        if (client.connect(serverAddress, serverPort)) {
            // Send HTTP GET request
            client.print("GET /NodeMCU-and-RFID-RC522-IoT-Projects/get_data.php?id=");
            client.print(ID);
            client.println(" HTTP/1.1");
            client.print("Host: ");
            client.println(serverAddress);
            client.println("Connection: close");
            client.println();

            // Wait for server response
            while (client.connected()) {
                String line = client.readStringUntil('\n');
                if (line == "\r") {
                    break;
                }
            }

            // Read and print the JSON response
  if (client.available()) {
    // Read data one byte at a time and append it to the dataa array
    while (client.available() && bytesRead < bufferSize - 1) {
      char c = client.read();
      
      dataa[bytesRead++] = c;
    }

    // Null-terminate the dataa array to make it a valid string
    dataa[bytesRead] = '\0';

    // Now, dataa contains the received data as a null-terminated string
    Serial.println(dataa);
   DynamicJsonDocument doc(1024); // Adjust the size based on your JSON data size

  // Deserialize the JSON data
  deserializeJson(doc, dataa);

  // Access and print individual JSON fields
  const char* namee = doc["name"];
  const char* id = doc["id"];
  const char* gender = doc["gender"];
  const char* email = doc["email"];
  const char* mobile = doc["mobile"];

  // Print the values to the Serial Monitor
  Serial.print("Name: ");
  Serial.println(namee);
  Serial.print("ID: ");
  Serial.println(id);
  Serial.print("Gender: ");
  Serial.println(gender);
  Serial.print("Email: ");
  Serial.println(email);
  Serial.print("Mobile: ");
  Serial.println(mobile);

            }

            client.stop();
                 
        } else {
            Serial.println("Failed to connect to server");
        }

        delay(5000); // Wait before making another request
    }
          // Reset bytesRead for the next iteration
    bytesRead = 0;

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
