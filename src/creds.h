
#ifndef CREDS_H
#define CREDS_H  // prevent multiple definitions on Arduino IDE.

// replace this with your home WiFi credentials
#define LOCAL_SSID "Home-w9zv_2.4G"
#define LOCAL_PASS "811tamms"

// Change the hostname, hostname can only contain letters, numbers, and hyphens.
const char* HOSTNAME = "FVP-RAS3";

// once  you are read to go live these settings are what you client will connect to
#define AP_SSID "TestWebSite"
#define AP_PASS "023456789"

#define WWW_USERNAME "Alfred"   // 11-apr-2024      added to support basic authentication
#define WWW_PASSWORD "ENewman"  // only defs in here, no allocations.

#define MY_RAS_HTTP_PORT 50010   // for now, adjust as desired, this seems to work.

// Define a structure for the WiFi APs
typedef struct {
  const char* ssid;
  const char* password;
} WiFiAP;

// Define the WiFi APs in an array of WiFiAP structures
WiFiAP apList[] = {
  // highest priority is first
  // {"Colin’s Wi-Fi Network",   "onthebeach5670"}, // next lower priority
  // {"Colin’s iPhone",          "01234567"}, // next lowest priority
  {"Verizon-MiFi8800L-B39A",  "39231b36"},  // jet direct card from zv's hanger
  {"Robert's iPhone",         "12345678"},
  {"Binford 2.4",             "00988900"},  // dads in ohio
  {"Home-w9zv_2.4G",          "811tamms"},  
  {"Max2.4",                  "00988900"},  // dads in ohio
  {"AP3",                     "password3"}, // mext lowest priority
  {"AP4",                     "password4"}  // lowest priority, add as many as you see fit.
};

const int numAPs = sizeof(apList) / sizeof(apList[0]);

#endif