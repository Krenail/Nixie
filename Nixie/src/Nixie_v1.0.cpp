#include <Arduino.h>              // Basic Arduino framework library
#include <ShiftRegister74HC595.h> // Shift Registers Library
#include <ESP8266WiFi.h>          // Basic ESP WiFi Library
#include <DNSServer.h>            // DNS Server Library for AP Mode
#include <WiFiManager.h>          // WiFi Manager Library for easy AP and WiFi configuration
#include <NTPClient.h>            // NTP Client Library
#include <WiFiUdp.h>              // UDP Packets Library
#include <DHT.h>                  // DHT22 Sensor Library
#include <DHT_U.h>                // DHT22 Sensor Library
#include <OneButton.h>            // Library to facilitate button usage
#define WEBSERVER_H               // Needed to bugfix the simultaneous use of WiFiManager and ESPAsyncWebServer
#include <ESPAsyncTCP.h>          // Server Library for STA Mode
#include <ESPAsyncWebServer.h>    // Server Library for STA Mode
#include <Hash.h>                 // Used for retriving icons
#include <LittleFS.h>             // Used to save variables in a file

#include <TZ.h>                   // TimeZone library
#include <coredecls.h>            // settimeofday_cb()
#include <PolledTimeout.h>
#include <time.h>                 // time() ctime()
#include <sys/time.h>             // struct timeval

// Set web server port number to 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <title>Nixie Clock</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
    integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
      font-family: Helvetica;
      display: inline-block;
      margin: 0px auto;
      text-align: center;
    }

    button {
      font-family: Helvetica;
    }

    body {
      padding: 25px;
      background-color: black;
      color: white;
      font-size: 25px;
    }

    .dark-mode {
      background-color: black;
      color: white;
    }

    .light-mode {
      background-color: white;
      color: black;
    }

    h2 {
      font-size: 2.0rem;
    }

    p {
      font-size: 2.0rem;
    }

    .units {
      font-size: 1.5rem;
    }

    .dht-labels {
      font-size: 1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
    }

    .input-forms {
      font-family: Helvetica;
      font-size: 1rem;
      vertical-align: middle;
    }
  </style>
</head>

<body>
  <h2>Nixie Web Server</h2>
  <p>
    <button onclick="darkMode()">Dark Mode</button>
    <button onclick="lightMode()">Light Mode</button>
  </p>
  <p>
    <i class="fas fa-clock" style="color:#078d4a;"></i>
    <span class="dht-labels">Ora:</span>
    <span id="time">%TIME%</span>
  </p>
  <p>
    <i class="fas fa-calendar-alt" style="color:#fa9107;"></i>
    <span class="dht-labels">Data:</span>
    <span id="date">%DATE%</span>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#eb2124;"></i>
    <span class="dht-labels">Temperatura:</span>
    <span id="temperature">%TEMPERATURE%</span>
    <span class="units">&deg;C</span>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Umidit&agrave;:</span>
    <span id="humidity">%HUMIDITY%</span>
    <span class="units">&#37;</span>
  </p>
  <form action="/get" target="hidden-form">
    <p>
      <i class="fas fa-moon" style="color:#555555;"></i>
      <span class="dht-labels">Modalit&agrave; notte:</span>
      <span id="nightMode">%NIGHTMODE%</span>
      <br>
      <label class="dht-labels" for="nightMode">Modifica:</label>
      <input class="input-forms" type="time" name="nightMode">
      <input class="input-forms" type="submit" value="Salva" onclick="submitMessage()">
    </p>
  </form>
  </p>
  <form action="/get" target="hidden-form">
    <p>
      <i class="fas fa-sun" style="color:#fab005;"></i>
      <span class="dht-labels">Modalit&agrave; giorno:</span>
      <span id="dayMode">%DAYMODE%</span>
      <br>
      <label class="dht-labels" for="dayMode">Modifica:</label>
      <input class="input-forms" type="time" name="dayMode">
      <input class="input-forms" type="submit" value="Salva" onclick="submitMessage()">
    </p>
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body>

<script>
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
  }, 10000);

  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
  }, 10000);

  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("time").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/time", true);
    xhttp.send();
  }, 1000);

  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("date").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/date", true);
    xhttp.send();
  }, 10000);

  function darkMode() {
    var element = document.body;
    var content = document.getElementById("DarkModetext");
    element.className = "dark-mode";
    content.innerText = "Dark Mode is ON";
  }
  function lightMode() {
    var element = document.body;
    var content = document.getElementById("DarkModetext");
    element.className = "light-mode";
    content.innerText = "Dark Mode is OFF";
  }

  function submitMessage() {
    alert("Valore salvato!");
    setTimeout(function () { document.location.reload(false); }, 500);
  }
</script>

</html>
)rawliteral";

const char *PARAM_NIGHT_MODE = "nightMode";
const char *PARAM_DAY_MODE = "dayMode";

IPAddress ip; // Define IP Address Class

/*
WiFiUDP ntpUDP; // Define UDP client

int GMTOffset = 3600;                                                              // Offset in seconds to adjust for the timezone
int NTPUpdateInterval = 60000;                                                     // Update interval in milliseconds of the NTP client
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GMTOffset, NTPUpdateInterval); // Define NTP client
*/

// This line is necessary, not sure what it does
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

// An object which can store a time
static time_t now;

// This uses the PolledTimeout library to allow an action to be performed every 20 seconds
static esp8266::polledTimeout::periodicMs getTimeNow(1000);

// define the correct timezone
#define MYTZ TZ_Europe_Rome

// Variables to store the current time received by the NTP Server
int currentHour;
int currentMinute;
int currentSeconds;
String formattedTime;
int currentDay;
int currentMonth;
int currentYear;
String formattedDate;

// Time Strings to beautify the webpage
String currentHourStr;
String currentMinuteStr;
String currentSecondsStr;
String currentDayStr;
String currentMonthStr;
String currentYearStr;

// Declare Shift Registers
int dataPin = D5;
int clockPin = D7;
int latchPin = D6;
ShiftRegister74HC595<2> sr(dataPin, clockPin, latchPin);

// Array used to store the Shift Registers output pins connected to each BCD to Decimal converter
int digitPins[4][4]{
    {3, 2, 1, 0},
    {7, 6, 5, 4},
    {11, 10, 9, 8},
    {15, 14, 13, 12}};

// Array containing the states we need to set the Shift Registers outpunt pins to display a certain number on the single tube
// This is based on the truth table of the BCD to Decimal converter (K155ID1 or 74141)
// The last (11th) output states is an "invalid" code that should turn off all the outputs of the BCD to decimal converter
// It is used primarly in nightMode to turn off all the cathodes of the tubes

int digitOut[11][4]{
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
    {1, 1, 1, 1}};

#define DHTPIN D3                 // Pin connected to the DHT sensor
#define DHTTYPE DHT22             // DHT sensor type
DHT_Unified dht(DHTPIN, DHTTYPE); // Define DHT Sensor

// Variables used to store Temperature and Humidity values
int tempValue;
int humValue;

// Define a pushbutton connected to the D8 pin
#define PBPIN D8
OneButton pb(
    PBPIN, // Pin
    false, // Input is HIGH when the button is pressed
    false  // Disable internal pull-up resistor
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
int nightModeHour;
int nightModeMinute;
int dayModeHour;
int dayModeMinute;

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
uint32_t getDHTDelay; // Minimum delay permitted between reading of the DHT22 sensor

uint32_t prevTimeMode = 0;
uint32_t prevNightMode = 0;
const uint32_t timeModeDelay = 1000; // Delay used in timeMode and in nightMode to update the clock digits only once a second

uint32_t prevCathPoison = 0;
const uint32_t cathPoisonDelay = 600000; // Delays used to trigger cathode poisoning prevention (ratio 60s : 0.2s as described here https://docs.daliborfarny.com/nixie-tubes/1/en/topic/cathode-poisoning-prevention-routine)

// Variables used to read / write the two colons output
bool colon1 = 0;
bool colon2 = 1;

// Declare functions for Platform.io compatibility
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
String webServerProcessor(const String &var);
void notFound(AsyncWebServerRequest *request);
String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void webServerSetup();

void setup()
{
  pinMode(D1, OUTPUT); // Output to first colon transistor
  pinMode(D2, OUTPUT); // Output to second colon transistor
  pinMode(D8, INPUT);  // Input from push button

  // Inizialize LittleFS
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Attach different push button click events to change clockMode
  pb.attachClick(singleClick);
  pb.attachDoubleClick(doubleClick);
  pb.attachLongPressStart(longClick);

  updateColons(colon1, colon2); // Turn on only the colon2 to signal startup phase 1
  Serial.begin(9600);           // Inizialize Serial Communication
  delay(1000);                  // Used to wait for Serial comm to begin

  Serial.println("To set the debug level send a number from 0 - 3 with no line-ending"); // ----------TO-DO---------- (not only in setup but you can send inputs even during the loop ideally)
  serialReceive();

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  // wifiManager.resetSettings();

  // set custom ip for portal
  // wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // Fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name here
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("Nixie");

  // if you get here you have connected to the WiFi
  Serial.println("Connected to WiFi");

  webServerSetup();

  updateColons(1, 0); // Turn on only the colon1 to signal startup phase 2
  // Save the IP address in a variable (actually a class that let you treat the address as an array of 4 int) and print it over serial
  Serial.print("The IP address of the clock is: ");
  ip = WiFi.localIP();
  Serial.println(ip);

  delay(500);

  DHTSetup(); // Initialize DHT sensor
  getDHT(1);  // Get TH info and print to console
  delay(500);

  // This is where your time zone is set
  configTime(MYTZ, "pool.ntp.org");
  getTime(1);         // Get time info and print to console
  delay(500);

  cathodePoisoningPrevention(1); // Everytime the clock start we test every digit using the cathode poisoning prevention routine and telling the function it is a test (1)

  Serial.println("Showing IP Address on the clock");
  IPMode(); // Show IP address on clock at the start-up

  String nightModeTime = readFile(LittleFS, "/nightMode.txt");
  String dayModeTime = readFile(LittleFS, "/dayMode.txt");
  nightModeHour = ((nightModeTime[0] - '0') * 10) + (nightModeTime[1] - '0');
  nightModeMinute = ((nightModeTime[3] - '0') * 10) + (nightModeTime[4] - '0');
  dayModeHour = ((dayModeTime[0] - '0') * 10) + (dayModeTime[1] - '0');
  dayModeMinute = ((dayModeTime[3] - '0') * 10) + (dayModeTime[4] - '0');

  Serial.println("Startup done, initiating main loop...");
}

void loop()
{
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
  if (currentMillis - prevCathPoison >= cathPoisonDelay)
  {
    prevCathPoison = currentMillis;
    cathodePoisoningPrevention(0);
  }

  // Determines if nightMode needs to be active
  if ((currentHour == nightModeHour) && (currentMinute == nightModeMinute))
  {
    clockMode = 5;
  }
  else if ((currentHour == dayModeHour) && (currentMinute == dayModeMinute))
  {
    clockMode = 1;
    updateColons(1, 0);
  }

  // clockMode Switch Routine
  switch (clockMode)
  {
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

String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- write failed");
  }
  file.close();
}

void DHTSetup()
{
  dht.begin(); // Inizialize DHT22 Sensor
  // Set delay between DHT22 sensor readings based on sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  getDHTDelay = sensor.min_delay / 1000;
  Serial.print("getDHTDelay: ");
  Serial.println(getDHTDelay);
}

void singleClick()
{
  clockMode = 2;
  if (debugLevel > 1)
  {
    Serial.println("Single click detected");
  }
}

void doubleClick()
{
  clockMode = 3;
  if (debugLevel > 1)
  {
    Serial.println("Double click detected");
  }
}

void longClick()
{
  clockMode = 4;
  if (debugLevel > 1)
  {
    Serial.println("Long click detected");
  }
}

void serialReceive()
{
  int receivedInt = 0;

  if (Serial.available() > 0)
  {
    receivedChar = Serial.read();
    receivedInt = receivedChar - '0';
    newData = true;
  }

  if (newData == true)
  {
    if (receivedInt > 3)
    {
      Serial.println("Invalid Value! --- The debug level must be set between 0 and 3");
      newData = false;
    }
    else
    {
      Serial.print("Debug level set to: ");
      Serial.println(receivedInt);
      debugLevel = receivedInt;
      newData = false;
    }
  }
}

void webServerSetup()
{
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, webServerProcessor); });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(tempValue).c_str()); });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(humValue).c_str()); });
  server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(formattedTime).c_str()); });
  server.on("/date", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(formattedDate).c_str()); });
  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_NIGHT_MODE)) {
      inputMessage = request->getParam(PARAM_NIGHT_MODE)->value();
      writeFile(LittleFS, "/nightMode.txt", inputMessage.c_str());
      nightModeHour = ((inputMessage[0] - '0') * 10) + (inputMessage[1] - '0');
      nightModeMinute = ((inputMessage[3] - '0') * 10) + (inputMessage[4] - '0');
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_DAY_MODE)) {
      inputMessage = request->getParam(PARAM_DAY_MODE)->value();
      writeFile(LittleFS, "/dayMode.txt", inputMessage.c_str());
      dayModeHour = ((inputMessage[0] - '0') * 10) + (inputMessage[1] - '0');
      dayModeMinute = ((inputMessage[3] - '0') * 10) + (inputMessage[4] - '0');
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(204); });
  server.onNotFound(notFound);
  // Start web server
  server.begin();
}

String webServerProcessor(const String &var)
{
  if (var == "TEMPERATURE")
  {
    return String(tempValue);
  }
  else if (var == "HUMIDITY")
  {
    return String(humValue);
  }
  else if (var == "TIME")
  {
    return String(formattedTime);
  }
  else if (var == "DATE")
  {
    return String(formattedDate);
  }
  else if (var == "NIGHTMODE")
  {
    return readFile(LittleFS, "/nightMode.txt");
  }
  else if (var == "DAYMODE")
  {
    return readFile(LittleFS, "/dayMode.txt");
  }
  return String();
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

// Function to update the time and save the received values from the NTP server into the variables
void getTime(bool test)
{
  // Updates the 'now' variable to the current time value
  now = time(nullptr);

  // Delay between updates from the system time
  if (getTimeNow == 1 || test == 1)
  {
    // Get the Hour value
    currentHour = localtime(&now)->tm_hour;
    if (currentHour < 10) {
      currentHourStr = '0' + String(currentHour);
    }
    else {
      currentHourStr = String(currentHour);
    }

    // Get the Minute value
    currentMinute = localtime(&now)->tm_min;
    if (currentMinute < 10) {
      currentMinuteStr = '0' + String(currentMinute);
    }
    else {
      currentMinuteStr = String(currentMinute);
    }
    
    // Get the Seconds value (can be viewed only in serial comms)
    currentSeconds = localtime(&now)->tm_sec;
    if (currentSeconds < 10) {
      currentSecondsStr = '0' + String(currentSeconds);
    }
    else {
      currentSecondsStr = String(currentSeconds);
    }
    
    // Get the formatted time for the web page
    
    formattedTime = currentHourStr + ':' + currentMinuteStr + ':' + currentSecondsStr;

    // Get the current Day
    currentDay = localtime(&now)->tm_mday;
    if (currentDay < 10) {
      currentDayStr = '0' + String(currentDay);
    }
    else {
      currentDayStr = String(currentDay);
    }
    
    // Get the current Month
    currentMonth = localtime(&now)->tm_mon + 1;
    if (currentMonth < 10) {
      currentMonthStr = '0' + String(currentMonth);
    }
    else {
      currentMonthStr = String(currentMonth);
    }
    
    // Get the current Year
    currentYear = localtime(&now)->tm_year + 1900;
    currentYearStr = String(currentYear);

    // Get the formatted date for the web page
    formattedDate = currentDayStr + '/' + currentMonthStr + '/' + currentYearStr;

    // Print to console
    if (debugLevel > 1 || test == 1)
    {
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
void getDHT(bool test)
{
  // Delay between measurements
  if (currentMillis - prevGetDHT >= getDHTDelay || test == 1)
  {
    prevGetDHT = currentMillis;
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
      Serial.println(F("Error reading temperature!"));
    }
    else
    {
      tempValue = event.temperature;
      if (debugLevel > 1 || test == 1)
      {
        Serial.print(F("Temperature: "));
        Serial.print(tempValue);
        Serial.println(F("°C"));
      }
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
      Serial.println(F("Error reading humidity!"));
    }
    else
    {
      humValue = event.relative_humidity;
      if (debugLevel > 1 || test == 1)
      {
        Serial.print(F("Humidity: "));
        Serial.print(humValue);
        Serial.println(F("%"));
      }
    }
  }
}

// Default routine that shows the time on the clock
void timeMode()
{
  // Divide the hour and minute values into single digits
  int HH = currentHour / 10 % 10;
  int hh = currentHour % 10;
  int MM = currentMinute / 10 % 10;
  int mm = currentMinute % 10;
  // Read and save the status of the colons output
  colon1 = digitalRead(D1);
  colon2 = digitalRead(D2);
  // Only update the tubes and colons once a second
  if (currentMillis - prevTimeMode >= timeModeDelay)
  {
    prevTimeMode = currentMillis;
    updateTubes(HH, hh, MM, mm, 1);
    updateColons(!colon1, !colon2);
  }
}

// Mode used to show the Day, Month and Year on the clock
// First by showing the day and the month on the two leftmost tubes and the month number on the other two tubes
// Then by showing the year on all four tubes
void dateMode()
{
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
void DHTMode()
{
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
void IPMode()
{
  // Set the single digit to an "invalid" value to turn off all the nixies digits
  int HH = 10;
  int hh = 10;
  int MM = 10;
  int mm = 10;

  updateColons(0, 0);
  // Divide the 4 bytes of the ip address into single digits (we need to check the lenght of each byte to prevent showing incorrect information)
  for (int i = 0; i < 4; i++)
  {
    if (ip[i] > 99)
    {
      hh = ip[i] / 100 % 10;
      MM = ip[i] / 10 % 10;
      mm = ip[i] % 10;
    }
    else if (ip[i] > 9)
    {
      hh = 10;
      MM = ip[i] / 10 % 10;
      mm = ip[i] % 10;
    }
    else
    {
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
void nightMode()
{
  // Only update the tubes and colons once a second
  if (currentMillis - prevNightMode >= timeModeDelay)
  {
    prevNightMode = currentMillis;
    // Write 10 to the tubes to turn them off
    updateTubes(10, 10, 10, 10, 5);
    updateColons(0, 0);
  }
}

// Function called every x amount of time to prevent the effects of cathode poisoning of the Nixie Tubes as described on the Dalibor Farny webpage
// We have an input test so we can distinguish between the setup() call to this function from the loop() one
void cathodePoisoningPrevention(bool test)
{
  for (int i = 0; i < 10; i++)
  {
    if (debugLevel > 0 && test == 0)
    {
      Serial.print("Cathode poisoning prevention is running, digit: ");
      Serial.println(i);
    }
    else if (test == 1)
    {
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
void updateTubes(int HH, int hh, int MM, int mm, int clockMode)
{
  if (debugLevel > 0)
  {
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
  for (int i = 0; i < 4; i++)
  {
    sr.set(digitPins[0][i], *(*(digitOut + HH) + i));
    if (debugLevel > 2)
    {
      Serial.print("1st nixie - SR output pin: ");
      Serial.print(digitPins[0][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[0][i]));
    }
  }
  for (int i = 0; i < 4; i++)
  {
    sr.set(digitPins[1][i], *(*(digitOut + hh) + i));
    if (debugLevel > 2)
    {
      Serial.print("2nd nixie - SR output pin: ");
      Serial.print(digitPins[1][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[1][i]));
    }
  }
  for (int i = 0; i < 4; i++)
  {
    sr.set(digitPins[2][i], *(*(digitOut + MM) + i));
    if (debugLevel > 2)
    {
      Serial.print("3rd nixie - SR output pin: ");
      Serial.print(digitPins[2][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[2][i]));
    }
  }
  for (int i = 0; i < 4; i++)
  {
    sr.set(digitPins[3][i], *(*(digitOut + mm) + i));
    if (debugLevel > 2)
    {
      Serial.print("4th nixie - SR output pin: ");
      Serial.print(digitPins[3][i]);
      Serial.print(" state: ");
      Serial.println(sr.get(digitPins[3][i]));
    }
  }
}

void updateColons(bool C1, bool C2)
{
  digitalWrite(D1, C1);
  digitalWrite(D2, C2);
}