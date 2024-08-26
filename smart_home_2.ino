#include <DHT.h>
#include <SPI.h>
#include <WiFi.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>  // Servo libraries for ESP32

#define DHT_SENSOR_PIN 15  // ESP32 pin GPIO23 connected to DHT11 sensor
#define DHT_SENSOR_TYPE DHT11
#define NUMPIXELS 4        // Number of 2812 lamps
#define RGB_PIN 14         // 2812 pin definition 13
#define TOUCH_PIN 2        // Key pin definition 12   מגע
#define RAIN_SENSOR_PIN 4  // The ESP32 pin GPIO25 connected to the sound sensor
#define SERVO_PIN 25       // The ESP32 pin GPIO26 connected to the relay
#define SS_PIN 5
#define RST_PIN 27
#define DOOR_SERVO_PIN 32
#define BUZZER 26
#define GAS_PIN 33
#define FAN_PIN 13

LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27 (from DIYables LCD), 16 column and 2 rows
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
Servo servo;                    // create servo object to control a servo
MFRC522 rfid(SS_PIN, RST_PIN);  // Create MFRC522 instance
Servo lockServo;

// Server on port 80
WiFiServer server(80);
String header;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// WiFi credentials
const char* ssid = "Borus";
const char* password = "borus0359";

// const char* ssid = "Bnotlea2";
// const char* password = "027193572";

int lockPos = 0;      // Locked position limit
int unlockPos = 180;  // Unlocked position limit
boolean locked = true;

byte allowedUIDs[][4] = {
  { 0xF3, 0x0D, 0xDA, 0xFD },  
  { 0xD3, 0xC5, 0x8A, 0xFA }   
};
const int allowedUIDsCount = sizeof(allowedUIDs) / sizeof(allowedUIDs[0]);

Adafruit_NeoPixel pixels(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);  //Creating light objects

int count_R = 0;
int count_G = 0;
int count_B = 0;
int count_key = 0;
int Press = 0;
int rain_state;          // the current state of rain sensor
int sensorThres = 3000;  // Threshold value for the gas sensor
bool lightOn = false;
bool fanOn = false;
bool windowOpen = false;
int red = 255, green = 255, blue = 255;

void setup() {
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  pinMode(BUZZER, OUTPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  dht_sensor.begin();  // initialize the DHT sensor
  lcd.init();          // initialize the lcd
  lcd.backlight();     // open the backlight
  pinMode(TOUCH_PIN, INPUT);
  pixels.begin();  // Initialize 2812 library functions
  pixels.show();
  pinMode(RAIN_SENSOR_PIN, INPUT);  // set ESP32 pin to input mode
  servo.attach(SERVO_PIN);          // attaches the servo on pin 9 to the servo object
  // servo.write(0);
  SPI.begin();      // Start SPI communication with reader
  rfid.PCD_Init();  // Initialization
  lockServo.attach(DOOR_SERVO_PIN);
  lockServo.write(lockPos);  // Move servo into locked position
  Serial.begin(9600);
}

void loop() {
  web();
  RFID();
  gas();
  LCD();
  pixels.clear();  // Lighting function
  touch();         //Key function
  rain();          //Raindrop sensor
  temp();
}

void web() {
  WiFiClient client = server.available();  // Check if a new client has connected
  if (client) {                            // If a new client is available
    currentTime = millis();                // Get the current time
    previousTime = currentTime;            // Set previous time to current time
    Serial.println("New Client.");         // Print a message for a new client
    String currentLine = "";               // Create an empty string to hold the current line of the request

    // While the client is connected and the timeout hasn't been reached
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();  // Update the current time

      if (client.available()) {  // If there's data available to read from the client
        char c = client.read();  // Read a character from the client
        Serial.write(c);         // Write the character to the Serial monitor
        header += c;             // Append the character to the header string

        if (c == '\n') {  // If the character is a newline
          // If the current line is empty, this marks the end of the header
          if (currentLine.length() == 0) {
            // Send a standard HTTP response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Light control
            if (header.indexOf("GET /light/toggle") >= 0) {                          // Check if the request is to toggle the light
              lightOn = !lightOn;                                                    // Toggle the light state
              setRGBColor(lightOn ? 255 : 0, lightOn ? 255 : 0, lightOn ? 255 : 0);  // Set the RGB color to white or off
            }

            // Fan control
            if (header.indexOf("GET /fan/on") >= 0) {  // Check if the request is to turn the fan on
              fanOn = true;
              digitalWrite(FAN_PIN, HIGH);                     // Turn the fan on
            } else if (header.indexOf("GET /fan/off") >= 0) {  // Check if the request is to turn the fan off
              fanOn = false;
              digitalWrite(FAN_PIN, LOW);  // Turn the fan off
            }

            // Set RGB values
            if (header.indexOf("GET /setRGB") >= 0) {                          // Check if the request is to set RGB values
              int colorIndex = header.indexOf("color=") + 6;                   // Find the index of the color parameter
              String colorHex = header.substring(colorIndex, colorIndex + 6);  // Extract the hex color value
              red = strtol(colorHex.substring(0, 2).c_str(), NULL, 16);        // Convert the red value from hex to decimal
              green = strtol(colorHex.substring(2, 4).c_str(), NULL, 16);      // Convert the green value from hex to decimal
              blue = strtol(colorHex.substring(4, 6).c_str(), NULL, 16);       // Convert the blue value from hex to decimal
              setRGBColor(red, green, blue);                                   // Set the RGB color
            }

            // Display updated HTML web page
            client.println(generateHtmlPage());  // Send the updated HTML page to the client
            break;                               // Exit the loop
          } else {
            currentLine = "";  // Clear the current line
          }
        } else if (c != '\r') {  // If the character is not a carriage return
          currentLine += c;      // Append the character to the current line
        }
      }
    }
    header = "";                             // Clear the header variable
    client.stop();                           // Stop the client connection
    Serial.println("Client disconnected.");  // Print a message for client disconnection
    Serial.println("");
  }
}

void setRGBColor(int r, int g, int b) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));  // Set the color of each pixel
  }
  pixels.show();  // Update the pixels to display the new color
}

String generateHtmlPage() {
  String htmlPage = "<!DOCTYPE html><html>";                                                                                                                                                                                 // Start the HTML document
  htmlPage += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";                                                                                                                              // Set the viewport meta tag
  htmlPage += "<link rel=\"icon\" href=\"data:,\">";                                                                                                                                                                         // Set the favicon
  htmlPage += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; background-color: #F2F2F2;}";                                                                              // Style for the HTML element
  htmlPage += ".container { width: 50%; margin: 40px auto; padding: 20px; background-color: #FFFFFF; border-radius: 15px; box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.1); }";                                                   // Style for the container
  htmlPage += ".button { background-color: #D3D3D3; border: 1px solid #000000; color: #000000; padding: 16px 40px; text-decoration: none; font-size: 25px;width:200px; margin: 2px; cursor: pointer; border-radius: 5px;}";  // Style for the buttons
  htmlPage += ".button:active { background-color: #BBBBBB; }";                                                                                                                                                               // Style for active buttons
  htmlPage += ".color-picker { width: 150px; height: 50px; border: 1px solid #000000; background-color: #fff; cursor: pointer; }";                                                                                           // Style for the color picker
  htmlPage += "h1 { font-size: 4.5em; text-shadow: 2px 2px #959595; }";                                                                                                                                                      // Style for the header
  htmlPage += "</style></head>";
  htmlPage += "<body><div class=\"container\"><h1>SmartHome</h1>";

  // Light control button
  if (lightOn) {
    htmlPage += "<button class=\"button\" onclick=\"sendCommand('light/toggle')\">Light OFF</button><br>";
  } else {
    htmlPage += "<button class=\"button\" onclick=\"sendCommand('light/toggle')\">Light ON</button><br>";
  }

  // Fan control button
  if (fanOn) {
    htmlPage += "<button class=\"button\" onclick=\"sendCommand('fan/off')\">Fan OFF</button><br>";
  } else {
    htmlPage += "<button class=\"button\" onclick=\"sendCommand('fan/on')\">Fan ON</button><br>";
  }

  // Set RGB values buttons
  htmlPage += "<div>";
  htmlPage += "<h3>Set RGB Color</h3>";
  htmlPage += "<input type=\"color\" id=\"colorPicker\" value=\"#FFFFFF\" onchange=\"setRGB()\" class=\"color-picker\">";
  htmlPage += "</div></div>";

  htmlPage += "<script>";
  htmlPage += "function sendCommand(command) {";  // JavaScript function to send a command to the server
  htmlPage += "var xhr = new XMLHttpRequest();";
  htmlPage += "xhr.open('GET', '/' + command, true);";
  htmlPage += "xhr.send();";
  htmlPage += "xhr.onreadystatechange = function() {";
  htmlPage += "if (xhr.readyState == 4 && xhr.status == 200) {";
  htmlPage += "location.reload();";  // Reload the page to update button text
  htmlPage += "}};}";
  htmlPage += "function setRGB() {";                                                     // JavaScript function to set the RGB color
  htmlPage += "var color = document.getElementById('colorPicker').value.substring(1);";  // Get the color value from the color picker
  htmlPage += "var xhr = new XMLHttpRequest();";
  htmlPage += "xhr.open('GET', '/setRGB?color=' + color, true);";
  htmlPage += "xhr.send();";
  htmlPage += "}";
  htmlPage += "</script>";

  htmlPage += "</body></html>";

  return htmlPage;  // Return the HTML page as a string
}

void RFID() {
  // Check if a new card is present and read its serial number
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;  // Exit if no new card or unable to read

  Serial.println("Card found");
  byte* uid = rfid.uid.uidByte;  // Pointer to the UID byte array

  // Check if the UID is allowed for access
  if (isAllowedUID(uid)) {
    Serial.println("Access Granted");

    // If the lock is closed, open it; otherwise, close it
    if (locked) {
      lockServo.write(unlockPos);
      locked = false;
      setRGBColor(255, 255, 255);  // Set RGB LED color to white
      lightOn = true;
    } else {
      lockServo.write(lockPos);
      locked = true;
    }
  } else {
    Serial.println("Access Denied");
  }

  rfid.PICC_HaltA();       // Halt PICC
  rfid.PCD_StopCrypto1();  // Stop encryption on PCD
}

bool isAllowedUID(byte* uid) {
  // Check if the given UID is in the allowedUIDs array
  for (int i = 0; i < allowedUIDsCount; i++) {
    if (memcmp(uid, allowedUIDs[i], 4) == 0) {
      return true;
    }
  }
  return false;
}

void gas() {
  int analogSensor = analogRead(GAS_PIN);  // Read the value from the gas sensor

  // Activate buzzer if sensor value exceeds threshold; otherwise, deactivate it
  if (analogSensor > sensorThres) {
    tone(BUZZER, 1000);
  } else {
    noTone(BUZZER);
  }
}

void RGB() {
  switch (count_key)  // Number of key presses
  {
    case 1:  // Turn on white light
      count_R++;
      if (count_R >= 255) count_R = 255;
      count_G = 0;
      count_B = 0;
      setRGBColor(count_R, count_R, count_R);
      lightOn = true;
      break;
    case 2:  // Turn on red light
      count_G++;
      if (count_G >= 255) count_G = 255;
      count_R = 0;
      count_B = 0;
      setRGBColor(count_G, 0, 0);
      lightOn = true;
      break;
    case 3:  // Turn on green light
      count_B++;
      if (count_B >= 255) count_B = 255;
      count_R = 0;
      count_G = 0;
      setRGBColor(0, count_B, 0);
      lightOn = true;
      break;
    case 4:  // Turn off all lights
      setRGBColor(0, 0, 0);
      lightOn = false;
      break;
    default: break;
  }
}

void touch()  // Key press function
{
  int i = digitalRead(TOUCH_PIN);  // Read digital state from touch pin
  if (i) {
    delay(20);
    if (i) Press = 1;  // Indicate a key press
  }

  i = digitalRead(TOUCH_PIN);
  if ((i == 0) && (Press == 1))  // Detect key release
  {
    Press = 0;
    count_key++;
    if (count_key == 5) count_key = 1;
    RGB();  // Change RGB color based on key press
  }
}

void rain() {
  rain_state = analogRead(RAIN_SENSOR_PIN);  // Read rain sensor state

  if (rain_state < 3500 && windowOpen) {  // Check if rain is detected and window is open
    Serial.println("Rain detected!");
    servo.write(0);  // Close window
    // windowOpen = false;
  }
}

void temp() {
  float tempC = dht_sensor.readTemperature();  // Read temperature sensor
  Serial.println(tempC);

  // Activate fan if temperature exceeds 27°C; otherwise, deactivate it
  if (tempC >= 27) {
    digitalWrite(FAN_PIN, HIGH);
  } else {
    digitalWrite(FAN_PIN, LOW);
    fanOn = false;
  }
}

void LCD() {
  float humi = dht_sensor.readHumidity();      // Read humidity sensor
  float tempC = dht_sensor.readTemperature();  // Read temperature sensor

  lcd.clear();  // Clear LCD display
  
  // Check if sensor readings are valid
  if (isnan(tempC) || isnan(humi)) {
    lcd.setCursor(0, 0);
    lcd.print("Failed");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(tempC);
    lcd.print((char)223);  // Print degree symbol
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Humi: ");
    lcd.print(humi);
    lcd.print("%");
  }
}
