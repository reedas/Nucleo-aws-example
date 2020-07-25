/* 
 * Thread to initialise and periodically read sensors 
 * Storing the acquired data in the thing struct
 *
 */


#include "DS1820.h"
#include "Dht11.h"

  struct thingData {

    float tempC;
    float prevTempC;
    int lightLvl;
    int prevLightlLvl;
    int relHumid;
    int prevRelHumid;
    float setPoint;
    int controlMode;
  };

void sensorThread(void);
