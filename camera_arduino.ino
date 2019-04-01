/*
 * HardwareProg
 * Projekt: Office-Kuehlschrank-Sicherheit
 * A.3 (camera_arduino)
 * 
 * Connections:
 * CAMERA_TX  -> arduino_2
 * CAMERA_RX  -> resistor -> GND
 *            -> resistor -> arduino_3
 * CAMERA_GND -> GND
 * CAMERA_VCC -> 5V
 * 
 * SD_CS      -> arduino_10
 * SD_SCK     -> arduino_13
 * SD_MOSI    -> arduino_11
 * SD_MISO    -> arduino_12
 * SD_GND     -> GND
 * BTN_VCC    -> 5V
 */

#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define chipSelect 10

SoftwareSerial cameraconnection = SoftwareSerial(2, 3);
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);

bool snapBool = false;
String snapStr = "";
File myFile;

//=====================================================================
void setup() {
  // When using hardware SPI, the SS pin MUST be set to an
  // output (even if not connected or used).  If left as a
  // floating input w/SPI on, this can cause lockuppage.
#if !defined(SOFTWARE_SPI)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if(chipSelect != 53) pinMode(53, OUTPUT); // SS on Mega
#else
  if(chipSelect != 10) pinMode(10, OUTPUT); // SS on Uno, etc.
#endif
#endif

  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
  
  Serial.begin(9600);
  Serial.println(F("VC0706 Camera snapshot test"));
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card failed, or not present"));
    // don't do anything more:
    return;
  }  
  
  // Try to locate the camera
  if (cam.begin()) {
    Serial.println(F("Camera Found:"));
  } else {
    Serial.println(F("No camera found?"));
    return;
  }
  // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print(F("Failed to get version"));
  } else {
    Serial.println(F("-----------------"));
    Serial.print(reply);
    Serial.println(F("-----------------"));
  }

  // Set the picture size - you can choose one of 640x480, 320x240 or 160x120 
  // Remember that bigger pictures take longer to transmit!
  
  //cam.setImageSize(VC0706_640x480);        // biggest
  cam.setImageSize(VC0706_320x240);        // medium
  //cam.setImageSize(VC0706_160x120);          // small

  // You can read the size back from the camera (optional, but maybe useful?)
  uint8_t imgsize = cam.getImageSize();
  Serial.print(F("Image size: "));
  if (imgsize == VC0706_640x480) Serial.println(F("640x480"));
  if (imgsize == VC0706_320x240) Serial.println(F("320x240"));
  if (imgsize == VC0706_160x120) Serial.println(F("160x120"));

  Serial.println(F("setup done"));
}

//=====================================================================
void loop() {
  //wait for event
  delay(100);
  if (snapBool) {
    takeSnapshot();
    snapBool = false;
    snapStr = "";
  }
}

//=====================================================================
void receiveEvent(int howMany) {
  String requestString = "";
  while (0 < Wire.available()) { // loop through all but the last
    char c = Wire.read(); // receive byte as a character
    requestString += c;
    //Serial.print(c);         // print the character
  }
  snapStr = requestString.substring(0, 10);
  Serial.print(F("receiveEvent with request: "));
  Serial.println(snapStr);
  snapBool = true;
}

//=====================================================================
void takeSnapshot() {
  Serial.println(F("takeSnapshot start.."));
  delay(100);

  if (! cam.takePicture()) 
    Serial.println(F("Failed to snap!"));
  else 
    Serial.println(F("Picture taken!"));
  
  // get filename for the picture
  char filename[13];
  strcpy(filename, "IMAGE00.JPG");
  for (int i = 0; i < 100; i++) {
    filename[5] = '0' + i/10;
    filename[6] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  // store filename & request-code (snapStr) in a log file
  myFile = SD.open("LOG.TXT", FILE_WRITE);
  Serial.print(F("FreeRam: "));Serial.println(FreeRam());
  if (myFile) {
    myFile.print(filename);
    myFile.print(",");
    myFile.println(snapStr);
    myFile.close();
    Serial.println(F("logString written to LOG.TXT"));
  } else {
    Serial.println(F("error opening LOG.TXT"));
  }
  
  
  // Open the img file for writing
  myFile = SD.open(filename, FILE_WRITE);
  Serial.print(F("FreeRam: "));Serial.println(FreeRam());
  // Get the size of the image (frame) taken  
  uint16_t jpglen = cam.frameLength();
  Serial.print(F("Storing "));
  Serial.print(jpglen, DEC);
  Serial.print(F(" byte image."));

  int32_t time = millis();
  pinMode(8, OUTPUT);
  // Read all the data up to # bytes!
  byte wCount = 0; // For counting # of writes
  while (jpglen > 0) {
    // read 32 bytes at a time;
    uint8_t *buffer;
    uint8_t bytesToRead = min(32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
    buffer = cam.readPicture(bytesToRead);
    myFile.write(buffer, bytesToRead);
    if(++wCount >= 64) { // Every 2K, give a little feedback so it doesn't appear locked up
      Serial.print('.');
      wCount = 0;
    }
    //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
    jpglen -= bytesToRead;
  }
  myFile.close();

  time = millis() - time;
  Serial.println(F("done!"));
  Serial.print(time);Serial.println(F(" ms elapsed"));
  Serial.println(F("takeSnapshot done."));
  cam.reset();
}

