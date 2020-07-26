#include "mbed.h"
#include "displayThread.h"
#include "awsPublish.h"
#include "ctime"
I2C i2c_lcd(I2C_SDA, I2C_SCL); // SDA, SCL

#ifdef LCD_PRESENT
#include "TextLCD.h"
TextLCD_I2C lcd(&i2c_lcd, 0x7e, TextLCD::LCD16x2 /*, TextLCD::WS0010*/);  
#endif

#ifdef OLED_PRESENT
#include "Adafruit_SSD1306.h"
Adafruit_SSD1306_I2c gOled2(i2c_lcd, D10, 0x78, 64, 128);
#endif




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
#ifdef OLED_PRESENT
    gOled2.setTextCursor(0, (y-1)<<3);
    gOled2.printf("%s", buffer);
    gOled2.display();
#endif
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

void displayThread()
{
    char buffer[128];

#ifdef LCD_PRESENT
    lcd.setBacklight(TextLCD::LightOn);
    lcd.cls();

    lcd.setCursor(TextLCD::CurOff_BlkOff);
//    ThisThread::sleep_for(100ms);
//    lcd.noCursor();
//    ThisThread::sleep_for(100ms);
    lcd.printf("Starting");
#endif
#ifdef OLED_PRESENT
    gOled2.clearDisplay();
    gOled2.setTextCursor(0, 0);
    gOled2.printf("Starting");
    gOled2.display();

    gOled2.clearDisplay();
    gOled2.setTextCursor(0, 0);
    gOled2.setTextWrap(false);
#endif
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
                    sprintf(buffer,"Temperature = %2.1fC  ", message->value);
                    displayAtXY(1, 2, buffer);
#ifdef LCD_PRESENT
                    lcd.locate( 3,1 );
                    lcd.putc('T');
                    lcd.printf( "%d",(int)round(message->value ));

#endif

                break;
                case CMD_setPoint:
                    sprintf(buffer,"Set Point = %2.1fC  ", message->value);
                    displayAtXY(1, 3, buffer);
#ifdef LCD_PRESENT
                    lcd.locate( 0, 1);
                   lcd.printf("S%d",(int)round(message->value ));
#endif
                break;
                case CMD_light:
                    sprintf(buffer,"Light Level = %d%c  ",(int)message->value, 0x25);
                    displayAtXY(1, 5, buffer);
#ifdef LCD_PRESENT
                    lcd.locate( 7,1 );
                    lcd.printf( "L%d%c",(int)message->value, 0x25 );
#endif
                break;
                case CMD_humid:
                    sprintf(buffer,"Rel Humidity =  %d%c  ",(int)message->value, 0x25);
                    displayAtXY(1, 6, buffer);
#ifdef LCD_PRESENT
                    lcd.locate( 11,1 );
                    lcd.printf( "RH%d%c",(int)message->value, 0x25 );
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
                    lcd.printf( "%s", buffer );
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
                    lcd.printf( "M=" );
                    lcd.printf( "%s", (buffer+7));
#endif
               break;
                case CMD_Debug:
                    displayAtXY(1, 7, printed);
                break;

            }
            mpool.free(message);

        }
    }

    while (1) {
        ThisThread::sleep_for(1000ms);
    }
}
