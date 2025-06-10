
#ifndef DEFS_H
#define DEFS_H  // prevent multiple definitions on Arduino IDE.

// structure to hold the hw status going up to the webpage.
struct RAS_HW_STATUS_STRUCT {
    bool R1_Status;
    bool R2_Status;
    bool R3_Status;
    bool R4_Status;
    bool A1_On_Status;
    bool A1_Off_Status;
    bool A2_On_Status;
    bool A2_Off_Status;
    bool ALL_GND_Status;
    bool Xmit_Indicator;
    const char *pSoftwareVersion;
};

// Declare the possible valid host commands in meaningful English. Enums start enumerating
// at zero, incrementing in steps of 1 unless overridden. We use an
// enum 'class' here for type safety and code readability
enum class HostCmdEnum : byte {
    UNUSED, // unused/invalid, defaults to 0
    SELECT_RADIO_1, // defaults to 1
    SELECT_RADIO_2, // defaults to 2
    SELECT_RADIO_3, // defaults to 3
    SELECT_RADIO_4, // defaults to 4
    AUX_1_ON, // defaults to 5
    AUX_2_ON, // ...
    AUX_1_OFF,
    AUX_2_OFF,
    STATUS_REQUEST,
    ALL_GROUNDED,
    RQST_MAX_EXEC_LOOP_TIME,
    NO_OP = 255
};

#endif
