#pragma once
// DebouncerClass.h - Debouncing + edge-detection of button inputs
// 23-Oct-2025 w9zv    v1.1    Added press/release edge reporting

#include <Arduino.h>

// Define to signal debouncer availability to other code
#ifndef HAVE_DEBOUNCER_CLASS
#define HAVE_DEBOUNCER_CLASS 1
#endif

#ifndef DEBOUNCE_MS
#define DEBOUNCE_MS 40   // default debounce time in milliseconds
#endif

// NOTE: Pins (RadBtnKeyIn_X, All_Grounded_In, Aux*_*) must be defined elsewhere
// and configured (usually INPUT_PULLUP) before calling update().

class DebouncedInputs {
public:
    // Stable debounced view of inputs for this loop iteration
    struct Snapshot {
        bool r1, r2, r3, r4;
        bool allGnd;
        bool aux1On, aux1Off;
        bool aux2On, aux2Off;
    };

    // Per-input edges computed on the last update()
    struct Edges {
        bool r1Press,  r1Release;
        bool r2Press,  r2Release;
        bool r3Press,  r3Release;
        bool r4Press,  r4Release;
        bool allGndPress,  allGndRelease;
        bool aux1OnPress,  aux1OnRelease;
        bool aux1OffPress, aux1OffRelease;
        bool aux2OnPress,  aux2OnRelease;
        bool aux2OffPress, aux2OffRelease;
    };

    explicit DebouncedInputs(unsigned long debounceMs)
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

        // One-time seed of state to avoid spurious edges on startup
        if (!initialized_) {
            for (int i = 0; i < COUNT; ++i) {
                int rd = digitalRead(states_[i].pin);
                states_[i].stable = rd;
                states_[i].last = rd;
                states_[i].lastChangeMs = now;
            }
            // Seed both current and previous snapshots consistently
            snap_ = mapSnapshotFromStable_();
            prev_ = snap_;
            initialized_ = true;
            // No edges on first update
            memset(&edges_, 0, sizeof(edges_));
            return;
        }

        // Debounce all tracked inputs
        for (int i = 0; i < COUNT; ++i) {
            int rd = digitalRead(states_[i].pin);
            if (rd != states_[i].last) {
                states_[i].last = rd;
                states_[i].lastChangeMs = now;
            }
            if ((now - states_[i].lastChangeMs) >= debounceMs_ &&
                states_[i].stable != states_[i].last) {
                states_[i].stable = states_[i].last;
            }
        }

        // Map to boolean snapshot (LOW == pressed with INPUT_PULLUP)
        snap_ = mapSnapshotFromStable_();

        // Compute press/release edges against previous snapshot
        edges_.r1Press       =  snap_.r1      && !prev_.r1;
        edges_.r1Release     = !snap_.r1      &&  prev_.r1;
        edges_.r2Press       =  snap_.r2      && !prev_.r2;
        edges_.r2Release     = !snap_.r2      &&  prev_.r2;
        edges_.r3Press       =  snap_.r3      && !prev_.r3;
        edges_.r3Release     = !snap_.r3      &&  prev_.r3;
        edges_.r4Press       =  snap_.r4      && !prev_.r4;
        edges_.r4Release     = !snap_.r4      &&  prev_.r4;

        edges_.allGndPress   =  snap_.allGnd  && !prev_.allGnd;
        edges_.allGndRelease = !snap_.allGnd  &&  prev_.allGnd;

        edges_.aux1OnPress    =  snap_.aux1On  && !prev_.aux1On;
        edges_.aux1OnRelease  = !snap_.aux1On  &&  prev_.aux1On;
        edges_.aux1OffPress   =  snap_.aux1Off && !prev_.aux1Off;
        edges_.aux1OffRelease = !snap_.aux1Off &&  prev_.aux1Off;
        edges_.aux2OnPress    =  snap_.aux2On  && !prev_.aux2On;
        edges_.aux2OnRelease  = !snap_.aux2On  &&  prev_.aux2On;
        edges_.aux2OffPress   =  snap_.aux2Off && !prev_.aux2Off;
        edges_.aux2OffRelease = !snap_.aux2Off &&  prev_.aux2Off;

        // Keep for next edge calculation
        prev_ = snap_;
    }

    // Accessors
    const Snapshot& get() const  { return snap_; }
    const Edges&    getEdges() const { return edges_; }

    // Convenience checks (radio-only helpers)
    bool anyRadio()  const { return snap_.r1 || snap_.r2 || snap_.r3 || snap_.r4; }
    bool noneRadio() const { return !snap_.r1 && !snap_.r2 && !snap_.r3 && !snap_.r4; }
    bool onlyR1() const { return  snap_.r1 && !snap_.r2 && !snap_.r3 && !snap_.r4; }
    bool onlyR2() const { return !snap_.r1 &&  snap_.r2 && !snap_.r3 && !snap_.r4; }
    bool onlyR3() const { return !snap_.r1 && !snap_.r2 &&  snap_.r3 && !snap_.r4; }
    bool onlyR4() const { return !snap_.r1 && !snap_.r2 && !snap_.r3 &&  snap_.r4; }

private:
    struct DebState {
        int pin = -1;
        int stable = HIGH;              // last stable level
        int last = HIGH;                // most recent read level
        unsigned long lastChangeMs = 0; // time of last change in 'last'
    };

    static constexpr int COUNT = 9;
    DebState      states_[COUNT];
    Snapshot      snap_{};
    Snapshot      prev_{};
    Edges         edges_{};
    unsigned long debounceMs_;
    bool          initialized_;

    // Map stable levels (HIGH/LOW) into Snapshot booleans (pressed == true)
    Snapshot mapSnapshotFromStable_() const {
        Snapshot s{};
        s.r1      = (states_[0].stable == LOW);
        s.r2      = (states_[1].stable == LOW);
        s.r3      = (states_[2].stable == LOW);
        s.r4      = (states_[3].stable == LOW);
        s.allGnd  = (states_[4].stable == LOW);
        s.aux1On  = (states_[5].stable == LOW);
        s.aux1Off = (states_[6].stable == LOW);
        s.aux2On  = (states_[7].stable == LOW);
        s.aux2Off = (states_[8].stable == LOW);
        return s;
    }
};

// Declare the global instance here; OR define it in exactly one .cpp:
// In one source file (e.g., RAS_hw.cpp) do (in this order):
//   #define DEBOUNCER_DEFINE
//   #include "DebouncerClass.h"

#ifdef DEBOUNCER_DEFINED
DebouncedInputs gInputs(DEBOUNCE_MS);
#else
extern DebouncedInputs gInputs;
#endif

// Remap the older macros to snapshot-based helpers
#ifdef ONLY_RADIO_1_SELECTED_OR_KEYED
#undef ONLY_RADIO_1_SELECTED_OR_KEYED
#endif
#ifdef ONLY_RADIO_2_SELECTED_OR_KEYED
#undef ONLY_RADIO_2_SELECTED_OR_KEYED
#endif
#ifdef ONLY_RADIO_3_SELECTED_OR_KEYED
#undef ONLY_RADIO_3_SELECTED_OR_KEYED
#endif
#ifdef ONLY_RADIO_4_SELECTED_OR_KEYED
#undef ONLY_RADIO_4_SELECTED_OR_KEYED
#endif
#ifdef ANY_RADIO_SELECTED_OR_KEYED
#undef ANY_RADIO_SELECTED_OR_KEYED
#endif
#ifdef NO_RADIO_SELECTED_OR_KEYED
#undef NO_RADIO_SELECTED_OR_KEYED
#endif

#define ONLY_RADIO_1_SELECTED_OR_KEYED() (gInputs.onlyR1())
#define ONLY_RADIO_2_SELECTED_OR_KEYED() (gInputs.onlyR2())
#define ONLY_RADIO_3_SELECTED_OR_KEYED() (gInputs.onlyR3())
#define ONLY_RADIO_4_SELECTED_OR_KEYED() (gInputs.onlyR4())
#define ANY_RADIO_SELECTED_OR_KEYED()    (gInputs.anyRadio())
#define NO_RADIO_SELECTED_OR_KEYED()     (gInputs.noneRadio())

// Usage pattern (once per loop):
//   gInputs.update();
//   const auto& ss = gInputs.get();
//   const auto& ee = gInputs.getEdges();
// Then act on ss.* (levels) or e.* (edges) as needed.

// if (e.r3Press && !s.r1 && !s.r2 && !s.r4) {
//     // act on R3 press edge only
// }
// if (e.r3Release) {
//     // act on R3 release edge
// }