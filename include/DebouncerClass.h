#include <Arduino.h>

#define DEBOUNCE_MS 40  // Debounce time in milliseconds
class DebouncedInputs {
    public:
    struct Snapshot {
        bool r1, r2, r3, r4;
        bool allGnd;
        bool aux1On, aux1Off;
        bool aux2On, aux2Off;
    };

    DebouncedInputs(unsigned long debounceMs)
        : debounceMs_(debounceMs), initialized_(false) {
        // Pin table must match Snapshot mapping order
        states_[0].pin = RadBtnKeyIn_1;
        states_[1].pin = RadBtnKeyIn_2;
        states_[2].pin = RadBtnKeyIn_3;
        states_[3].pin = RadBtnKeyIn_4;
        states_[4].pin = All_Grounded_In;
        states_[5].pin = Aux1_On;
        states_[6].pin = Aux1_OFF;
        states_[7].pin = Aux2_On;
        states_[8].pin = Aux2_OFF;
    }

    void update() {
        unsigned long now = millis();
        if (!initialized_) {
            for (int i = 0; i < COUNT; ++i) {
                int rd = digitalRead(states_[i].pin);
                states_[i].stable = rd;
                states_[i].last = rd;
                states_[i].lastChangeMs = now;
            }
            initialized_ = true;
        }
        for (int i = 0; i < COUNT; ++i) {
            int rd = digitalRead(states_[i].pin);
            if (rd != states_[i].last) {
                states_[i].last = rd;
                states_[i].lastChangeMs = now;
            }
            if ((now - states_[i].lastChangeMs) >= debounceMs_ && states_[i].stable != states_[i].last) {
                states_[i].stable = states_[i].last;
            }
        }
        // Map stable levels to pressed booleans (LOW == pressed with INPUT_PULLUP)
        snap_.r1      = (states_[0].stable == LOW);
        snap_.r2      = (states_[1].stable == LOW);
        snap_.r3      = (states_[2].stable == LOW);
        snap_.r4      = (states_[3].stable == LOW);
        snap_.allGnd  = (states_[4].stable == LOW);
        snap_.aux1On  = (states_[5].stable == LOW);
        snap_.aux1Off = (states_[6].stable == LOW);
        snap_.aux2On  = (states_[7].stable == LOW);
        snap_.aux2Off = (states_[8].stable == LOW);
    }

    const Snapshot& get() const { return snap_; }

    // Convenience checks used by macros
    bool anyRadio() const { return snap_.r1 || snap_.r2 || snap_.r3 || snap_.r4; }
    bool noneRadio() const { return !snap_.r1 && !snap_.r2 && !snap_.r3 && !snap_.r4; }
    bool onlyR1() const { return snap_.r1 && !snap_.r2 && !snap_.r3 && !snap_.r4; }
    bool onlyR2() const { return !snap_.r1 && snap_.r2 && !snap_.r3 && !snap_.r4; }
    bool onlyR3() const { return !snap_.r1 && !snap_.r2 && snap_.r3 && !snap_.r4; }
    bool onlyR4() const { return !snap_.r1 && !snap_.r2 && !snap_.r3 && snap_.r4; }

private:
    struct DebState {
        int pin = -1;
        int stable = HIGH;
        int last = HIGH;
        unsigned long lastChangeMs = 0;
    };
    static constexpr int COUNT = 9;
    DebState states_[COUNT];
    Snapshot snap_{};
    unsigned long debounceMs_;
    bool initialized_;
};

// Global instance
static DebouncedInputs gInputs(DEBOUNCE_MS);

// Remap the multi-read macros to snapshot-based methods
#undef ONLY_RADIO_1_SELECTED_OR_KEYED
#undef ONLY_RADIO_2_SELECTED_OR_KEYED
#undef ONLY_RADIO_3_SELECTED_OR_KEYED
#undef ONLY_RADIO_4_SELECTED_OR_KEYED
#undef ANY_RADIO_SELECTED_OR_KEYED
#undef NO_RADIO_SELECTED_OR_KEYED

#define ONLY_RADIO_1_SELECTED_OR_KEYED() (gInputs.onlyR1())
#define ONLY_RADIO_2_SELECTED_OR_KEYED() (gInputs.onlyR2())
#define ONLY_RADIO_3_SELECTED_OR_KEYED() (gInputs.onlyR3())
#define ONLY_RADIO_4_SELECTED_OR_KEYED() (gInputs.onlyR4())
#define ANY_RADIO_SELECTED_OR_KEYED()    (gInputs.anyRadio())
#define NO_RADIO_SELECTED_OR_KEYED()     (gInputs.noneRadio())
// ...existing code...


//Call update() once per loop
// ...existing code...
// void loop2(HostCmdEnum& myHostCmd)
// {
//     // Snapshot all debounced inputs once per iteration
//     gInputs.update();

//     // ...existing code...
//     if (Serial.available() > 0) {
//         myHostCmd = (HostCmdEnum)(Serial.read());
//     }
//     // ...existing code...
// }
// // ...existing code...
