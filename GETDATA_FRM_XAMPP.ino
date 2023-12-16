#include <ArduinoJson.h>

void setup() {
  Serial.begin(115200);

  // Your JSON data in C format
  const char jsonData[] = "{\"name\":\"VINYAS\",\"id\":\"477B7576\",\"gender\":\"Male\",\"email\":\"shettyvinyas1@gmail.com\",\"mobile\":\"9480719963\"}";

  // Create a JSON document to parse the data
  DynamicJsonDocument doc(1024); // Adjust the size based on your JSON data size

  // Deserialize the JSON data
  DeserializationError error = deserializeJson(doc, jsonData);

  // Check for parsing errors
  if (error) {
    Serial.print(F("JSON parsing failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Access and print individual JSON fields
  const char* name = doc["name"];
  const char* id = doc["id"];
  const char* gender = doc["gender"];
  const char* email = doc["email"];
  const char* mobile = doc["mobile"];

  // Print the values to the Serial Monitor
  Serial.print("Name: ");
  Serial.println(name);
  Serial.print("ID: ");
  Serial.println(id);
  Serial.print("Gender: ");
  Serial.println(gender);
  Serial.print("Email: ");
  Serial.println(email);
  Serial.print("Mobile: ");
  Serial.println(mobile);
}

void loop() {
  // Your code here
}
