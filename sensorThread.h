/* 
 * Thread to initialise and periodically read sensors 
 * Storing the acquired data in the thing struct
 *
 */


#include "DS1820.h"
#include "Dht11.h"

struct thingData {
    float tempC = 0.1;
    float prevTempC  = -101;
    int lightLvl = 101;
    int prevLightlLvl = 0;
    int relHumid = 101;
    int prevRelHumid = 0;
    float setPoint = 20.5;
    int controlMode = 0;
} ;
//void sensorThread(void);
