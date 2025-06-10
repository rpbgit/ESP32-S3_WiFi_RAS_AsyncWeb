/*
  this example will show
  1. how to use and ESP 32 for reading pins
  2. building a web page for a client (web browser, smartphone, smartTV) to connect to
  3. sending data from the ESP to the client to update JUST changed data
  4. sending data from the web page (like a slider or button press) to the ESP to tell the ESP to do something

  If you are not familiar with HTML, CSS page styling, and javascript, be patient, these code platforms are
  not intuitive and syntax is very inconsitent between platforms

  I know of 4 ways to update a web page
  1. send the whole page--very slow updates, causes ugly page redraws and is what you see in most examples
  2. send XML data to the web page that will update just the changed data--fast updates but older method
  3. JSON strings which are similar to XML but newer method
  4. web sockets very very fast updates, but not sure all the library support is available for ESP's

  I use XML here...

  compile options
  1. esp32 dev module
  2. upload speed 921600
  3. cpu speed 240 mhz
  flash speed 80 mhz
  flash mode qio
  flash size 4mb
  partition scheme default


  NOTE if your ESP fails to program press the BOOT button during programm when the IDE is "looking for the ESP"

*/
#include <Arduino.h>
#include <string.h>
#include "RAS_Web.h"       // .h file that stores your html page code
#include <WiFi.h>           // standard library
#include <ESPAsyncWebServer.h> // Async web server library
#include "defs.h"
#include "creds.h"  // unique credentials file for sharing this code, unique network stuff here.

// a circular/ring buffer of strings that will also concatenate all strings into one.
#include "StrRingBuffer.h"

// here you post web pages to your homes intranet which will make page debugging easier
// as you just need to refresh the browser as opposed to reconnection to the web server
#define USE_INTRANET

// start your defines for pins for sensors, outputs etc.
#define PIN_LED 48 // On board LED

int gLongest_loop_time; // so that the hw handler can access it.

// forward ref function prototype
void SendWebsite(AsyncWebServerRequest *request);
void SendXML(AsyncWebServerRequest *request);
void PrintWifiStatus();

void R1Select_handler(AsyncWebServerRequest *request);
void R2Select_handler(AsyncWebServerRequest *request);
void R3Select_handler(AsyncWebServerRequest *request);
void R4Select_handler(AsyncWebServerRequest *request);
void Aux1_OnSelect_handler(AsyncWebServerRequest *request);
void Aux1_OffSelect_handler(AsyncWebServerRequest *request);
void Aux2_OnSelect_handler(AsyncWebServerRequest *request);
void Aux2_OffSelect_handler(AsyncWebServerRequest *request);
void All_Grounded_handler(AsyncWebServerRequest *request);

void WebText(const char *format, ...);
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

void startupWiFiWithList();
void showWiFiNetworksFound(int numNetworks);

// the XML array size needs to be bigger than your maximum expected size.
char XML_buf[4096];

// just some buffer holder for char operations
char buf[512]  = {0};

// circular/ring buffer of fixed length strings to hold INFO message before being built into an XML msg
StringRingBuffer RingBuffer;

// variable for the IP reported when you connect to your homes intranet (during debug mode)
IPAddress Actual_IP;

// definitions of your desired intranet created by the ESP32
IPAddress PageIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress ip;

// gotta create a server
AsyncWebServer server(MY_RAS_HTTP_PORT);  // use 50010 an "unknown" port for use on the public WAN.

// this is the command sent by EITHER the VB console -OR- the webpage console
// its the only coupling needed between the WiFi/Web page stuff and the hardware.
HostCmdEnum gHostCmd = HostCmdEnum::NO_OP; // initial value of 255/0xFF = no-op

// external references to tell the linker to look for these things in another object file.
extern void loop2(HostCmdEnum & command);
extern void setup2();
extern RAS_HW_STATUS_STRUCT RAS_Status;
extern RAS_HW_STATUS_STRUCT *pRas; 
extern void get_hardware_status(RAS_HW_STATUS_STRUCT& myhw );

void setup()
{
    // standard stuff here
    Serial.begin(115200);
    delay(3000); //give the serial monitor on the IDE a chance to come up before sending stuff up the chute...

    randomSeed(1234L);
    
    setup2(); // setup2() in Lightning_hw.cpp does all the hardware initialization

    // if your web page or XML are large, you may not get a call back from the web page
    // and the ESP will think something has locked up and reboot the ESP
    // not sure I like this feature, actually I kinda hate it
    // disable watch dog timer 0
    //  disableCore0WDT();

    // maybe disable watch dog timer 1 if needed
    //  disableCore1WDT();

    // if you have this #define USE_INTRANET,  you will connect to your home intranet, again makes debugging easier

    //WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    
#ifdef USE_INTRANET
    // just an update to progress
    Serial.println("starting Webserver/WiFi, each dot below is an attempt to connect, we wait 500ms between attempts");
    // 11-JAN-2025 rpb  try a list of AP's as defined in creds.h
    startupWiFiWithList();
    delay(250);
    Actual_IP = WiFi.localIP();
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    // WiFi.begin(LOCAL_SSID, LOCAL_PASS);
    // while (WiFi.status() != WL_CONNECTED) {
    //     delay(500);
    //     Serial.print(".");
    // }
    // Serial.println("");
    Actual_IP = WiFi.localIP();
#endif

    // if you don't have #define USE_INTRANET, here's where you will create and access point
    // an intranet with no internet connection. But Clients can connect to your intranet and see
    // the web page you are about to serve up
#ifndef USE_INTRANET
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(100);
    WiFi.softAPConfig(PageIP, gateway, subnet);
    delay(100);
    Actual_IP = WiFi.softAPIP();
    Serial.print("IP address: ");
    Serial.println(Actual_IP);
#endif
    PrintWifiStatus();

    // these calls will handle data coming back from your web page
    // this one is a page request, upon ESP getting / string the web page will be sent
 
    // upon typing the URL into the browser, the browser makes a request of the server that is null, which gets intercepted
    // by this "lambda" function to cause the basic authentication (which is built into the browser) dialog to pop-up.
    // until the authentication is successful, it spins in a circle constantly asking for authentication.
    //  once the authentication is successful, the function "SendWebsite()" is called, which downloads to the browser
    // the http/javascript webpage.
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (!request->authenticate(WWW_USERNAME, WWW_PASSWORD)) {  // see creds.h for definitions of uname/pwd
               // return request->requestAuthentication();  // keep spinning, looking for auth to be OK
            }   
            SendWebsite(request);
        }
    );

    // upon ESP getting /XML string, ESP will build and send the XML, this is how we refresh
    // just parts of the web page
    server.on("/xml", HTTP_GET, SendXML);

    // upon ESP getting various handler strings, ESP will execute the corresponding handler functions
    // same notion for the following .on calls
    
    // These lines register HTTP PUT handlers for specific URL endpoints.
    // When the ESP32 web server receives a PUT request at one of these endpoints,
    // it will call the corresponding handler function (e.g., R1Select_handler).
    // Each handler sets a command (gHostCmd) for the hardware logic and sends a simple HTTP 200 OK response.
    // This mechanism allows the web page or other clients to control hardware functions (like selecting radios or toggling outputs)
    // by making HTTP requests to these endpoints.
    server.on("/R1Select", HTTP_PUT, R1Select_handler);
    server.on("/R2Select", HTTP_PUT, R2Select_handler);
    server.on("/R3Select", HTTP_PUT, R3Select_handler);
    server.on("/R4Select", HTTP_PUT, R4Select_handler);

    server.on("/Aux1_OnSel", HTTP_PUT, Aux1_OnSelect_handler);
    server.on("/Aux1_OffSel", HTTP_PUT, Aux1_OffSelect_handler);

    server.on("/Aux2_OnSel", HTTP_PUT, Aux2_OnSelect_handler);
    server.on("/Aux2_OffSel", HTTP_PUT, Aux2_OffSelect_handler);

    server.on("/ALL_GND", HTTP_PUT, All_Grounded_handler);

    // finally begin the server
    server.begin();

    Serial.println(F("Out of setup()...")); 
    delay(250);
}

void loop()
{
    // these will get initialized every time thru the loop
    unsigned long this_time = 0;
    unsigned long loop_begin = millis(); 

//HostCmd = HostCmdEnum::NO_OP; // this will get initialized every time thru the loop

    // your main loop that measures, processes, runs code, etc.

    // all the hardware management is handled by loop2() in RAS_hw.ino/cpp
    loop2(gHostCmd);

    // keep track of the longest exec loop time
    this_time = millis() - loop_begin;
    if (this_time > gLongest_loop_time) {
        gLongest_loop_time = this_time;
        Serial.print(F("EXEC_LOOP MAX LATENCY MS: "));
        Serial.println(gLongest_loop_time, DEC);
    }
    delay(2); // give other threads a chance
}

// wifi event handlers.
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
  Actual_IP = WiFi.localIP();
}
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("WiFi lost connection. Reason: ");
  Serial.print(info.wifi_sta_disconnected.reason);
  Serial.println(" - Trying to Reconnect");
  WiFi.reconnect();
}
// functional message handlers 

// Radio selection handlers
void R1Select_handler(AsyncWebServerRequest *request) {
//    Serial.println(F("R1 Select Handler"));
    gHostCmd = HostCmdEnum::SELECT_RADIO_1;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void R2Select_handler(AsyncWebServerRequest *request) {
//    Serial.println(F("R2 Select Handler"));
    gHostCmd = HostCmdEnum::SELECT_RADIO_2;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void R3Select_handler(AsyncWebServerRequest *request) {
    Serial.println(F("R3 Select Handler"));
    gHostCmd = HostCmdEnum::SELECT_RADIO_3;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void R4Select_handler(AsyncWebServerRequest *request) {
    Serial.println(F("R4 Select Handler"));
    gHostCmd = HostCmdEnum::SELECT_RADIO_4;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void Aux1_OnSelect_handler(AsyncWebServerRequest *request) {
    //Serial.println(F("Aux1_OnSelect_handler"));
    gHostCmd = HostCmdEnum::AUX_1_ON;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void Aux1_OffSelect_handler(AsyncWebServerRequest *request) {
    //Serial.println(F("Aux1_OffSelect_handler"));
    gHostCmd = HostCmdEnum::AUX_1_OFF;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void Aux2_OnSelect_handler(AsyncWebServerRequest *request) {
    //Serial.println(F("Aux2_OnSelect_handler"));
    gHostCmd = HostCmdEnum::AUX_2_ON;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void Aux2_OffSelect_handler(AsyncWebServerRequest *request) {
    //Serial.println(F("Aux2_OffSelect_handler"));
    gHostCmd = HostCmdEnum::AUX_2_OFF;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

void All_Grounded_handler(AsyncWebServerRequest *request) {
    //Serial.println(F("All_Grounded_handler"));
    gHostCmd = HostCmdEnum::ALL_GROUNDED;
    // for now just send a 200 OK msg
    request->send(200, "text/plain", ""); // Send web page ack
}

// WiFi setup print out
void PrintWifiStatus()
{
    // print the SSID of the network you're attached to:
    Serial.print(F("SSID: "));
    Serial.println(WiFi.SSID());
  
    // print your boards IP address:
    Actual_IP = WiFi.localIP();
    Serial.print(F("IP Address: "));
    Serial.print(Actual_IP);

    Serial.printf("  Hostname: %s\n", WiFi.getHostname());
  
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print(F("signal strength (RSSI):"));
    Serial.print(rssi);
    Serial.println(F(" dBm"));
}

// code to send the main web page
// PAGE_MAIN is a large char defined in SuperMon.h
void SendWebsite(AsyncWebServerRequest *request)
{
    Serial.println("sending web page");
    // you may have to play with this value, big pages need more porcessing time, and hence
    // a longer timeout that 200 ms
    request->send(200, "text/html", PAGE_MAIN);  // This loads the webpage in RAS_Web.h as the web page to the browser.
}

// this function will get the current status of all hardware and create the XML message with status
// and send it back to the page
// below is an improved version of SendXML() that uses snprintf for better buffer management
void SendXML(AsyncWebServerRequest *request)
{
    // Collect the hardware status that reflects the current state of the buttons/LEDs
    get_hardware_status(RAS_Status);

    // Use a pointer to keep track of the buffer position for efficiency
    char *ptr = XML_buf;
    int remaining = sizeof(XML_buf);

    // Helper macro to append formatted strings safely
    #define APPEND(fmt, ...) \
        ptr += snprintf(ptr, remaining, fmt, ##__VA_ARGS__); \
        remaining = sizeof(XML_buf) - (ptr - XML_buf);

    // Start XML
    APPEND("<?xml version='1.0'?>\n<Data>\n");

    // Add version from RAS_Status or defs.h
    APPEND("<VER>%s</VER>\n", RAS_Status.pSoftwareVersion);

    // Build currently selected radios with 1 or 0
    APPEND("<R1>%d</R1>\n", pRas->R1_Status ? 1 : 0);
    APPEND("<R2>%d</R2>\n", pRas->R2_Status ? 1 : 0);
    APPEND("<R3>%d</R3>\n", pRas->R3_Status ? 1 : 0);
    APPEND("<R4>%d</R4>\n", pRas->R4_Status ? 1 : 0);

    // Send current status of A1 On/Off
    APPEND("<A1_On>%d</A1_On>\n", pRas->A1_On_Status ? 1 : 0);
    APPEND("<A1_Off>%d</A1_Off>\n", pRas->A1_Off_Status ? 1 : 0);

    // Send current status of A2 On/Off
    APPEND("<A2_On>%d</A2_On>\n", pRas->A2_On_Status ? 1 : 0);
    APPEND("<A2_Off>%d</A2_Off>\n", pRas->A2_Off_Status ? 1 : 0);

    // Send current status of Xmit Indicator and ALL_GND
    APPEND("<Xmit_Ind>%d</Xmit_Ind>\n", pRas->Xmit_Indicator ? 1 : 0);
    APPEND("<ALL_GND>%d</ALL_GND>\n", pRas->ALL_GND_Status ? 1 : 0);

    // Append INFO tag from RingBuffer if not empty
    if (!RingBuffer.isEmpty()) {
        APPEND("<INFO>");
        RingBuffer.concat_all(ptr);
        ptr += strlen(ptr);
        remaining = sizeof(XML_buf) - (ptr - XML_buf);
        APPEND("</INFO>\n");
    }
    RingBuffer.delete_all();  // Clear for next time

    // Close XML block
    APPEND("</Data>\n");

    // Print XML to serial monitor for debugging (limit to first 4 times)
    static int x = 0;
    if (x < 4) {
        Serial.println(XML_buf);
        x++;
    }

    // Send XML response
    request->send(200, "text/xml", XML_buf);

    #undef APPEND // avoid macro redefinition issues
}

// provides a means of sending something to the web page text box.
void WebText(const char *format, ...) {
    // Temporary buffer to hold the formatted string
    static char tempBuf[2048] = {0};
    // Initialize the variable argument list
    va_list args;
    va_start(args, format);

    // Format the input string with the provided arguments
    vsnprintf(tempBuf, sizeof(tempBuf), format, args);

    // End processing the variable arguments
    va_end(args);

    Serial.print(tempBuf);
    RingBuffer.put(tempBuf); // add it to the ring buffer, will ovewrite oldest string if not serviced, but safe
    return;
}

/*
    startup the wifi given a list of AP's, sorted in order of preference.  the array of AP's is in creds.h
    scan the wifi network, only attempt to connect to the AP's in the list.
    this will add some time to the startup, but will allow the ESP to connect to the best AP available.
    useful for a device that may be moved around and connected to different AP's, like iphone vs local wifi AP.
    11-Jan-2025 w9zv
*/
void startupWiFiWithList()
{
   // Initialize WiFi
    WiFi.setHostname(HOSTNAME); // NOTE: MUST be called BEFORE WiFi.Mode() is set else the default is used.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    delay(250);

    // Scan for available networks
    Serial.println("Scanning for WiFi networks... stby");
    int numNetworks = WiFi.scanNetworks();
    showWiFiNetworksFound(numNetworks);

    // Try to connect to each AP in the priority list
    for (int i = 0; i < numAPs; i++) {
        for (int j = 0; j < numNetworks; j++) {
            if (strcmp(WiFi.SSID(j).c_str(), apList[i].ssid) == 0) {
                Serial.printf("Trying to connect to: %s ", apList[i].ssid);
                WiFi.begin(apList[i].ssid, apList[i].password);

                // Wait for connection
                int timeout = 0;
                while (WiFi.status() != WL_CONNECTED && timeout < 40) {
                    Serial.print(".");
                    delay(500);
                    timeout++;
                }

                if (WiFi.status() == WL_CONNECTED) {
                    Serial.printf("\nConnected to %s\n", apList[i].ssid);
                    //Serial.printf("\nConnected !\n");
                    break; // stop trying to connect to other AP's found in the scan
                } else {
                    Serial.printf("\nFailed to connect to %s\n", apList[i].ssid);
                }
            }
        }
        if (WiFi.status() == WL_CONNECTED) {
            break; // stop trying to connect to other AP's on the preferred list
        }
    }
    // Delete the scan result to free memory 
    WiFi.scanDelete();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to any AP");
    }
}

void showWiFiNetworksFound(int numNetworks) {
    if (numNetworks == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(numNetworks);
        Serial.println(" networks found");
        Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
        for (int i = 0; i < numNetworks; ++i) {
            // Print SSID and RSSI for each network found
            Serial.printf("%2d", i + 1);
            Serial.print(" | ");
            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            Serial.print(" | ");
            Serial.printf("%4d", WiFi.RSSI(i));
            Serial.print(" | ");
            Serial.printf("%2d", WiFi.channel(i));
            Serial.print(" | ");

            switch (WiFi.encryptionType(i)) {
            case WIFI_AUTH_OPEN:
                Serial.print("open");
                break;
            case WIFI_AUTH_WEP:
                Serial.print("WEP");
                break;
            case WIFI_AUTH_WPA_PSK:
                Serial.print("WPA");
                break;
            case WIFI_AUTH_WPA2_PSK:
                Serial.print("WPA2");
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                Serial.print("WPA+WPA2");
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                Serial.print("WPA2-EAP");
                break;
            case WIFI_AUTH_WPA3_PSK:
                Serial.print("WPA3");
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                Serial.print("WPA2+WPA3");
                break;
            case WIFI_AUTH_WAPI_PSK:
                Serial.print("WAPI");
                break;
            default:
                Serial.print("unknown");
            }
            Serial.println("");
        } 
        Serial.println("END of Networks discovered");
    }
}
