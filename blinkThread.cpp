#include "mbed.h"
#include "blinkThread.h"
#include "displayThread.h"
#include "ResetReason.h"
#include <string>

extern DigitalOut greenLED;
extern bool A_OK;

std::string reset_reason_to_string(const reset_reason_t reason)
{
    switch (reason) {
        case RESET_REASON_POWER_ON:
            return "Power On";
        case RESET_REASON_PIN_RESET:
            return "Hardware Pin";
        case RESET_REASON_SOFTWARE:
            return "Software Reset";
        case RESET_REASON_WATCHDOG:
            return "Watchdog";
        default:
            return "Other Reason";
    }
}

void blinkThread()
{
    const reset_reason_t reason = ResetReason::get();

    printf("Last system reset reason: %s\r\n", reset_reason_to_string(reason).c_str());

//  Await start signal  
    while(!A_OK) {
        ThisThread::sleep_for(100ms);
        greenLED = !greenLED;
    }
    Watchdog &watchdog = Watchdog::get_instance();
    watchdog.start(10000);

    // Started
    while (A_OK) 
    {
        greenLED = !greenLED;
        displaySendUpdateTime();
        Watchdog::get_instance().kick();
        ThisThread::sleep_for(500ms);
    }
}
