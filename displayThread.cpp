#include "mbed.h"
#include "displayThread.h"
#include "awsPublish.h"

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
    msg_t *message = mpool.alloc();
    if(message)
    {
        message->cmd = CMD_temperature;
        message->value = temperature;
        queue.put(message);
    }
}

void displaySendUpdateTime()
{
    msg_t *message = mpool.alloc();
    if(message)
    {
        message->cmd = CMD_time;
        message->value = 0;
        queue.put(message);
    }
}

void displaySendUpdateSetPoint(float setPoint)
{
    msg_t *message = mpool.alloc();
    if(message)
    {
        message->cmd = CMD_setPoint;
        message->value = setPoint;
        queue.put(message);
    }
}


void displaySendUpdateMode(float mode)
{
    msg_t *message = mpool.alloc();
    if(message)
    {
        message->cmd = CMD_mode;
        message->value = mode;
        queue.put(message);
    }
}
void displaySendUpdateHumid(int humid)
{
    msg_t *message = mpool.alloc();
    if(message)
    {
        message->cmd = CMD_humid;
        message->value = humid;
        queue.put(message);
    }
}
void displaySendUpdateLight(int light)
{
    msg_t *message = mpool.alloc();
    if(message)
    {
        message->cmd = CMD_light;
        message->value = light;
        queue.put(message);
    }
}
void displaySendDebug(char *text)
{
    msg_t *message = mpool.alloc();
    if(message)
    {
        message->cmd = CMD_Debug;
        message->value = 0;
        queue.put(message);
        strcpy(printed, text);
    }
}

static void displayAtXY(int x, int y,char *buffer)
{
    #ifdef TARGET_CY8CKIT_062_WIFI_BT
    GUI_SetTextAlign(GUI_TA_LEFT);
    GUI_DispStringAt(buffer,(x-1)*8,(y-1)*16);
    #endif
    // row column
    printf("\033[%d;%dH%s",y,x,buffer);
    fflush(stdout);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

void displayThread()
{
    char buffer[128];

    printf("\033[2J\033[H"); // Clear Screen and go Home
    printf("\033[?25l"); // Turn the cursor off
    fflush(stdout);

    #ifdef TARGET_CY8CKIT_062_WIFI_BT
        GUI_Init();
        GUI_SetColor(GUI_WHITE);
        GUI_SetBkColor(GUI_BLACK);
        GUI_SetFont(GUI_FONT_8X16_1);
    #endif

    while(A_OK)
    {
        osEvent evt = queue.get();
        if (evt.status == osEventMessage) {
            msg_t *message = (msg_t*)evt.value.p;
            switch(message->cmd)
            {
                case CMD_temperature:
                    sprintf(buffer,"Temperature = %d.%dC",
                           (int)message->value, (int)(message->value*10)%10);
                    displayAtXY(1, 2, buffer);
                break;
                case CMD_setPoint:
                    sprintf(buffer,"Set Point = %d.%dC",
                           (int)message->value, (int)(message->value*10)%10);
                    displayAtXY(1, 3, buffer);
                break;
                case CMD_light:
                    sprintf(buffer,"Light Level = %d%c",(int)message->value, 0x25);
                    displayAtXY(1, 5, buffer);
                break;
                case CMD_humid:
                    sprintf(buffer,"Rel Humidity =  %d%c",(int)message->value, 0x25);
                    displayAtXY(1, 6, buffer);
                break;
                case CMD_time:
                    time_t rawtime;
                    struct tm * timeinfo;
                    time (&rawtime);
                    rawtime = rawtime; 
                    timeinfo = localtime (&rawtime);
                    strftime (buffer,sizeof(buffer),"%r",timeinfo);
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
}
