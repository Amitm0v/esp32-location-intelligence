#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <SD.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <time.h>

// Wi-Fi Access Point Credentials
const char* ap_ssid = "GPS_LOGGER_by_Amit";
const char* ap_password = "amitkumar"; 

// Hardware Pins
#define RXD2 16
#define TXD2 17
#define SD_CS 5

HardwareSerial neogps(2);
TinyGPSPlus gps;

// Web Server and DNS Server Objects
AsyncWebServer server(80);
DNSServer dnsServer;

// Access Point IP configuration
IPAddress ap_local_IP(192, 168, 4, 1);
IPAddress ap_gateway(192, 168, 4, 1);
IPAddress ap_subnet(255, 255, 255, 0);

// Timezone Offset: UTC + 5:30 (India Standard Time)
const int hoursOffset = 5;
const int minsOffset = 30;

// Timing Control: 30-second GPS interval (30000 ms)
unsigned long lastLogTime = 0;
const unsigned long logInterval = 30000; 

// Function Prototypes
void logGPSToSD(String dateStr, String timeStr, double lat, double lng, double speed);
void getLocalTimeFromGPS(char* dateBuf, char* timeBuf);

// Inline Captive Portal HTML UI
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>GPS Logger - Amit</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; padding: 30px; background-color: #f4f4f9; }
    .card { background: white; padding: 25px; border-radius: 12px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 350px; margin: auto; }
    h2 { color: #333; margin-bottom: 8px; }
    p { color: #666; font-size: 14px; margin-bottom: 20px; }
    .btn { display: inline-block; background-color: #007bff; color: white; padding: 12px 24px; text-decoration: none; font-size: 16px; border-radius: 6px; font-weight: bold; border: none; cursor: pointer; }
    .btn:hover { background-color: #0056b3; }
  </style>
</head>
<body>
  <div class="card">
    <h2>GPS Logger Portal</h2>
    <p>Logged by <b>Amit</b></p>
    <a href="/download" class="btn">📥 Download GPS CSV Data</a>
  </div>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(1000);

  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
  } else {
    Serial.println("SD Card ready.");
  }

  // Configure Wi-Fi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet);
  
  Serial.println("\n-------------------------------------------");
  Serial.print("Access Point Started: ");
  Serial.println(ap_ssid);
  Serial.print("Portal IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("-------------------------------------------");

  // Start DNS Server to redirect all domains to 192.168.4.1 (Captive Portal)
  dnsServer.start(53, "*", ap_local_IP);

  // Serve Main Captive Portal Page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  // Captive Portal probes detection (Android / Windows / Apple captive portal URLs)
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_html); });
  server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_html); });
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_html); });

  // Direct CSV File Download Endpoint
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SD.exists("/gps_data.csv")) {
      request->send(SD, "/gps_data.csv", "text/csv", true); // true = force attachment download
    } else {
      request->send(404, "text/plain", "No gps_data.csv file found on SD Card!");
    }
  });

  // Catch-all: Redirect any request to the captive portal landing page
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  // Start Web Server
  server.begin();
  Serial.println("Captive Portal Server started.");

  lastLogTime = millis();
}

void loop() {
  // Process Captive Portal DNS redirection requests
  dnsServer.processNextRequest();

  // Feed GPS serial stream
  while (neogps.available() > 0) {
    gps.encode(neogps.read());
  }

  // Log GPS data to SD card every 30 seconds
  if (millis() - lastLogTime >= logInterval) {
    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
      
      char dateBuffer[12];
      char timeBuffer[10];
      getLocalTimeFromGPS(dateBuffer, timeBuffer);

      String dStr = String(dateBuffer);
      String tStr = String(timeBuffer);
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      double spd = gps.speed.kmph();

      // Log to local SD card in same CSV format
      logGPSToSD(dStr, tStr, lat, lng, spd);

      lastLogTime = millis();
    } else {
      Serial.println("Waiting for valid GPS fix with Date & Time...");
    }
  }
}

void getLocalTimeFromGPS(char* dateBuf, char* timeBuf) {
  struct tm t;
  t.tm_year = gps.date.year() - 1900;
  t.tm_mon  = gps.date.month() - 1;
  t.tm_mday = gps.date.day();
  t.tm_hour = gps.time.hour();
  t.tm_min  = gps.time.minute();
  t.tm_sec  = gps.time.second();
  t.tm_isdst = 0;

  time_t utcEpoch = mktime(&t);
  time_t localEpoch = utcEpoch + (hoursOffset * 3600) + (minsOffset * 60);

  struct tm *localTime = localtime(&localEpoch);
  
  sprintf(dateBuf, "%04d-%02d-%02d", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday);
  sprintf(timeBuf, "%02d:%02d:%02d", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
}

void logGPSToSD(String dateStr, String timeStr, double lat, double lng, double speed) {
  File dataFile = SD.open("/gps_data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(dateStr);
    dataFile.print(",");
    dataFile.print(timeStr);
    dataFile.print(",");
    dataFile.print(lat, 6);
    dataFile.print(",");
    dataFile.print(lng, 6);
    dataFile.print(",");
    dataFile.println(speed);
    dataFile.close();
    
    Serial.printf("Logged [%s %s] Lat: %.6f, Lng: %.6f, Speed: %.2f km/h to SD Card.\n", 
                  dateStr.c_str(), timeStr.c_str(), lat, lng, speed);
  } else {
    Serial.println("Failed to open gps_data.csv on SD Card.");
  }
}