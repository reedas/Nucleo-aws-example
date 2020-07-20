/* 
 * Thread to initialise and periodically read sensors 
 * Storing the acquired data in the thing struct
 *
 */


#include "DS1820.h"
#include "Dht11.h"

void sensorThread(void);
