#include "mbed.h"
#include "displayThread.h"
#include "awsPublish.h"
#include "ctime"
#include "i2cLCD.h"

//I2C i2c(I2C_SDA, I2C_SCL);

//#define lcdAddress 0x7e
extern bool A_OK;
char printed[80];

typedef enum {
    CMD_temperature,
    CMD_setPoint,
    CMD_time,
    CMD_mode,
    CMD_Debug,
    CMD_light,
    CMD_humid
} command_t;


typedef struct {
    command_t cmd;
    float    value;
} msg_t;


static Queue<msg_t, 32> queue;
static MemoryPool<msg_t, 32> mpool;


void displaySendUpdateTemperature(float temperature)
{
    msg_t *message = mpool.try_alloc();
    if(message)
    {
        message->cmd = CMD_temperature;
        message->value = temperature;
        queue.try_put(message);
    }
}

void displaySendUpdateTime()
{
    msg_t *message = mpool.try_alloc();
    if(message)
    {
        message->cmd = CMD_time;
        message->value = 0;
        queue.try_put(message);
    }
}

void displaySendUpdateSetPoint(float setPoint)
{
    msg_t *message = mpool.try_alloc();
    if(message)
    {
        message->cmd = CMD_setPoint;
        message->value = setPoint;
        queue.try_put(message);
    }
}


void displaySendUpdateMode(float mode)
{
    msg_t *message = mpool.try_alloc();
    if(message)
    {
        message->cmd = CMD_mode;
        message->value = mode;
        queue.try_put(message);
    }
}
void displaySendUpdateHumid(int humid)
{
    msg_t *message = mpool.try_alloc();
    if(message)
    {
        message->cmd = CMD_humid;
        message->value = humid;
        queue.try_put(message);
    }
}
void displaySendUpdateLight(int light)
{
    msg_t *message = mpool.try_alloc();
    if(message)
    {
        message->cmd = CMD_light;
        message->value = light;
        queue.try_put(message);
    }
}
void displaySendDebug(char *text)
{
    msg_t *message = mpool.try_alloc();
    if(message)
    {
        message->cmd = CMD_Debug;
        message->value = 0;
        queue.try_put(message);
        strcpy(printed, text);
    }
}

static void displayAtXY(int x, int y,char *buffer)
{
    // row column
    printf("\033[%d;%dH%s",y,x,buffer);
    fflush(stdout);
    lcd_locate( x-1, y-1);
    lcd_send_string( buffer );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

void displayThread()
{
    char buffer[128];

    lcd_begin();
    lcd_clear();
//    ThisThread::sleep_for(100ms);
//    lcd_noCursor();
//    ThisThread::sleep_for(100ms);
    lcd_send_string((char *)"Starting");


    while(!A_OK) {
        ThisThread::sleep_for(100ms);
    }

    printf("\033[2J\033[H"); // Clear Screen and go Home
    printf("\033[?25l"); // Turn the cursor off
    fflush(stdout);

    while(A_OK)
    {
//        msg_t *message;
//        if (queue.try_get(&message)) { 
     
        osEvent evt = queue.get();
        if (evt.status == osEventMessage) {
            msg_t *message = (msg_t*)evt.value.p;
//            msg_t *message;
//            if (queue.try_get(&message)){
            switch(message->cmd)
            {
                case CMD_temperature:
                    sprintf(buffer,"Temperature = %d.%dC  ",
                           (int)message->value, (int)(message->value*10)%10);
                    displayAtXY(1, 2, buffer);
                break;
                case CMD_setPoint:
                    sprintf(buffer,"Set Point = %d.%dC  ",
                           (int)message->value, (int)(message->value*10)%10);
                    displayAtXY(1, 3, buffer);
                break;
                case CMD_light:
                    sprintf(buffer,"Light Level = %d%c  ",(int)message->value, 0x25);
                    displayAtXY(1, 5, buffer);
                break;
                case CMD_humid:
                    sprintf(buffer,"Rel Humidity =  %d%c  ",(int)message->value, 0x25);
                    displayAtXY(1, 6, buffer);
                break;
                case CMD_time:
                    time_t rawtime;
                    time (&rawtime);
                    sprintf(buffer, "%.*s  ", strlen(ctime(&rawtime))-1, ctime(&rawtime));
                    displayAtXY(1, 1, buffer);
                break;
                case CMD_mode:
                    if(message->value == 0.0)
                        sprintf(buffer,"Mode = Off ");
                    else if (message->value < 0.0)
                        sprintf(buffer,"Mode = Cool");
                    else
                        sprintf(buffer,"Mode = Heat");
                    displayAtXY(1, 4, buffer);
                break;
                case CMD_Debug:
                    displayAtXY(1, 8, printed);
                break;

            }
            mpool.free(message);

        }
    }
    lcd_locate(0,0);
    lcd_send_string((char *)"Shutting Down     ");
    while (1) {
        ThisThread::sleep_for(1000ms);
    }
}
