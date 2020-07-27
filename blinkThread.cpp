#include "mbed.h"
#include "blinkThread.h"
#include "displayThread.h"


extern DigitalOut greenLED;
extern bool A_OK;

void blinkThread()
{
    //  Await start signal  
    while(!A_OK) {
        ThisThread::sleep_for(100ms);
        greenLED = !greenLED;
    }

    // Started
    while (A_OK) 
    {
        greenLED = !greenLED;
        displaySendUpdateTime();

        ThisThread::sleep_for(500ms);
    }
}
