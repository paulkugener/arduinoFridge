/*
 * HardwareProg
 * Projekt: Office-Kuehlschrank-Sicherheit
 * A.3 (rfid_button_arduino)
 * 
 * Connections:
 * RFID_SDA  -> arduino_10
 * RFID_SCK  -> arduino_13
 * RFID_MOSI -> arduino_11
 * RFID_MISO -> arduino_12
 * RFID_IRQ  -> not connected
 * RFID_GND  -> GND
 * RFID_RST  -> arduino_9
 * RFID_VCC  -> 3.3V
 * 
 * BTN_S     -> arduino_3
 * BTN_-     -> GND
 * BTN_+     -> 5V
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>

#define BUTTON_PIN 3
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
bool TEST_MODE = false; // if TEST_MODE == true, we print to 'Serial Monitor', else we write to camera_arduino

int last_button_value;


void setup() {
  Wire.begin(); // join i2c bus (address optional for master)

  if (TEST_MODE) Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522

  pinMode(BUTTON_PIN, INPUT);
  last_button_value = digitalRead(BUTTON_PIN);

  if (TEST_MODE) {
    Serial.println("HardwareProg\nProjekt: Office-Kuehlschrank-Sicherheit\nA.3 (rfid_button_arduino: master_writer)\nTEST_MODE is enabled: printing to Serial Monitor");
    Serial.println();
  }
}


void loop() {
  int current_button_value = digitalRead(BUTTON_PIN);  // read input value
  if (last_button_value != current_button_value) {     // check for change
    last_button_value = current_button_value;
    if (current_button_value == HIGH) {                // check if the input is HIGH
      // button is NOT pressed
      return;
    } else {
      // button is pressed
      open_fridge();
    }
  }
}


void open_fridge() {
  if (TEST_MODE) Serial.println("fridge is open");
  
  delay(100);
  bool high_risk = true;
  String request_uid = "no_uid";
  
  // check rfid
  if (mfrc522.PICC_IsNewCardPresent()) {
    mfrc522.PICC_ReadCardSerial();
    
    // check UID
    String content = "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      //Serial.print(mfrc522.uid.uidByte[i], HEX);
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    
    if (content.substring(1) == "B2 E8 50 C3") { // check against the UID of the card/cards that you want to give access
      // access authorized
      high_risk = false;
    } else {
      // access denied
    }
    request_uid = content.substring(1);
  }
  
  // request snapshot
  String request_string = "";
  if (TEST_MODE) Serial.print("request snapshot: ");
  if (high_risk) {
    if (TEST_MODE) Serial.print("high risk, ");
    request_string += "H"; 
  } else {
    if (TEST_MODE) Serial.print("low risk, ");
    request_string += "L"; 
  }
  if (request_uid == "no_uid") {
    if (TEST_MODE) Serial.println("no UID");
    request_string += "N";
  } else {
    if (TEST_MODE) Serial.println(request_uid);
    request_string += "Y";
    request_uid.replace(" ", ""); // remove whitespace
    request_string += request_uid;
  }
  //if (TEST_MODE) Serial.println(request_string);
  char request_char_array[10];
  for (int i = 0; i < 10; i++) {
    if(request_string[i]){
      request_char_array[i] = request_string[i];
    } else {
      request_char_array[i] = '0';
    }
  }

  Wire.beginTransmission(8);   // transmit to device #8
  Wire.write(request_char_array);  // sends request_string
  Wire.endTransmission();      // stop transmitting
    
  delay(10000); // block because sending the snapshot from the camera to the sd-card takes some time
}


