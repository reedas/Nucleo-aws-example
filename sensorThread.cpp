#include "mbed.h"
#include "sensorThread.h"
#include "awsPublish.h"
#include "displayThread.h"

extern float setPoint;
extern bool A_OK;

void sensorThread(void) 
{

  struct thingData {

    float tempC = 0.1;
    float prevTempC = -101;
    int lightLvl = 101;
    int prevLightlLvl = 0;
    int relHumid = 101;
    int prevRelHumid = 0;
    float setPoint = 20.5;
    int controlMode = 0;
  } myData;

  Dht11 humid(D9);
  DS1820 ds1820(D8);
  AnalogIn ldr(A0);

    while(!A_OK) {
        ThisThread::sleep_for(100ms);
    }
  awsSendUpdateSetPoint(myData.setPoint);
  displaySendUpdateSetPoint(myData.setPoint);
  awsSendUpdateMode(myData.controlMode);
  displaySendUpdateMode(myData.controlMode);
  awsSendUpdateIPAddress();

  for (int i=0; i < 5; i++) {
    if (ds1820.begin()) break;
    ThisThread::sleep_for(10ms);
  }
  ThisThread::sleep_for(10ms);

  while (A_OK) { //  float currentTemp, currentSetPt, currentLightLevel,
              //  currentRelHumid;
    myData.lightLvl = (1 - ldr) * 100;
    //    }
    if (humid.read() == 0) {
      myData.relHumid = humid.getHumidity();
    }

    float tempVal;
    ds1820.startConversion();
    if ((ds1820.read(tempVal)) == 0) {
      myData.tempC = tempVal;
    }
    /* Do we need to cool(-1), heat(+1) or do nothing(0) */
    if (myData.tempC != myData.prevTempC) {
      awsSendUpdateTemperature(myData.tempC);
      displaySendUpdateTemperature(myData.tempC);
      myData.prevTempC = myData.tempC;
    }
    if (myData.tempC > myData.setPoint + 0.5) {
      if (myData.controlMode != -1) {
        myData.controlMode = -1;
        awsSendUpdateMode(myData.controlMode);
        displaySendUpdateMode(myData.controlMode);
      }
    } else if (myData.tempC < myData.setPoint - 0.5) {

      if (myData.controlMode != 1) {
        myData.controlMode = 1;
        awsSendUpdateMode(myData.controlMode);
        displaySendUpdateMode(myData.controlMode);
      }
    } else if (myData.controlMode != 0) {
      myData.controlMode = 0;
      awsSendUpdateMode(myData.controlMode);
      displaySendUpdateMode(myData.controlMode);
    }

    if (abs(myData.lightLvl - myData.prevLightlLvl) > 2) {
      awsSendUpdateLight(myData.lightLvl);
      displaySendUpdateLight(myData.lightLvl);
      myData.prevLightlLvl = myData.lightLvl;
    }
    if (abs(myData.relHumid - myData.prevRelHumid) >= 2) {
      awsSendUpdateHumid(myData.relHumid);
      displaySendUpdateHumid(myData.relHumid);
      myData.prevRelHumid = myData.relHumid;
    }
    if (myData.setPoint != setPoint) {
      myData.setPoint = setPoint;
      awsSendUpdateSetPoint(myData.setPoint);
      displaySendUpdateSetPoint(myData.setPoint);
    }

/*    printf("Temp/setPoint %d.%d/%d.%d, light %d%c, RelHumid %d%c\r\n",
           (int)myData.tempC, (int)(myData.tempC * 10) % 10,
           (int)myData.setPoint, (int)(myData.setPoint * 10) % 10,
           (int)myData.lightLvl, 0x25,
           (int)myData.relHumid, 0x25);
 */ 
    ThisThread::sleep_for(1000ms);
  }
  printf("Sensors turned Off.....\r\n");
}