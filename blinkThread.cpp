#include "mbed.h"
#include "blinkThread.h"
#include "displayThread.h"

static DigitalOut led1(LED1);
extern bool A_OK;
void blinkThread()
{
    //  Await start signal  
    while(!A_OK) {
        ThisThread::sleep_for(100ms);
        led1 = !led1;
    }

    // Started
    while (A_OK) 
    {
        led1 = !led1;
        displaySendUpdateTime();
        ThisThread::sleep_for(500ms);
    }
}
