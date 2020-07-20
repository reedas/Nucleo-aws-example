#ifndef DISPLAY_THREAD_H
#define DISPLAY_THREAD_H

void displayThread();


void displaySendUpdateTemperature(float temperature);
void displaySendUpdateTime();
void displaySendUpdateSetPoint(float setPoint);
void displaySendUpdateMode(float mode);
void displaySendDebug(char *text);
void displaySendUpdateLight(int light);
void displaySendUpdateHumid(int humidity);


#endif