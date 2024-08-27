**Smart Home System**

This project is a Smart Home system built using an ESP32 microcontroller. The system includes various sensors and actuators that automate home functions, such as lighting, window control, gas detection, and more. Additionally, it features a simple web-based application for user control.

Features
RFID Entry: Unlock the door and turn on the lights (RGB Neopixel) using an RFID card.
Rain Detection: Automatically close the window when rain is detected.
Gas Detection: Trigger a buzzer when gas is detected.
Temperature and Humidity Display: Continuously display temperature and humidity on an LCD screen.
Automatic Fan Control: Turn on the fan when the temperature exceeds 27°C.
Motor Control: Use servo motors to open and close the window and door.
User Interface: Control the light color, window, and fan via a simple HTML-based web application.
Components
ESP32: Main microcontroller.
RFID Module: For door entry.
Neopixel RGB LED Strip: For lighting.
Rain Sensor: For detecting rain and controlling the window.
Gas Sensor: For gas detection and buzzer activation.
DHT11 Sensor: For measuring temperature and humidity.
16x2 LCD Screen: For displaying temperature and humidity.
Servo Motors: For controlling the door and window.
Buzzer: For alarm in case of gas detection.
Installation
Hardware Setup:

Connect the ESP32 to the various sensors and actuators according to the wiring diagram (to be included or described here).
Power the ESP32 and ensure all components are correctly connected.
Software Setup:

Download the Arduino IDE and install the required libraries:
Adafruit_NeoPixel
ESPAsyncWebServer
DHT
LiquidCrystal
Any other necessary libraries (list them).
Upload the provided code to the ESP32.
Web Application:

The web application is hosted on the ESP32. Access it by connecting to the ESP32's IP address from a web browser.
Usage
RFID Entry: Swipe the RFID card to unlock the door and turn on the lights.
Rain Detection: The window will automatically close when rain is detected.
Gas Detection: The buzzer will sound if gas is detected.
Temperature and Humidity: View real-time temperature and humidity on the LCD screen. The fan will turn on automatically if the temperature exceeds 27°C.
Web Application: Use the web interface to control the light color, window, and fan.
Future Enhancements
Add voice control using a service like Google Assistant.
Expand the web application with more control options and a better user interface.
Implement energy monitoring features.
Contributing
Feel free to fork this project, submit pull requests, or suggest features in the issues section.

License
This project is licensed under the MIT License - see the LICENSE file for details.
