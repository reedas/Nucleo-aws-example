#ifndef DISPLAY_THREAD_H
#define DISPLAY_THREAD_H

//#define OLED_PRESENT
#undef OLED_PRESENT
#define LCD_PRESENT
#ifdef TARGET_Cypress
#define D10 P10_7
#endif
void displayThread();


void displaySendUpdateTemperature(float temperature);
void displaySendUpdateTime();
void displaySendUpdateSetPoint(float setPoint);
void displaySendUpdateMode(float mode);
void displaySendDebug(char *text);
void displaySendUpdateLight(int light);
void displaySendUpdateHumid(int humidity);


#endif