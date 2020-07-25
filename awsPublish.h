#ifndef AWS_THREAD_H
#define AWS_THREAD_H

#include "sensorThread.h"

void awsSendUpdateTemperature(float temperature);
void awsSendAnnounce1(void);
void awsSendAnnounce2(void);
void awsSendIPAddress(void);
void awsSendUpdateSetPoint(float setPoint);
void awsSendUpdateDelta(float delta);
void awsSendUpdateMode(int controlMode);
void awsSendUpdateLight(int lighLlevel);
void awsSendUpdateHumid(int relHumidity);
void awsSendUpdateIPAddress(void);
void awsSendUpdateShadow(thingData myData);

#endif