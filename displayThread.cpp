#include "mbed.h"
#include "displayThread.h"
#include "awsPublish.h"
#include "ctime"
#define LCD_PRESENT
#ifdef LCD_PRESENT
#include "i2cLCD.h"
#endif
//I2C i2c(I2C_SDA, I2C_SCL);
i2cLCD lcd(0x7e, 16, 2);

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
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

void displayThread()
{
    char buffer[128];

    lcd.begin();
    lcd.clear();

    lcd.noCursor();
//    ThisThread::sleep_for(100ms);
//    lcd.noCursor();
//    ThisThread::sleep_for(100ms);
    lcd.printString((char *)"Starting");
    lcd.noBlink();

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
#ifdef LCD_PRESENT
                    sprintf(buffer,"%d",(int)message->value);
                    lcd.locate( 3,1 );
                    lcd.printChar('T');
                    lcd.printString( buffer );

#endif

                break;
                case CMD_setPoint:
                    sprintf(buffer,"Set Point = %d.%dC  ",
                           (int)message->value, (int)(message->value*10)%10);
                    displayAtXY(1, 3, buffer);
#ifdef LCD_PRESENT
                    lcd.locate( 0, 1);
                    lcd.printChar('S');
                    sprintf(buffer,"%d",(int)message->value);
                    lcd.printString( buffer );
#endif
                break;
                case CMD_light:
                    sprintf(buffer,"Light Level = %d%c  ",(int)message->value, 0x25);
                    displayAtXY(1, 5, buffer);
#ifdef LCD_PRESENT
                    sprintf(buffer,"L%d%c",(int)message->value, 0x25);
                    lcd.locate( 7,1 );
                    lcd.printString( buffer );
#endif
                break;
                case CMD_humid:
                    sprintf(buffer,"Rel Humidity =  %d%c  ",(int)message->value, 0x25);
                    displayAtXY(1, 6, buffer);
#ifdef LCD_PRESENT
                    sprintf(buffer,"RH%d%c",(int)message->value, 0x25);
                    lcd.locate( 11,1 );
                    lcd.printString( buffer );
#endif
                break;
                case CMD_time:
                    time_t rawtime;
                    struct tm *timeinfo;
                    time (&rawtime);
                    timeinfo = localtime(&rawtime);
                    sprintf(buffer, "%.*s  ", strlen(ctime(&rawtime))-1, ctime(&rawtime));
                    displayAtXY(1, 1, buffer);
#ifdef LCD_PRESENT
                    lcd.locate( 0, 0 );
                    strftime(buffer, 9, "%H:%M:%S", timeinfo);
                    lcd.printString( buffer );
#endif
                break;
                case CMD_mode:
                    if(message->value == 0.0)
                        sprintf(buffer,"Mode = Off ");
                    else if (message->value < 0.0)
                        sprintf(buffer,"Mode = Cool");
                    else
                        sprintf(buffer,"Mode = Heat");
                    displayAtXY(1, 4, buffer);
 #ifdef LCD_PRESENT
                    lcd.locate(10,0);
                    lcd.printString( (char *)"M=" );
                    lcd.printString( (char *) (buffer+7));
#endif
               break;
                case CMD_Debug:
                    displayAtXY(1, 8, printed);
                break;

            }
            mpool.free(message);

        }
    }
    lcd.locate(0,0);
    lcd.printString((char *)"Shutting Down     ");
    while (1) {
        ThisThread::sleep_for(1000ms);
    }
}
