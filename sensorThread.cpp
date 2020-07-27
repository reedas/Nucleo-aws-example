#include "mbed.h"
#include "awsPublish.h"
#include "displayThread.h"

extern float setPoint;
extern bool A_OK;
extern DigitalOut redLED;
extern DigitalOut blueLED;

void sensorThread(void) 
{

  struct thingData myData;
  myData.setPoint = setPoint;
  bool updateRequired = true;

  Dht11 humid(D9);
  DS1820 ds1820(D8);
  AnalogIn ldr(A0);
  AnalogIn TM136(A1);

    while(!A_OK) {
        ThisThread::sleep_for(100ms);
    }
  awsSendUpdateSetPoint(myData.setPoint);
  displaySendUpdateSetPoint(myData.setPoint);
  awsSendUpdateMode(myData.controlMode);
  displaySendUpdateMode(myData.controlMode);
  awsSendUpdateIPAddress();

  for (int i=0; i < 10; i++) {
    blueLED = 0;    
    if (ds1820.begin()) break;
    else blueLED = 1;
    ThisThread::sleep_for(100ms);

  }
  ThisThread::sleep_for(10ms);
  int count = 0;
  char counter[50];
  while (A_OK) { //  float currentTemp, currentSetPt, currentLightLevel,
              //  currentRelHumid;
    myData.lightLvl = (1 - ldr) * 100;
    //    }
    if (humid.read() == 0) {
      myData.relHumid = humid.getHumidity();
//      myData.tempC = humid.getCelsius();
    }
/*  does not work on release build but fine on debug
    float tempVal;
    ds1820.startConversion();
    ThisThread::sleep_for(1000ms);
    redLED = 0;
    if ((ds1820.read(tempVal)) == 0) {
      myData.tempC = tempVal;
    }
    else redLED = 1;
 */
    float volts = 0;
    
    for (int i = 0; i < 10000; i++) volts += TM136.read(); volts *= 0.033;
    myData.tempC =  (volts-50);
    ThisThread::sleep_for(1000ms);
    /* Do we need to cool(-1), heat(+1) or do nothing(0) */
    if (round(10*myData.tempC) != round(10*myData.prevTempC)) {
      awsSendUpdateTemperature(myData.tempC);
      displaySendUpdateTemperature(myData.tempC);
//      sprintf(counter, "%d, %f, %f", count++, myData.tempC, myData.prevTempC);
//      displaySendDebug(counter);
      myData.prevTempC = myData.tempC;
      updateRequired = true;
    }
    if (myData.tempC > myData.setPoint + 1) {
      if (myData.controlMode != -1) {
        myData.controlMode = -1;
        awsSendUpdateMode(myData.controlMode);
        displaySendUpdateMode(myData.controlMode);
        updateRequired = true;
      }
    } else if (myData.tempC < myData.setPoint - 1) {

      if (myData.controlMode != 1) {
        myData.controlMode = 1;
        awsSendUpdateMode(myData.controlMode);
        displaySendUpdateMode(myData.controlMode);
        updateRequired = true;
      }
    } else if (myData.controlMode != 0) {
      myData.controlMode = 0;
      awsSendUpdateMode(myData.controlMode);
      displaySendUpdateMode(myData.controlMode);
      updateRequired = true;
    }

    if (abs(myData.lightLvl - myData.prevLightlLvl) > 2) {
      awsSendUpdateLight(myData.lightLvl);
      displaySendUpdateLight(myData.lightLvl);
      myData.prevLightlLvl = myData.lightLvl;
      updateRequired = true;
    }
    if (abs(myData.relHumid - myData.prevRelHumid) >= 2) {
      awsSendUpdateHumid(myData.relHumid);
      displaySendUpdateHumid(myData.relHumid);
      myData.prevRelHumid = myData.relHumid;
      updateRequired = true;
    }
    if (myData.setPoint != setPoint) {
      myData.setPoint = setPoint;
      awsSendUpdateSetPoint(myData.setPoint);
      displaySendUpdateSetPoint(myData.setPoint);
      updateRequired = true;
    }
//    ThisThread::sleep_for(1000ms);
    if (updateRequired) {
        awsSendUpdateShadow( myData );
        updateRequired = false;
    }

  }
  printf("Sensors turned Off.....\r\n");
  fflush(stdout);

  while(1) {
      ThisThread::sleep_for(1000ms);
  }
}