
/* 

08-Jun-2025 w9zv    v1.0    initial commit of RAS Wifi Async project  (Original build in 2023)      
06-Jun-2025 WA9FVP	V2.0	Added Network Loss and reconnect logic.
08-Jul-2025 w9zv    v2.1    removed NANO condx compile, fixed to use WOKWI sim again, added gFlexOperationInProgressFlag so that turning the
                            astron off and other FSM timed Flex operations cannot be pre-empted until completed.  added && MAIN_PWR_IS_ON()  
                            to Aux2_On
23-Oct-2025 w9zv    v3.0    Added DebouncerClass for debounced button inputs and edge detection, in XMIT state handlers to detect lost relay ownership.

*/
#include <Arduino.h>
// #include <EEPROM.h>

#include "defs.h"

// define the version of the code which is displayed on TFT/Serial/and web page. This is the version of the code, not the hardware.
// pse update this whenver a new version of the code is released.
constexpr const char* CODE_VERSION_STR = "v3.0";  // a string for the application version number

//#define USING_WOKWI_SIMULATOR   // if on WOKWI simulator must be defined https://wokwi.com/projects/393426348150643713

// ESP32 pin defs here.
// Button Inputs
#define RadBtnKeyIn_1 4 // connects to front panel Button 1, wire or-ed with KeyIn from xmtr on PCB.
#define RadBtnKeyIn_2 5 // connects to front panel Button 2, wire or-ed with KeyIn from xmtr on PCB.
#define RadBtnKeyIn_3 6 // connects to front panel Button 3, wire or-ed with KeyIn from xmtr on PCB.
#define RadBtnKeyIn_4 7 // connects to front panel Button 4, wire or-ed with KeyIn from xmtr on PCB.
#define All_Grounded_In 10 // Disconnects the antennas and grounds the radio ports (Button5)

// Aux Input pins
#define Aux1_On 8 // Was Aux1on
#define Aux2_On 3 // Was Aux2on
#define Aux1_OFF 46 // Was Aux1off
#define Aux2_OFF 9 // Was Aux2off

// Amplifier and pulldown Output Keying pins
#define Amp_Key_Out1 11 // Keys an amplifier
#define Aux2_PullDown_FlexRMT_Out 12 // A pull down on a Rig with remote On/Off Power

// Transceiver PTT triggers the output pins.
#define Relay1_Out 15 // Was Relay1
#define Relay2_Out 16 // Was Relay2
#define Relay3_Out 1 // Was Relay3
#define Relay4_Out 2 // Was Relay4
#define Aux1_Out 13 // Was Aux1 out
#define Aux2_Out 14 // Was Aux2 out   //Turns on/off LED on control Box
#define Aux1_Off_LED 47 // Was Aux1 out
#define Aux2_Off_LED 21 // Was Aux2 out   //Turns on/off LED on control Box

#ifdef USING_WOKWI_SIMULATOR   // if using the WOKWI simulator...we have no web or main.cpp, so we need to fake out some of the externs.
int gLongest_loop_time; // this is a global in main.cpp, allow access here to respond to request for it
HostCmdEnum gHostCmd = HostCmdEnum::NO_OP;

void setup2(); // forward declaration of setup2() function
void loop2(HostCmdEnum& myHostCmd); // forward declaration of loop2() function

extern HostCmdEnum gHostCmd;
// fakeout extern void WebText(const char *); since we dont have the Webserver or main.cpp on a nano
//#define WebText(a)   // a hollow function, generates no code
#define WebText Serial.printf

void setup() {
    Serial.begin(115200); // open serial port at 115200 baud.
    delay(100);
    setup2();
}
void loop() {
    loop2(gHostCmd);
}

#else // NORMAL operation on a real ESP32
extern void WebText(const char *format, ...);
extern HostCmdEnum gHostCmd;// = HostCmdEnum::NO_OP;
extern int gLongest_loop_time; // this is a global in main.cpp, allow access here to respond to request for it
#endif // end of #ifdef USING_WOKWI_SIMULATOR


// declare the global structure and a pointer to it that is shared between the hardware handling done here
// and the web page message population and status updates.  
RAS_HW_STATUS_STRUCT RAS_Status;
RAS_HW_STATUS_STRUCT *pRas = &RAS_Status; // create a pointer to the RAS_STatus structure
void get_hardware_status(RAS_HW_STATUS_STRUCT& myhw );

// static file scoped state variable to indicate that a timed Flex operation is in progress
static bool gFlexOperationInProgressFlag = false;

//
// Here is an example of what you can do for readability if you wish to exploit the C++ PreProcessor's abilities to create macros
//  to use (which in this case is text substitution of code ) that is done prior to the actual compile.
#define MAIN_PWR_IS_ON() ( (digitalRead(Aux1_Out)) == HIGH )
#define MAIN_PWR_IS_OFF() ( (digitalRead(Aux1_Out)) == LOW )

// non-blocking delays
#ifdef USING_WOKWI_SIMULATOR  // if we are using the WOKWI simulator, realime is dialated by about 4x also reduce timing to test faster.
#define DELAY_DIVISOR 10
#else // normal ESP32 operation
#define DELAY_DIVISOR 1
#endif // end of #ifdef USING_WOKWI_SIMULATOR
const unsigned long REM_PWR_OFF_DELAY = 20000/DELAY_DIVISOR; // Wait for radio to Sleep Then send the "Radio is Sleeping" message"
const unsigned long BOOT_WAIT_DELAY = 43000/DELAY_DIVISOR; // Wait for radio to Wakeup then Send the "Radio Awake" message
const unsigned long MAINS_OFF_DELAY = 20000/DELAY_DIVISOR; // Wait for radio to Sleep Then send the "Radio is Sleeping" message"
const unsigned long MAINS_ON_DELAY = 0;

// debouncer Class for the button inputs, which also remaps/replaces the macros below to use the debounced inputs
// comment out this next line to disable the debouncer class and read pins directly.
#define HAVE_DEBOUNCER_CLASS   // signal that we have the debouncer class available

#ifndef HAVE_DEBOUNCER_CLASS  // if we dont have the debouncer class, define the macros here directly
// macros for when one and only one radio button or KeyIn line is selected... a LOW == selected
#define ONLY_RADIO_1_SELECTED_OR_KEYED() ( \
    (digitalRead(RadBtnKeyIn_1) == LOW) && \
    (digitalRead(RadBtnKeyIn_2) == HIGH) && \
    (digitalRead(RadBtnKeyIn_3) == HIGH) && \
    (digitalRead(RadBtnKeyIn_4) == HIGH) )

#define ONLY_RADIO_2_SELECTED_OR_KEYED() ( \
    (digitalRead(RadBtnKeyIn_1) == HIGH) && \
    (digitalRead(RadBtnKeyIn_2) == LOW) && \
    (digitalRead(RadBtnKeyIn_3) == HIGH) && \
    (digitalRead(RadBtnKeyIn_4) == HIGH) )

#define ONLY_RADIO_3_SELECTED_OR_KEYED() ( \
    (digitalRead(RadBtnKeyIn_1) == HIGH) && \
    (digitalRead(RadBtnKeyIn_2) == HIGH) && \
    (digitalRead(RadBtnKeyIn_3) == LOW) && \
    (digitalRead(RadBtnKeyIn_4) == HIGH) )

#define ONLY_RADIO_4_SELECTED_OR_KEYED() ( \
    (digitalRead(RadBtnKeyIn_1) == HIGH) && \
    (digitalRead(RadBtnKeyIn_2) == HIGH) && \
    (digitalRead(RadBtnKeyIn_3) == HIGH) && \
    (digitalRead(RadBtnKeyIn_4) == LOW) )

#define ANY_RADIO_SELECTED_OR_KEYED() ( \
    (digitalRead(RadBtnKeyIn_1) == LOW) || \
    (digitalRead(RadBtnKeyIn_2) == LOW) || \
    (digitalRead(RadBtnKeyIn_3) == LOW) || \
    (digitalRead(RadBtnKeyIn_4) == LOW) )

#define NO_RADIO_SELECTED_OR_KEYED() ( \
    (digitalRead(RadBtnKeyIn_1) == HIGH) && \
    (digitalRead(RadBtnKeyIn_2) == HIGH) && \
    (digitalRead(RadBtnKeyIn_3) == HIGH) && \
    (digitalRead(RadBtnKeyIn_4) == HIGH) )

#else  // we have the debouncer class available
// Include the debouncer class for reading and debouncing the input buttons
#define DEBOUNCER_DEFINED // gInputs is defined in the debouncer class header if this is defined
#include "DebouncerClass.h"   // note  - must be AFTER the pin definitions above
const auto& ss = gInputs.get(); // a globally available read only snapshot of the current debounced states
const auto& ee = gInputs.getEdges(); // a globally available read only snapshot of the edges detected on last update()
// Then act on ss.* (levels) or ee.* (edges) as needed.
#endif //ifndef HAVE_DEBOUNCER_CLASS
    
//Remove this macro:
// #define NO_RADIO_ANT_SELECTED() ( \
//     (digitalRead(Relay1_Out) == LOW) && \
//     (digitalRead(Relay2_Out) == LOW) && \
//     (digitalRead(Relay3_Out) == LOW) && \
//     (digitalRead(Relay4_Out) == LOW) )
// Inline replacement (file-scoped)
static inline bool noRadioAntSelected() {
    return digitalRead(Relay1_Out) == LOW &&
           digitalRead(Relay2_Out) == LOW &&
           digitalRead(Relay3_Out) == LOW &&
           digitalRead(Relay4_Out) == LOW;
}
#define RAS_GET_R1_RELAY_STATUS() ( (digitalRead(Relay1_Out)) )
#define RAS_GET_R2_RELAY_STATUS() ( (digitalRead(Relay2_Out)) )
#define RAS_GET_R3_RELAY_STATUS() ( (digitalRead(Relay3_Out)) )
#define RAS_GET_R4_RELAY_STATUS() ( (digitalRead(Relay4_Out)) )
#define RAS_GET_A1_OUT_STATUS()   ( (digitalRead(Aux1_Out)) )
#define RAS_GET_A2_OUT_STATUS()   ( (digitalRead(Aux2_Out)) )
#define RAS_GET_FlexRMT_Out()     ( (digitalRead(Aux2_PullDown_FlexRMT_Out)) )
#define RAS_GET_AMP_KEY_Out()     ( (digitalRead(Amp_Key_Out1)) )


// function declarations, needed to resolve forward references
// void display_freeram(); // useful if you think you are running out of SRAM or getting heap corruption...
// int freeRam();
void RadioOne_Handler(HostCmdEnum);
void RadioTwo_Handler(HostCmdEnum);
void RadioThree_Handler(HostCmdEnum);
void RadioFour_Handler(HostCmdEnum);
void Aux1_Handler(HostCmdEnum);
void Aux2_Handler(HostCmdEnum);
void All_Grounded_Handler(HostCmdEnum);
void ReportStatus();
void AllGrounded();
void displayState(String);
void SendMsgToConsole(const char *msg);  // sends to both web and serial console(s) if available.

void setup2()
{
    pRas->pSoftwareVersion = CODE_VERSION_STR; // set the code version string in the RAS_Status structure
    // Assign Input Pins
    pinMode(RadBtnKeyIn_1, INPUT_PULLUP);
    pinMode(RadBtnKeyIn_2, INPUT_PULLUP);
    pinMode(RadBtnKeyIn_3, INPUT_PULLUP);
    pinMode(RadBtnKeyIn_4, INPUT_PULLUP);
    pinMode(Aux1_On, INPUT_PULLUP);
    pinMode(Aux2_On, INPUT_PULLUP);
    pinMode(Aux1_OFF, INPUT_PULLUP);
    pinMode(Aux2_OFF, INPUT_PULLUP);
    pinMode(All_Grounded_In, INPUT_PULLUP);

    // Assign output pins
    pinMode(Relay1_Out, OUTPUT); // <<<< running this causes the ESP32 to crash !!!
    pinMode(Relay2_Out, OUTPUT);
    // return;
    pinMode(Relay3_Out, OUTPUT);
    pinMode(Relay4_Out, OUTPUT);
    pinMode(Amp_Key_Out1, OUTPUT);
    pinMode(Aux2_PullDown_FlexRMT_Out, OUTPUT);
    pinMode(Aux1_Out, OUTPUT);
    pinMode(Aux2_Out, OUTPUT);

    // initialize the pins
    digitalWrite(Relay1_Out, LOW);
    digitalWrite(Relay2_Out, LOW);
    digitalWrite(Relay3_Out, LOW);
    digitalWrite(Relay4_Out, LOW);
    digitalWrite(Aux1_Out, LOW);
    digitalWrite(Aux2_Out, LOW);
    digitalWrite(Amp_Key_Out1, LOW);
    digitalWrite(Aux2_PullDown_FlexRMT_Out, LOW);

    Serial.println(F("WiFi RAS "));
    Serial.print(F("Compiled on : "));
    Serial.println(__TIMESTAMP__);

    // dump a metric of how much unallocated SRAM is avail.
    // display_freeram();

    Serial.println(F("All Grounded, AUX1 & Aux2 Off"));

    Serial.println(F("Out of setup2()...")); delay(20);
}

void loop2(HostCmdEnum& myHostCmd)
{
    // this is the command sent by EITHER the VB console -OR- the webpage console
    // its the only coupling needed between the WiFi/Web page stuff and the hardware.
    // HostCmdEnum myHostCmd = HostCmdEnum::NO_OP; // initial value of 255/0xFF = no-op

    // read the serial port, see if there is an incoming host command to handle. value is an integer cast to an enum
    if (Serial.available() > 0) {
        // myHostCmd = (HostCmdEnum)(Serial.read() - 48);  // read it convert to decimal
        myHostCmd = (HostCmdEnum)(Serial.read()); // read it convert to decimal, coerce into a HostCmdEnum type
        // Serial.print(F("CMD: ")); Serial.println((int)myHostCmd,DEC);
    }

    #ifdef HAVE_DEBOUNCER_CLASS
    // for the debouncer class, call update() once per loop to sample and debounce the inputs MANDATORY  
    gInputs.update();  // refresh debounced inputs, all access to debounced states is via the 'ss' snapshot structure

    bool xtest1 = ss.r1; // example of how to use the debounced inputs, if x is true, button 1 is pressed (active low).
    if( ss.r1 ) {
        // button 1 is pressed
    }   
    #endif // HAVE_DEBOUNCER_CLASS

    // handle any other commands or features not handled by hardware handlers.
    switch (myHostCmd) {
    case HostCmdEnum::STATUS_REQUEST:
        // report status to host
        ReportStatus();
        myHostCmd = HostCmdEnum::NO_OP;
        break;
    case HostCmdEnum::ALL_GROUNDED:
        AllGrounded();
        Serial.println(F("CON::Button5"));
        break;
    case HostCmdEnum::RQST_MAX_EXEC_LOOP_TIME:
        Serial.print(F("EXEC_LOOPMAX: ")),
            Serial.println(gLongest_loop_time, DEC);
        myHostCmd = HostCmdEnum::NO_OP;
        break;
    case HostCmdEnum::NO_OP:
        break; // no-op command, does nothing, but differentiate from unknown
    default:
        // handle any unknown host command...
        break;
    }

    // handle each button or radio's in their own handler, passing the host command to it
    RadioOne_Handler(myHostCmd);
    RadioTwo_Handler(myHostCmd);
    RadioThree_Handler(myHostCmd);
    RadioFour_Handler(myHostCmd);
    Aux1_Handler(myHostCmd);
    Aux2_Handler(myHostCmd);
    All_Grounded_Handler(myHostCmd);

    // let loop() fall through, it will get called again
    // myHostCmd will get re-initialized to a -1 no-op when loop() is called again.

    myHostCmd = HostCmdEnum::NO_OP;
}

void RadioOne_Handler(HostCmdEnum host_cmd)
{
    // Declare the possible states in meaningful English. Enums start enumerating
    // at zero, incrementing in steps of 1 unless overridden. We use an
    // enum 'class' here for type safety and code readability
    enum class RadioState : uint8_t {
        NOTHING, // unused/invalid, defaults to 0
        IDLE, // defaults to 1
        XMIT
    };
    // Keep track of the current State (it's a variable of a type RadioState)
    static RadioState currState = RadioState::IDLE; // must be a static declaration, as it persists across invocations.
    static bool do_once = false;

    // handle Radio 1 stuff & the incoming command if needed.
    switch (currState) {
    case RadioState::IDLE:
        if (ONLY_RADIO_1_SELECTED_OR_KEYED() && MAIN_PWR_IS_ON()) {
            if (do_once == false) {
                if (digitalRead(Relay1_Out) == LOW){
                    Serial.println(F("CON::Button1")); // send only if we havent yet selected the antenna
                    SendMsgToConsole(" Receivers to Antenna\n");
                } 
                AllGrounded();
                digitalWrite(Relay1_Out, HIGH);
                digitalWrite(Amp_Key_Out1, LOW);   // per FVP 10/27/2025changed to LOW from HIGH - when R1 becomes xmit capable, return to HIGH
                do_once = true;
            }
            currState = RadioState::XMIT;
        } else if (NO_RADIO_SELECTED_OR_KEYED() && host_cmd == HostCmdEnum::SELECT_RADIO_1 && MAIN_PWR_IS_ON()) {
            AllGrounded();
            digitalWrite(Relay1_Out, HIGH);
            Serial.println(F("CON::Button1"));
            SendMsgToConsole(" Receivers to Antenna\n");
        }
        break;

    case RadioState::XMIT: // this radio is currently transmitting, handle xmit negation
        displayState("\tR1XMIT state");
        // If another handler grounded or stole the relay, we lost ownership.
        if (digitalRead(Relay1_Out) == LOW) {
            do_once = false;
            displayState(F("\tR1IDLE state (lost relay)"));
            currState = RadioState::IDLE;
            break;
        }
        if (( !ss.r1 && digitalRead(Relay1_Out) == HIGH) && do_once == true) {
            //digitalWrite(Amp_Key_Out1, LOW); // no amp on this one yet.
            do_once = false;
            displayState("\tR1IDLE state");
            currState = RadioState::IDLE; // and back to idle...
        }
        break;

    default:
        break;
    }
}

// Radio 2 is currently the Flexradio
void RadioTwo_Handler(HostCmdEnum host_cmd)
{
    enum class RadioState : uint8_t {
        NOTHING, // unused, defaults to 0
        IDLE, // defaults to 1
        XMIT, // defaults to 2
        BOOT_WAIT, // defaults to 3
        REM_POWER_OFF_WAIT, // defaults to 4
    };

    // Keep track of the current State (it's a variable of a type RadioState)
    static RadioState currState = RadioState::IDLE; // must be a static declaration, so it persists across invocations.
    static bool do_once = false;
    
    static unsigned long timerStart = 0; // timer begin
    static unsigned long timerDelay = 0; // how long to wait.

    // handle radio one stuff & the incoming command if needed.
    switch (currState) {
    case RadioState::IDLE:
        // displayState("\tR2IDLE state");
        if (ONLY_RADIO_2_SELECTED_OR_KEYED() && MAIN_PWR_IS_ON()) {
            if (do_once == false) {
                if (digitalRead(Relay2_Out) == LOW){
                    Serial.println(F("CON::Button2")); // send only if we havent yet selected the antenna
                    WebText(" Flex-6600 to KPA500\n"); 
                }
                AllGrounded();
                digitalWrite(Relay2_Out, HIGH);
                digitalWrite(Amp_Key_Out1, HIGH);
                Serial.println(F("CON::LEDRed"));

                do_once = true;
            }
            currState = RadioState::XMIT;
            
            // if AUX2_On is pressed, and its not yet on -or- if a host command to select radio 2 occurs
        } else if ((((digitalRead(Aux2_On) == LOW && digitalRead(Aux2_Out) == LOW)) || host_cmd == HostCmdEnum::AUX_2_ON) && MAIN_PWR_IS_ON()) {
            gFlexOperationInProgressFlag = true; // set the global flag to indicate a Flex operation is in progress
            digitalWrite(Aux2_Out, HIGH);
            digitalWrite(Aux2_PullDown_FlexRMT_Out, HIGH);
            Serial.println(F("Radio Booting...")); // informatonal message
            timerStart = millis();
            timerDelay = BOOT_WAIT_DELAY;
            Serial.println(F("CON::Aux2 On"));
             WebText(" Flex-6600 is Booting\n");          // <--- Add your Text here, for example  "My Device\n"
             Serial.print(F("\tTimerDelay: "));
             Serial.println(timerDelay, DEC);
            currState = RadioState::BOOT_WAIT;

        } else if ((digitalRead(Aux2_OFF) == LOW && digitalRead(Aux2_Out) == HIGH) || host_cmd == HostCmdEnum::AUX_2_OFF) {
            /////////////////////  Flex Remote Off (Aux2 Off) Button Press //////////////////////////
            gFlexOperationInProgressFlag = true; // set the global flag to indicate a Flex operation is in progress
            digitalWrite(Aux2_Out, LOW);
            digitalWrite(Aux2_PullDown_FlexRMT_Out, LOW);
            timerStart = millis();
            timerDelay = REM_PWR_OFF_DELAY;
            Serial.println(F("CON::Aux2 Off"));
            Serial.print(F("\tTimerDelay: "));
            WebText(" Flex-6600 shutting Down\n");          // <--- Add your Text here, for example  "My Device\n"
            Serial.println(timerDelay, DEC);
            currState = RadioState::REM_POWER_OFF_WAIT;

        } else if (NO_RADIO_SELECTED_OR_KEYED() && host_cmd == HostCmdEnum::SELECT_RADIO_2 && MAIN_PWR_IS_ON()) { // or if selected from console 4-apr-24 added
            AllGrounded();
            digitalWrite(Relay2_Out, HIGH);
            Serial.println(F("CON::Button2"));
            WebText("Flex6600 To Antenna\n");          // <--- Add your Text here, for example  "My Device\n"
        }
        break;

    case RadioState::XMIT: // we are transmitting, handle xmit negation
        displayState(F("\tR2XMIT state"));
        // If another handler grounded or stole the relay, we lost ownership.
        if (digitalRead(Relay2_Out) == LOW) {
            do_once = false;
            displayState(F("\tR2IDLE state (lost relay)"));
            currState = RadioState::IDLE;
            break;
        }
        if ( ( !ss.r2 && digitalRead(Relay2_Out) == HIGH) && do_once == true) {
            digitalWrite(Amp_Key_Out1, LOW);
            Serial.println(F("CON::LEDBlack"));
            do_once = false;
            displayState(F("\tR2IDLE state"));
            currState = RadioState::IDLE; // and back to idle...
        }
        break;

    case RadioState::BOOT_WAIT:
        if (!do_once) {
            displayState(F("\tR2BOOT_WAIT state"));
            do_once = true;
        }
        // we are currently xmitting, if negated go back to idle
        // Has the timer expired? Ignore further Flex cmds until after boot wait expires
        if (millis() - timerStart >= timerDelay) {
            Serial.println(F("CON::Aux2 Ready On"));
            WebText(" Flex-6600 is On\n");          // <--- Add your Text here, for example  "My Device\n"
            // if (digitalRead(Aux2_On) == HIGH && digitalRead(Aux2_Out) == HIGH && Button == HIGH) {
            // AllGrounded();
            // digitalWrite(Relay2_Out, HIGH);
            // Serial.println(F("CON::LEDRed"));
            // Move to next state
            displayState(F("\tR2IDLE state"));
            do_once = false;
            currState = RadioState::IDLE;
            gFlexOperationInProgressFlag = false; // clear the global flag to indicate a Flex operation is no longer in progress
        }
        break;

    case RadioState::REM_POWER_OFF_WAIT:
        if (!do_once) {
            displayState(F("\tR2REM_POWER_OFF_WAIT state"));
            do_once = true;
        }
        if (millis() - timerStart >= timerDelay) { // time the delay, non-blocking
            Serial.println(F("CON::Aux2 Ready Off"));
            WebText(" Flex-6600 is Sleeping\n");          // <--- Add your Text here, for example  "My Device\n"
            do_once = false;
            displayState(F("\tR2IDLE state"));
            // Move back to idle...
            currState = RadioState::IDLE;
            gFlexOperationInProgressFlag = false; // clear the global flag to indicate a Flex operation is no longer in progress
        }
        break;

    default:
        Serial.println(F("'Default' Switch Case reached - Error"));
        break;
    }
}

//////////////////// Icom IC7300 Send & Front Panel Button Press, Xmit/recv, host command to select /////////////////
void RadioThree_Handler(HostCmdEnum host_cmd)
{
    enum class RadioState : uint8_t {
        NOTHING, // unused/invalid, defaults to 0
        IDLE, // defaults to 1
        XMIT // radio is in XMIT
    };
    // Keep track of the current State (it's a variable of a type RadioState)
    static RadioState currState = RadioState::IDLE; // must be a static declaration, as it persists across invocations.
    static bool do_once = false;

    // handle Radio 3 stuff & the incoming command if needed.
    switch (currState) {
    case RadioState::IDLE:
        // displayState("\tIDLE state");
        if (ONLY_RADIO_3_SELECTED_OR_KEYED() && MAIN_PWR_IS_ON()) {
            if (do_once == false) {
                if (digitalRead(Relay3_Out) == LOW) {
                    Serial.println(F("CON::Button3")); // send only if we havent yet selected the antenna
                    SendMsgToConsole("IC-7300 To Antenna");
                }
                AllGrounded();
                digitalWrite(Relay3_Out, HIGH); // HIGH == antenna 3 selected.
                digitalWrite(Amp_Key_Out1, HIGH);
                Serial.println(F("CON::LEDRed"));
                // Serial.println(F("CON::Button3"));
                do_once = true;
            }
            currState = RadioState::XMIT;
        } else if (NO_RADIO_SELECTED_OR_KEYED() && host_cmd == HostCmdEnum::SELECT_RADIO_3 && MAIN_PWR_IS_ON()) {
            AllGrounded();
            digitalWrite(Relay3_Out, HIGH);
            Serial.println(F("CON::Button3"));
            SendMsgToConsole("IC-7300 To Antenna");
        }
        break;

    case RadioState::XMIT: // we are transmitting, handle xmit negation
        displayState("\tR3XMIT state");
        // If another handler grounded or stole the relay, we lost ownership.
        if (digitalRead(Relay3_Out) == LOW) {
            do_once = false;
            displayState(F("\tR3IDLE state (lost relay)"));
            currState = RadioState::IDLE;
            break;
        }
        if ( ( !ss.r3 && digitalRead(Relay3_Out) == HIGH) && do_once == true) {
            digitalWrite(Amp_Key_Out1, LOW);
            Serial.println(F("CON::LEDBlack"));
            do_once = false;
            displayState(F("\tR3IDLE state"));
            currState = RadioState::IDLE; // and back to idle...
        }
        break;

    default:
        break;
    }
}

void RadioFour_Handler(HostCmdEnum host_cmd)
{
    enum class RadioState : uint8_t {
        NOTHING, // unused/invalid, defaults to 0
        IDLE, // defaults to 1
        XMIT // radio is in XMIT
    };
    // Keep track of the current State (it's a variable of a type RadioState)
    static RadioState currState = RadioState::IDLE; // must be a static declaration, as it persists across invocations.
    static bool do_once = false;

    // handle Radio 4 stuff & the incoming command if needed.
    switch (currState) {
    case RadioState::IDLE:
        // displayState("\tR4IDLE state");
        if (ONLY_RADIO_4_SELECTED_OR_KEYED() && MAIN_PWR_IS_ON()) {
            if (do_once == false) {
                if (digitalRead(Relay4_Out) == LOW) {
                    Serial.println(F("CON::Button4")); // send only if we havent yet selected the antenna
                    SendMsgToConsole("IC-705 To Antenna"); //<--- Add your radio here, fpr example "My Radio\n"
                }
                AllGrounded();
                digitalWrite(Relay4_Out, HIGH);
                digitalWrite(Amp_Key_Out1, HIGH);
                Serial.println(F("CON::LEDRed"));
                do_once = true;
            }
            currState = RadioState::XMIT;
        } else if (NO_RADIO_SELECTED_OR_KEYED() && host_cmd == HostCmdEnum::SELECT_RADIO_4 && MAIN_PWR_IS_ON()) {
            AllGrounded();
            digitalWrite(Relay4_Out, HIGH);
            Serial.println(F("CON::Button4"));
            SendMsgToConsole("IC-705 To Antenna"); //<--- Add your radio here, fpr example "My Radio\n"
        }
        break;

    case RadioState::XMIT: // we are transmitting, handle xmit negation
        displayState("\tR4XMIT state");
        // If another handler grounded or stole the relay, we lost ownership.
        if (digitalRead(Relay4_Out) == LOW) {
            do_once = false;
            displayState(F("\tR4IDLE state (lost relay)"));
            currState = RadioState::IDLE;
            break;
        }
        if ( ( !ss.r4 && digitalRead(Relay4_Out) == HIGH) && do_once == true) {
            digitalWrite(Amp_Key_Out1, LOW);
            Serial.println(F("CON::LEDBlack"));
            do_once = false;
            displayState(F("\tR4IDLE state"));
            currState = RadioState::IDLE; // and back to idle...
        }
        break;

    default:
        break;
    }
}

////////////////////   Main Power  ////////////////
void Aux1_Handler(HostCmdEnum host_cmd)
{
    enum class RadioState : uint8_t {
        NOTHING, // unused/invalid, defaults to 0
        IDLE, // defaults to 1
        MAINS_POWER_OFF_WAIT,
        MAINS_POWER_ON_WAIT
    };
    // Keep track of the current State (it's a variable of a type RadioState)
    static RadioState currState = RadioState::IDLE; // must be a static declaration, as it persists across invocations.
    static bool do_once = false;

    static unsigned long timerStart = 0; // timer begin
    static unsigned long timerDelay = 0; // how long to wait.
    // Auxilliary handler
    switch (currState) {
    case RadioState::IDLE:
        // Mains power off requested...
        // if Aux1 OFF button is commanded
        if (digitalRead(Aux1_OFF) == LOW || host_cmd == HostCmdEnum::AUX_1_OFF) {
            if (gFlexOperationInProgressFlag) return; // dont do anything if a Flex FSM timing operation is in progress

            if (NO_RADIO_SELECTED_OR_KEYED()) { // DONT allow power off a radio is keyed ??
                if (do_once == false) {
                    AllGrounded();
                    Serial.println(F("CON::Aux1 Off")); // Radio Remote off print it
                    Serial.println(F("CON::Button5"));
                    timerStart = millis();
                    // see if any mains power off delay is desired or not
                    timerDelay = digitalRead(Aux2_PullDown_FlexRMT_Out) ? MAINS_OFF_DELAY : 0;
                    digitalWrite(Aux2_PullDown_FlexRMT_Out, LOW); // command the flex to goto stby
                    digitalWrite(Aux2_Out, LOW); // led on the box.
                    WebText(" Flex-6600 shutting Down\n");          // <--- Add your Text here, for example  "My Device\n"
                    Serial.print(F("\tTimerDelay: "));
                    Serial.println(timerDelay, DEC);
                    do_once = true;
                    displayState(F("\tMAINS_POWER_OFF_WAIT state"));
                    currState = RadioState::MAINS_POWER_OFF_WAIT;
                }
            }
        } else if (digitalRead(Aux1_On) == LOW && NO_RADIO_SELECTED_OR_KEYED() && MAIN_PWR_IS_OFF() || host_cmd == HostCmdEnum::AUX_1_ON) {
            // HANDLE MAINS ON
            if (NO_RADIO_SELECTED_OR_KEYED()) {
                digitalWrite(Aux1_Out, HIGH); // turns on MAINS
                digitalWrite(Aux2_PullDown_FlexRMT_Out, LOW); // Make sure radio Remote is Off
                Serial.println("CON::Aux1 On");
                timerStart = millis();
                timerDelay = MAINS_ON_DELAY;
                Serial.print(F("\tTimerDelay: "));
                Serial.println(timerDelay, DEC);
                do_once = false;
                displayState(F("\tMAINS_POWER_ON_WAIT state"));
                currState = RadioState::MAINS_POWER_ON_WAIT;
            }
        }
        break;
    case RadioState::MAINS_POWER_ON_WAIT:
        if (millis() - timerStart >= timerDelay) {
            Serial.println(F("CON::Aux1 Ready On"));
            displayState(F("\tAUX1 ON IDLE state"));
            WebText(" Astron Power On\n");         // <--- Add your Text here, for example  "My Device\n"
            currState = RadioState::IDLE; // and back to idle...
        }
        break;
    case RadioState::MAINS_POWER_OFF_WAIT:
        if (millis() - timerStart >= timerDelay) {
            digitalWrite(Aux1_Out, LOW); // turns OFF mains
            digitalWrite(Aux2_Out, LOW); // should this be PRIOR to the delay?????
            Serial.println(F("CON::Aux1 Ready Off"));
            displayState(F("\tAUX1 Off IDLE state"));
            WebText(" Astron Power Off\n");         // <--- Add your Text here, for example  "My Device\n"
            currState = RadioState::IDLE; // and back to idle...
        }
        break;
    default:
        break; // do something ?
    }
}

void Aux2_Handler(HostCmdEnum host_cmd)
{
    enum class RadioState : uint8_t {
        NOTHING, // unused/invalid, defaults to 0
        IDLE // defaults to 1
    };
    // Keep track of the current State (it's a variable of a type RadioState)
    static RadioState currState = RadioState::IDLE; // must be a static declaration, as it persists across invocations.
    static bool do_once = false;
}

void All_Grounded_Handler(HostCmdEnum host_cmd)
{
    enum class RadioState : uint8_t {
        NOTHING, // unused/invalid, defaults to 0
        IDLE // defaults to 1
    };
    // Keep track of the current State (it's a variable of a type RadioState)
    static RadioState currState = RadioState::IDLE; // must be a static declaration, as it persists across invocations.
    static bool do_once = false;

    // handle all grounded button stuff & the incoming command if needed.
    switch (currState) {
    case RadioState::IDLE: // do the all grounded function
        // if (digitalRead(All_Grounded_In) == LOW && digitalRead(RadBtnKeyIn_1) == HIGH && digitalRead(RadBtnKeyIn_2) == HIGH && digitalRead(RadBtnKeyIn_3) == HIGH || 
        if ( (ss.allGnd && NO_RADIO_SELECTED_OR_KEYED()) || host_cmd == HostCmdEnum::ALL_GROUNDED) {
            if (do_once == false) {
                AllGrounded();
                Serial.println(F("CON::Button5"));
                do_once = true;
            }
        }
        
        // if (digitalRead(All_Grounded_In) == HIGH) {
        if (!ss.allGnd) { // use debounced input
            do_once = false;
        }
        break;
    default:
        break;
    }
}

void AllGrounded()
{
    Serial.println(F("AllGrounded()"));
    digitalWrite(Relay1_Out, LOW);
    digitalWrite(Relay2_Out, LOW);
    digitalWrite(Relay3_Out, LOW);
    digitalWrite(Relay4_Out, LOW);
    // digitalWrite(Aux1_Out, LOW);
    //  digitalWrite(Aux2_Out, LOW);
}

// Helper routine to track state machine progress
void displayState(String currState)
{
    static String prevState = F("");
    // static unsigned long interval = millis();

    if (currState != prevState) {
        Serial.println(currState);
        // Serial.print(": ");
        // Serial.println(millis() - interval);
        //  printf("%s, interval %d\n", currState, ((millis()-interval)/1000));
        // interval = millis();
        prevState = currState;
    }
}

// use this when you want to send the same message to BOTH serial and web console's.
void SendMsgToConsole(const char *msg) 
{
    Serial.println(msg);
    WebText(msg); 
}

// this function is used to populate the HW_STATUS_STRUCTure that is used
// by main.cpp to build the XML status responses and updates to the webpage
// we put this here to isolate all hardware related accesses to this file.
void get_hardware_status(RAS_HW_STATUS_STRUCT& myhw ) 
{

    myhw.R1_Status = RAS_GET_R1_RELAY_STATUS(); //random(2);
    myhw.R2_Status = RAS_GET_R2_RELAY_STATUS(); //random(2);
    myhw.R3_Status = RAS_GET_R3_RELAY_STATUS(); //random(2);
    myhw.R4_Status = RAS_GET_R4_RELAY_STATUS(); //random(2);
    
    myhw.A1_On_Status = RAS_GET_A1_OUT_STATUS(); // turns on MAINS  random(2);
    myhw.A1_Off_Status = 0; // random(2);
    
    myhw.A2_On_Status = RAS_GET_A2_OUT_STATUS(); // random(2);
    myhw.A2_Off_Status = 0; // random(2);??????????
    
    myhw.ALL_GND_Status = noRadioAntSelected(); // random(2);

    myhw.Xmit_Indicator = ANY_RADIO_SELECTED_OR_KEYED();  // one possible way, maybe look if amp is keyed instead...
    myhw.pSoftwareVersion = CODE_VERSION_STR;
}   

void ReportStatus() // During this reads the status of the outputs
{

    if (digitalRead(Aux1_Out) == HIGH) {
        Serial.println(F("CON::Aux1 Status On"));
    }
    if (digitalRead(Aux1_Out) == LOW) {
        Serial.println(F("CON::Aux1 Status Off"));
    }

    if (digitalRead(Aux2_Out) == HIGH) {
        Serial.println(F("CON::Aux2 Status On"));
    }
    if (digitalRead(Aux2_Out) == LOW) {
        Serial.println(F("CON::Aux2 Status Off"));
    }
    if (digitalRead(Relay1_Out) == HIGH) {
        Serial.println(F("CON::Button1"));
    }

    if (digitalRead(Relay2_Out) == HIGH) {
        Serial.println(F("CON::Button2"));
    }

    if (digitalRead(Relay3_Out) == HIGH) {
        Serial.println(F("CON::Button3"));
    }
    if (digitalRead(Relay4_Out) == HIGH) {
        Serial.println(F("CON::Button4"));
    }
    // ANY_RADIO_SELECTED_OR_KEYED() ? Serial.println(F("CON::LEDRed")) : Serial.println(F("CON::LEDBlack"));
    if (ANY_RADIO_SELECTED_OR_KEYED())
        Serial.println(F("CON::LEDRed"));
    else
        Serial.println(F("CON::LEDBlack"));

}

// from https://en.cppreference.com/w/c/memory/malloc
// void display_freeram()
//{
//    Serial.print(F("- SRAM left: "));
//    Serial.println(freeRam());
//}
// int freeRam()
//{
//    extern int __heap_start, *__brkval;
//    int v;
//    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
//}