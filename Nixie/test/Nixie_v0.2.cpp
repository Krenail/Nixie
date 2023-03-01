#include <Arduino.h>              // Basic Arduino framework library
#include <ShiftRegister74HC595.h> // Shift Registers Library
#include <ESP8266WiFi.h>          // Basic ESP WiFi Library
#include <DNSServer.h>            // DNS Server Library for AP Mode
#include <ESP8266WebServer.h>     // Server Library for AP Mode
#include <WiFiManager.h>          // WiFi Manager Library for easy AP and WiFi configuration
#include <NTPClient.h>            // NTP Client Library
#include <WiFiUdp.h>              // UDP Packets Library
#include <DHT.h>                  // DHT22 Sensor Library
#include <DHT_U.h>                // DHT22 Sensor Library
#include <OneButton.h>            // Library to facilitate button usage

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

IPAddress ip;

WiFiUDP ntpUDP;             // Define UDP client

int GMTOffset = 3600;           // Offset in seconds to adjust for the timezone
int NTPUpdateInterval = 60000;  // Update interval in milliseconds of the NTP client
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GMTOffset, NTPUpdateInterval);   // Define NTP client

// Variables to store the current time received by the NTP Server
int currentHour;
int currentMinute;
int currentSeconds;
String formattedTime;
int currentDay;
int currentMonth;
int currentYear;

// Declare Shift Registers
int dataPin = D5;
int clockPin = D7;
int latchPin = D6;
ShiftRegister74HC595<2> sr(dataPin, clockPin, latchPin);

// Array used to store the Shift Registers output pins connected to each BCD to Decimal converter
int digitPins [4][4] {
  {3, 2, 1, 0},
  {7, 6, 5, 4},
  {11, 10, 9, 8},
  {15, 14, 13, 12}
};

// Array containing the states we need to set the Shift Registers outpunt pins to display a certain number on the single tube
// This is based on the truth table of the BCD to Decimal converter (K155ID1 or 74141)
// The last (11th) output states is an "invalid" code that should turn off all the outputs of the BCD to decimal converter
// It is used primarly in nightMode to turn off all the cathodes of the tubes

int digitOut [11][4] {
  {0, 0, 0, 0},
  {0, 0, 0, 1},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 1, 0, 0},
  {0, 1, 0, 1},
  {0, 1, 1, 0},
  {0, 1, 1, 1},
  {1, 0, 0, 0},
  {1, 0, 0, 1},
  {1, 1, 1, 1}
};

#define DHTPIN D3                     // Pin connected to the DHT sensor
#define DHTTYPE DHT22                 // DHT sensor type
DHT_Unified dht(DHTPIN, DHTTYPE);     // Define DHT Sensor

// Variables used to store Temperature and Humidity values
int tempValue;
int humValue;

// Define a pushbutton connected to the D8 pin
#define PBPIN D8
OneButton pb(
  PBPIN,  // Pin
  false,  // Input is HIGH when the button is pressed
  false   // Disable internal pull-up resistor
);

/* Variable to store the display mode of the clock, the possible values are:

    Mode 1 = timeMode (default mode)
    Mode 2 = dateMode
    Mode 3 = DHTMode
    Mode 4 = IPMode (shows at startup)
    Mode 5 = nightMode
    Mode 6 = cathodePoisoningPrevention

    The default mode at startup is Mode 4 to show the clock IP address, then after showing the 4 bits the clock switches to Mode 1 and stays there
    To switch mode thereafter you have to press the button on the back of the clock

*/

int clockMode = 4;

// Variables to define when to activate and deactivate nightMode
int nightModeHour = 23;
int nightModeMinute = 30;
int dayModeHour = 7;
int dayModeMinute = 0;

/* Variable to store the error state, the possible values are:

    Error 0 = no error
    Error 1 = NTP error
    Error 2 = DHT temp error
    Error 3 = DHT hum error

*/

int errorState = 0;

/* Variable to store the debug level select at startup, the possible values are:

    DL 0 = void setup() messages + errors (default)
    DL 1 = DL 0 + input of the updateTubes() function
    DL 2 = DL 1 + output of getTime() + getDHT() + push button events
    DL 3 = DL 2 + output of the updateTubes() function

*/

int debugLevel = 0;

// Variables needed to read values from console
char receivedChar;
bool newData = false;

// Timing variables used in conjuction with millis() to replace delay()
uint32_t currentMillis;

uint32_t prevGetDHT = 0;
uint32_t getDHTDelay;             // Minimum delay permitted between reading of the DHT22 sensor

uint32_t prevGetTime = 0;
const uint32_t getTimeDelay = 1000;         // Delay used to update the time information in the variables from the NTP client (This is not the delay between NTP requests, that is definied by the NTPUpdateInterval variable)

uint32_t prevTimeMode = 0;
uint32_t prevNightMode = 0;
const uint32_t timeModeDelay = 1000;        // Delay used in timeMode and in nightMode to update the clock digits only once a second

uint32_t prevCathPoison = 0;
const uint32_t cathPoisonDelay = 600000;      // Delays used to trigger cathode poisoning prevention (ratio 60s : 0.2s as described here https://docs.daliborfarny.com/nixie-tubes/1/en/topic/cathode-poisoning-prevention-routine)

// Variables used to read / write the two colons output
bool colon1 = 0;
bool colon2 = 1;

void DHTSetup();
void singleClick();
void doubleClick();
void longClick();
void serialReceive();
void webServer();
void getTime(bool test);
void getDHT(bool test);
void timeMode();
void dateMode();
void DHTMode();
void IPMode();
void nightMode();
void cathodePoisoningPrevention(bool test);
void updateTubes(int HH, int hh, int MM, int mm, int clockMode);
void updateColons(bool C1, bool C2);

void setup() {
  pinMode(D1, OUTPUT);    // Output to first colon transistor
  pinMode(D2, OUTPUT);    // Output to second colon transistor
  pinMode(D8, INPUT);     // Input from push button

  // Attach different push button click events to change clockMode
  pb.attachClick(singleClick);
  pb.attachDoubleClick(doubleClick);
  pb.attachLongPressStart(longClick);

  updateColons(colon1, colon2);   // Turn on only the colon2 to signal startup phase 1
  Serial.begin(9600);     // Inizialize Serial Communication
  delay(1000);            // Used to wait for Serial comm to begin

  Serial.println("To set the debug level send a number from 0 - 3 with no line-ending");      // ----------TO-DO---------- (not only in setup but you can send inputs even during the loop ideally)
  serialReceive();

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // Fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name here
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("Nixie");

  // if you get here you have connected to the WiFi
  Serial.println("Connected to WiFi");

  server.begin();

  updateColons(1, 0); // Turn on only the colon1 to signal startup phase 2
  // Save the IP address in a variable (actually a class that let you treat the address as an array of 4 int) and print it over serial
  Serial.print("The IP address of the clock is: ");
  ip = WiFi.localIP();
  Serial.println(ip);

  delay(500);
  
  DHTSetup();   // Initialize DHT sensor
  getDHT(1);    // Get TH info and print to console
  delay(500);

  timeClient.begin();     // Initialize NTPClient to get time
  getTime(1);    // Get time info and print to console
  delay(500);

  cathodePoisoningPrevention(1);     // Everytime the clock start we test every digit using the cathode poisoning prevention routine and telling the function it is a test (1)

  Serial.println("Showing IP Address on the clock");
  IPMode();     // Show IP address on clock at the start-up
  Serial.println("Startup done, initiating main loop...");
}

void DHTSetup() {
  dht.begin();            // Inizialize DHT22 Sensor
  // Set delay between DHT22 sensor readings based on sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  getDHTDelay = sensor.min_delay / 1000;
  Serial.print("getDHTDelay: ");
  Serial.println(getDHTDelay);
}

void singleClick() {
  clockMode = 2;
  if (debugLevel > 1) {
    Serial.println("Single click detected");
  }
}

void doubleClick() {
  clockMode = 3;
  if (debugLevel > 1) {
    Serial.println("Double click detected");
  }
}

void longClick() {
  clockMode = 4;
  if (debugLevel > 1) {
    Serial.println("Long click detected");
  }
}

void loop() {
  // Make webServer available
  webServer();
  
  // Keep watching for serial packets
  serialReceive();

  // Keep watching the push button:
  pb.tick();

  // Save current millis
  currentMillis = millis();

  // Update data routines
  getTime(0);
  getDHT(0);

  // Call the cathodePoisoningPrevention function only if cathPoisonDelay has elapsed since the last time it has run
  // The output 0 to the function is to tell that this is not a test so we can output the correct values to serial
  if (currentMillis - prevCathPoison >= cathPoisonDelay && clockMode == 1) {
    prevCathPoison = currentMillis;
    cathodePoisoningPrevention(0);
  }

  // Determines if nightMode needs to be active
  if ((currentHour == nightModeHour) && (currentMinute == nightModeMinute)) {
    clockMode = 5;
  }
  else if ((currentHour == dayModeHour) && (currentMinute == dayModeMinute)) {
    clockMode = 1;
    updateColons(1, 0);
  }

  // clockMode Switch Routine
  switch (clockMode) {
    case 1:
      timeMode();
      break;
    case 2:
      dateMode();
      break;
    case 3:
      DHTMode();
      break;
    case 4:
      IPMode();
      break;
    case 5:
      nightMode();
      break;
  }
}

void serialReceive() {
  int receivedInt = 0;

  if (Serial.available() > 0) {
    receivedChar = Serial.read();
    receivedInt = receivedChar - '0';
    newData = true;
  }

  if (newData == true) {
    if (receivedInt > 3) {
      Serial.println("Invalid Value! --- The debug level must be set between 0 and 3");
      newData = false;
    }
    else {
      Serial.print("Debug level set to: ");
      Serial.println(receivedInt);
      debugLevel = receivedInt;
      newData = false;
    }
  }
}

void webServer() {
   WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    if (debugLevel > 0) {
      Serial.println("New Client.");          // print a message out in the serial port
    }
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        if (debugLevel > 0) {
          Serial.write(c);                    // print it out the serial monitor
        }
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("th, td {border-bottom: 1px solid #ddd;}");
            client.println("table {margin: auto;}</style></head>");
          
            // Web Page Heading
            client.println("<body><h1>Nixie Web Server</h1>");

            client.println("<table align>");
            client.println("<tr><th>Ora:</th><td>" + formattedTime + "</td>");
            client.println("<tr><th>Data:</th><td>" + String(currentDay) + "/" + String(currentMonth) + "/" + String(currentYear) + "</td>");
            client.println("<tr><th>Temperatura:</th><td>" + String(tempValue) + " &deg;C</td>");
            client.println("<tr><th>Umidit&agrave;:</th><td>" + String(humValue) + " %</td></table>");
            
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    if (debugLevel > 0) {
      Serial.println("Client disconnected.");
      Serial.println("");
    }
  }
}

//Function to update the time and save the received values from the NTP server into the variables
void getTime(bool test) {
  // Delay between updates from the system time
  if (currentMillis - prevGetTime >= getTimeDelay || test == 1) {
    prevGetTime = currentMillis;
    timeClient.update();    // Update the time from the NTP Server

    // Get the UNIX epochTime (needed for conversions later)
    time_t epochTime = timeClient.getEpochTime();
    // Serial.print("Epoch Time: ");
    // Serial.println(epochTime);

    // Get a time structure (needed for conversions later)
    struct tm *ptm = gmtime ((time_t *)&epochTime);

    // Get the Hour value
    currentHour = timeClient.getHours();
    // Get the Minute value
    currentMinute = timeClient.getMinutes();
    // Get the Seconds value (can be viewed only on the webpage and in serial comms ideally)
    currentSeconds = timeClient.getSeconds();

    formattedTime = timeClient.getFormattedTime();

    // Get the current Day
    currentDay = ptm->tm_mday;
    // Get the current Month
    currentMonth = ptm->tm_mon + 1;
    // Get the current Year
    currentYear = ptm->tm_year + 1900;

    // Print to console
    if (debugLevel > 1 || test == 1) {
      Serial.print("Time: ");
      Serial.print(currentHour);
      Serial.print(":");
      Serial.print(currentMinute);
      Serial.print(":");
      Serial.println(currentSeconds);

      Serial.print("Day: ");
      Serial.print(currentDay);
      Serial.print("/");
      Serial.print(currentMonth);
      Serial.print("/");
      Serial.println(currentYear);
    }
  }
}

// Function to get the Temperature and Humidity Values from the DHT22 Sensor and save the values into the variables
void getDHT(bool test) {
  // Delay between measurements
  if (currentMillis - prevGetDHT >= getDHTDelay || test == 1) {
    prevGetDHT = currentMillis;
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      tempValue = event.temperature;
      if (debugLevel > 1 || test == 1) {
        Serial.print(F("Temperature: "));
        Serial.print(tempValue);
        Serial.println(F("°C"));
      }
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    }
    else {
      humValue = event.relative_humidity;
      if (debugLevel > 1 || test == 1) {
        Serial.print(F("Humidity: "));
        Serial.print(humValue);
        Serial.println(F("%"));
      }
    }
  }
}

// Default routine that shows the time on the clock
void timeMode() {
  // Divide the hour and minute values into single digits
  int HH = currentHour / 10 % 10;
  int hh = currentHour % 10;
  int MM = currentMinute / 10 % 10;
  int mm = currentMinute % 10;
  // Read and save the status of the colons output
  colon1 = digitalRead(D1);
  colon2 = digitalRead(D2);
  // Only update the tubes and colons once a second
  if (currentMillis - prevTimeMode >= timeModeDelay) {
    prevTimeMode = currentMillis;
    updateTubes(HH, hh, MM, mm, 1);
    updateColons(!colon1, !colon2);
  }
}

// Mode used to show the Day, Month and Year on the clock
// First by showing the day and the month on the two leftmost tubes and the month number on the other two tubes
// Then by showing the year on all four tubes
void dateMode() {
  // Divide the day and month values into single digits
  int HH_d = currentDay / 10 % 10;
  int hh_d = currentDay % 10;
  int MM_m = currentMonth / 10 % 10;
  int mm_m = currentMonth % 10;
  // Divide the year value into single digits
  int HH_y = currentYear / 1000 % 10;
  int hh_y = currentYear / 100 % 10;
  int MM_y = currentYear / 10 % 10;
  int mm_y = currentYear % 10;
  // Update tubes with day and month values
  updateTubes(HH_d, hh_d, MM_m, mm_m, 2);
  updateColons(1, 1);
  delay(3000);
  // Update tubes with year values
  updateTubes(HH_y, hh_y, MM_y, mm_y, 2);
  updateColons(0, 0);
  delay(3000);
  // Return to timeMode
  clockMode = 1;
  updateColons(1, 0);
}

// Mode used to show the actual temperature and humidity values (the two leftmost tubes show temperature while the others two the humidity)
void DHTMode() {
  // Divide the temperature and humidity values into single digits
  int HH = tempValue / 10 % 10;
  int hh = tempValue % 10;
  int MM = humValue / 10 % 10;
  int mm = humValue % 10;
  // // Update tubes with temperature and humidity values
  updateTubes(HH, hh, MM, mm, 3);
  updateColons(1, 1);
  // Return to timeMode
  delay(3000);
  clockMode = 1;
  updateColons(1, 0);
}

// Mode used to show the actual IP Address of the clock on the Tubes ("scrolling" through the 4 bits)
void IPMode() {
  // Set the single digit to an "invalid" value to turn off all the nixies digits
  int HH = 10;
  int hh = 10;
  int MM = 10;
  int mm = 10;

  updateColons(0, 0);
  // Divide the 4 bytes of the ip address into single digits (we need to check the lenght of each byte to prevent showing incorrect information)
  for (int i = 0; i < 4; i++) {
    if (ip[i] > 100) {
      hh = ip[i] / 100 % 10;
      MM = ip[i] / 10 % 10;
      mm = ip[i] % 10;
    }
    else if (ip[i] > 10) {
      hh = 10;
      MM = ip[i] / 10 % 10;
      mm = ip[i] % 10;
    }
    else {
      hh = 10;
      MM = 10;
      mm = ip[i] % 10;
    }
    // Update tubes with the correct values
    updateTubes(HH, hh, MM, mm, 4);
    delay(2000);
  }
  // Return to timeMode
  clockMode = 1;
  updateColons(1, 0);
}

// Mode to preserve life of the Nixie Tubes by switching them off
void nightMode() {
  // Only update the tubes and colons once a second
  if (currentMillis - prevNightMode >= timeModeDelay) {
    prevNightMode = currentMillis;
    // Write 10 to the tubes to turn them off
    updateTubes(10, 10, 10, 10, 5);
    updateColons(0, 0);
  }
}

// Function called every x amount of time to prevent the effects of cathode poisoning of the Nixie Tubes as described on the Dalibor Farny webpage
// We have an input test so we can distinguish between the setup() call to this function from the loop() one
void cathodePoisoningPrevention(bool test) {
  for (int i = 0; i < 10; i++) {
    if (debugLevel > 0 && test == 0) {
      Serial.print("Cathode poisoning prevention is running, digit: ");
      Serial.println(i);
    }
    else if (test == 1) {
      Serial.print("Initial test is running, digit: ");
      Serial.println(i);
    }
    // // Update tubes with i value
    updateTubes(i, i, i, i, 6);
    delay(2000);
  }
  updateColons(1, 0);
}

// Main routine called when the tubes needs to change values
// The asterisk (*) sign before the digitOut array is there to "dereference" the array (i think) but i don't really understood why (C shenanigans)
// However with an asterisk before an array the first value of the array is returned,
// so to get the needed values based on the input of the function you just add the HH, hh, MM and mm values to the digitOut array and then prepend an *
// You do the same with the for loop indexes by adding the result of the *(digitOut+HH) pointer to i and prepending an *
// In this way you get the correct values in the two dimensional array
void updateTubes(int HH, int hh, int MM, int mm, int clockMode) {
  if (debugLevel > 0) {
    Serial.print("clockMode: ");
    Serial.print(clockMode);
    Serial.print("   ---   updateTubes function input: ");
    Serial.print(HH);
    Serial.print(" --- ");
    Serial.print(hh);
    Serial.print(" --- ");
    Serial.print(MM);
    Serial.print(" --- ");
    Serial.println(mm);
  }
  for (int i = 0; i < 4; i++) {
    sr.set(digitPins[0][i], *(*(digitOut + HH) + i));
    if (debugLevel > 2) {
      Serial.print("1st nixie - SR output pin: ");
      Serial.print(digitPins[0][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[0][i]));
    }
  }
  for (int i = 0; i < 4; i++) {
    sr.set(digitPins[1][i], *(*(digitOut + hh) + i));
    if (debugLevel > 2) {
      Serial.print("2nd nixie - SR output pin: ");
      Serial.print(digitPins[1][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[1][i]));
    }
  }
  for (int i = 0; i < 4; i++) {
    sr.set(digitPins[2][i], *(*(digitOut + MM) + i));
    if (debugLevel > 2) {
      Serial.print("3rd nixie - SR output pin: ");
      Serial.print(digitPins[2][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[2][i]));
    }
  }
  for (int i = 0; i < 4; i++) {
    sr.set(digitPins[3][i], *(*(digitOut + mm) + i));
    if (debugLevel > 2) {
      Serial.print("4th nixie - SR output pin: ");
      Serial.print(digitPins[3][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[3][i]));
    }
  }
}

void updateColons(bool C1, bool C2) {
  digitalWrite(D1, C1);
  digitalWrite(D2, C2);
}