#include "mbed.h"
#include "awsPublish.h"
#include "displayThread.h"
#ifdef TARGET_CY8CPROTO_062_4343W
#define D8 P10_3
#define D9 P10_4
#define A0 P10_0
#define A1 P10_1
#define A2 P10_2

#endif
#define THERMISTOR_PRESENT
#undef TMP36_PRESENT
#undef DS18B20_Present

extern float setPoint;
extern bool A_OK;
extern DigitalOut redLED;
extern DigitalOut blueLED;
#ifdef THERMISTOR_PRESENT

/* Reference resistor in series with the thermistor is of 10 KOhm */
#define R_REFERENCE (float)(10000)

/* Beta constant of this thermistor is 3380 Kelvin. See the thermistor
   (NCP18XH103F03RB) data sheet for more details. */
#define B_CONSTANT (float)(3380)

/* Resistance of the thermistor is 10K at 25 degrees C (from data sheet)
   Therefore R0 = 10000 Ohm, and T0 = 298.15 Kelvin, which gives
   R_INFINITY = R0 e^(-B_CONSTANT / T0) = 0.1192855 */
#define R_INFINITY (float)(0.1192855)

/* Zero Kelvin in degree C */
#define ABSOLUTE_ZERO (float)(-273.15)

//static DigitalIn thermVDD(P10_0);  // if wing is detached and powered from 3.3v
//static DigitalIn thermGND(P10_3);  // don't need to control power to thermistor
                                     // freeing up some inputs P10_0, P10_1, P10_3
static AnalogIn thermOut1(A2);
// static AnalogIn thermOut2(P10_7);
// static AnalogIn testing(P10_4);

static float readTemp()
{
//    thermVDD = 1;
    static int lcount = 0;
    static float Therm[8];
    float thermValue;
//    uint16_t intThermValue;
    float rThermistor;
    for (int lcount = 0; lcount < 8; lcount++) {
    thermValue = thermOut1.read();//*2.4/3.3;
    Therm[lcount] = (B_CONSTANT / (logf((R_REFERENCE / ( 1 / thermValue - 1 )) / 
                                               R_INFINITY))) + ABSOLUTE_ZERO;
    }
        float TempAverage = 0;
        for (int i = 0; i < 8; i++){ 
            TempAverage += Therm[i]; 
            //printf("%d ", (int)(Therm[i]*10));
        }
//        intThermValue = thermOut1.read_u16();
//        printf(" %d ", intThermValue & 0xfff);

        return TempAverage/8;

/*    float refVoltage = thermOut.read() * 2.4; // Range of ADC 0->2*Vref
    float refCurrent = refVoltage  / 10000.0; // 10k Reference Resistor
    float thermVoltage = 3.3 - refVoltage;    // Assume supply voltage is 3.3v
    float thermResistance = thermVoltage / refCurrent; 
    float logrT = (float32_t)log((float64_t)thermResistance);

    // Calculate temperature from the resistance of thermistor using Steinhart-Hart Equation 
    float stEqn = (float32_t)((0.0009032679) + ((0.000248772) * logrT) + 
                             ((2.041094E-07) * pow((float64)logrT, (int)3)));

    temperatureC = (float32_t)(((1.0 / stEqn) - 273.15)  + 0.5);
    float temperatureF = (temperatureC * 9.0/5.0) + 32;
 */
}
#endif
#ifdef TMP36
float readTemp() {
    float volts = 0;
    
    for (int i = 0; i < 10000; i++) volts += TM136.read(); volts = volts*0.033;
    return (volts-50);

}
#endif
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

#ifdef DS18B20_Present
  for (int i=0; i < 10; i++) {
    blueLED = 0;    
    if (ds1820.begin()) break;
    else blueLED = 1;
    ThisThread::sleep_for(100ms);
  }
#endif
//  int count = 0;
//  char counter[50];
  while (A_OK) { //  float currentTemp, currentSetPt, currentLightLevel,
              //  currentRelHumid;
    ThisThread::sleep_for(1000ms);
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
    myData.tempC = readTemp();

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