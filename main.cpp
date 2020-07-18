#include "mbed.h"
#include "mbed_trace.h"
#include "mbedtls/debug.h"
#include "aws_credentials.h"
#include "EthernetInterface.h"
#include "DS1820.h"
#include "Dht11.h"
#include "NTPClient.h"
//#include "TM1638.h"
#include <string>
extern "C" {
// sdk initialization
#include "iot_init.h"
// mqtt methods
#include "iot_mqtt.h"
}
#define USING_LEDKEY8
#ifdef USING_LEDKEY8
#include "TM1638.h"
#include "Font_7Seg.h"

// KeyData_t size is 4 bytes
TM1638::KeyData_t keydata;

// TM1638_LEDKEY8 declaration (mosi, miso, sclk, cs SPI bus pins)
TM1638_LEDKEY8 LEDKEY8(D11, D12, D13, D10);
static char displayBuffer[20]= "Starting";
#endif
DigitalOut blueled(LED2);
DigitalOut greenLED(LED1);
DigitalOut redLED(LED3);
// debugging facilities
#define TRACE_GROUP "Main"
static Mutex trace_mutex;
static float setPoint = 20.0;
static bool interrupted = false;
static void trace_mutex_lock()
{
    trace_mutex.lock();
}
static void trace_mutex_unlock()
{
    trace_mutex.unlock();
}
extern "C" void aws_iot_puts(const char *msg) {
    trace_mutex_lock();
    puts(msg);
    trace_mutex_unlock();
}
static volatile bool buttonPress = false;

#ifdef USING_LEDKEY8

Thread thread;
void display_thread()
{
            while(1) {
                LEDKEY8.displayStringAt(displayBuffer, 0);
                printf("%s\r\n", displayBuffer);
                blueled = !blueled;
                ThisThread::sleep_for(2000ms);
            }
}
#endif
 /*
 * Callback function called when the button1 is clicked.
 */
void btn1_rise_handler() { buttonPress = true; }

EthernetInterface net;
NTPClient ntp(&net);
#define MQTT_TIMEOUT_MS    15000

// subscription event handler
static void on_message_received(void * pCallbackContext, IotMqttCallbackParam_t *pCallbackParam) {
    auto wait_sem = static_cast<Semaphore*>(pCallbackContext);
    char* payload = (char*)pCallbackParam->u.message.info.pPayload;
    auto payloadLen = pCallbackParam->u.message.info.payloadLength;
    tr_debug("from topic:%s; msg: %.*s", pCallbackParam->u.message.info.pTopicName, payloadLen, payload);
    setPoint = std::stof(payload);
//  if (strncmp("Warning", payload, 7) != 0) {
    tr_info("Temperature Set Point %.*s !", payloadLen, payload);
    interrupted = true;
    redLED = !redLED;
    wait_sem->release();
    
}

int main()
{
    Dht11 humid(D9);
    DS1820 ds1820(D8);
    ds1820.begin();
    static float temperature;
    static int currentTemperature = 0;
    static int lastTemperature = 0;
    AnalogIn ldr(A0);
    static float lightLevel;
    int8_t currentLightLevel = 0;
    int8_t lastlightLevel = 0;
    mbed_trace_mutex_wait_function_set( trace_mutex_lock ); // only if thread safety is needed
    mbed_trace_mutex_release_function_set( trace_mutex_unlock ); // only if thread safety is needed
    mbed_trace_init();
#ifdef USING_LEDKEY8
    LEDKEY8.cls();
//  LEDKEY8.writeData(all_str);
//  ThisThread::sleep_for(2ms);
    LEDKEY8.setBrightness(TM1638_BRT3);
    ThisThread::sleep_for(1ms);
    LEDKEY8.setBrightness(TM1638_BRT0);
    ThisThread::sleep_for(1ms);
    LEDKEY8.setBrightness(TM1638_BRT4);

    ThisThread::sleep_for(1ms);
    LEDKEY8.cls(true);
    LEDKEY8.displayStringAt((char *) "HELLO",0);

    thread.start(display_thread);
    tr_info("Connecting to the network...");
#endif

/*    auto eth = NetworkInterface::get_default_instance();
    if (eth == NULL) {
        tr_error("No Network interface found.");
        return -1;
    }
 
    auto ret = eth->connect();
    if (ret != 0) {
        tr_error("Connection error: %x", ret);
        return -1;
    }
    tr_info("MAC: %s", eth->get_mac_address());
//    tr_info("IP: %s", eth->get_ip_address());
    tr_info("Connection Success");
*/
    net.connect();
    SocketAddress eth;
    net.get_ip_address(&eth);
    tr_info("IP address: %s", eth.get_ip_address() ? eth.get_ip_address() : "None");

// Set the correct time
    time_t now = ntp.get_timestamp() + 3600;
    set_time(now);
// Enable button 1
    InterruptIn btn1(BUTTON1);
    btn1.rise(btn1_rise_handler);
    // demo :
    // - Init sdk
    if (!IotSdk_Init()) {
        tr_error("AWS Sdk: failed to initialize IotSdk");
        return -1;
    }
    auto init_status = IotMqtt_Init();
    if (init_status != IOT_MQTT_SUCCESS) {
        tr_error("AWS Sdk: Failed to initialize IotMqtt with %u", init_status);
        return -1;
    }
    // - Connect to mqtt broker
    IotMqttNetworkInfo_t network_info = IOT_MQTT_NETWORK_INFO_INITIALIZER;
    network_info.pNetworkInterface = aws::get_iot_network_interface();
    // create nework connection
    network_info.createNetworkConnection = true;
    network_info.u.setup.pNetworkServerInfo = {
        .hostname = MBED_CONF_APP_AWS_ENDPOINT,
        .port = 8883
    };
    network_info.u.setup.pNetworkCredentialInfo = {
        .rootCA = aws::credentials::rootCA,
        .clientCrt = aws::credentials::clientCrt,
        .clientKey = aws::credentials::clientKey
    };

    IotMqttConnectInfo_t connect_info = IOT_MQTT_CONNECT_INFO_INITIALIZER;
    connect_info.awsIotMqttMode = true; // we are connecting to aws servers
    connect_info.pClientIdentifier = MBED_CONF_APP_AWS_CLIENT_IDENTIFIER;
    connect_info.clientIdentifierLength = strlen(MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);

    IotMqttConnection_t connection = IOT_MQTT_CONNECTION_INITIALIZER;
    auto connect_status = IotMqtt_Connect(&network_info, &connect_info, /* timeout ms */ MQTT_TIMEOUT_MS, &connection);
    if (connect_status != IOT_MQTT_SUCCESS) {
        tr_error("AWS Sdk: Connection to the MQTT broker failed with %u", connect_status);
        return -1;
    }
    // - Subscribe to sdkTest/sub
    //   On message
    //   - Display on the console: "Hello %s", message
    /* Set the members of the subscription. */
    static char topic[50];  //MBED_CONF_APP_AWS_MQTT_TOPIC;
    Semaphore wait_sem {/* count */ 0, /* max_count */ 1};
    sprintf( topic,"%s/setPoint", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);

    IotMqttSubscription_t subscription = IOT_MQTT_SUBSCRIPTION_INITIALIZER;
    subscription.qos = IOT_MQTT_QOS_1;
    subscription.pTopicFilter = topic;
    subscription.topicFilterLength = strlen(topic);
    subscription.callback.function = on_message_received;
    subscription.callback.pCallbackContext = &wait_sem;

    /* Subscribe to the topic using the blocking SUBSCRIBE
     * function. */
    auto sub_status = IotMqtt_SubscribeSync(connection, &subscription,
                                            /* subscription count */ 1, /* flags */ 0,
                                            /* timeout ms */ MQTT_TIMEOUT_MS );
    if (sub_status != IOT_MQTT_SUCCESS) {
        tr_error("AWS Subscribe failed with : %u", sub_status);
    }
    
    /* Set the members of the publish info. */
    IotMqttPublishInfo_t publish = IOT_MQTT_PUBLISH_INFO_INITIALIZER;
    publish.qos = IOT_MQTT_QOS_1;
    publish.retryLimit = 3;
    publish.retryMs = 1000;
//    for (uint32_t i = 0; i < 10; i++) {
    bool A_OK = true;
    int errorCount = 0;
    while(A_OK) {
        // - for i in 0..9
        //  - wait up to 1 sec
        //  - if no message received Publish: "You have %d sec remaining to say hello...", 10-i
        //  - other wise, exit
        temperature = 0;
        float tempval;
        lightLevel = 0;
        int result = 0;
        bool tempError = false;
        humid.read();
        for (int i=0; i < 10; i++){
            ds1820.startConversion();
            if (wait_sem.try_acquire_for(1000ms)) {
                break;
            }
            ThisThread::sleep_for(10ms);
            if((ds1820.read(tempval)) != 0) {
                tempval = currentTemperature / 10;
                tempError = true;
                tr_warning("DS18B20 read Error.");
            }
            temperature += tempval;
            lightLevel += (1 - ldr) * 10;
        }

        /* prepare any messages */
        static char message[64];
        if (!interrupted) {
            currentTemperature = (int)temperature;
            currentLightLevel = (int)lightLevel;
        }
        else {
            interrupted = false;
            tr_info("Got interrupted ... discarding measurements %d, %d", (int)temperature, (int)lightLevel);
        }
//        float dht11TempC=
        printf("Temperature is %d.%d, Light Level is %d%c, Temperature is %d, RH is %d%c\r\n",
                currentTemperature/10, currentTemperature%10, currentLightLevel, 0x25, 
                (int)humid.getCelsius(), (int)humid.getHumidity(), 0x25);
        if (currentTemperature != lastTemperature) {
            snprintf(message, 64, "%d.%d", currentTemperature / 10, currentTemperature % 10 );
            lastTemperature = currentTemperature;
            publish.pPayload = message;
            publish.payloadLength = strlen(message);

            /* Publish the message. */
            sprintf( topic,"%s/temperature", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
            publish.pTopicName = topic;
            publish.topicNameLength = strlen(topic);

            tr_info("Publishing telemetry message: %s", message);
            auto pub_status = IotMqtt_PublishSync(connection, &publish,
                                              /* flags */ 0, /* timeout ms */ MQTT_TIMEOUT_MS);
            if (pub_status != IOT_MQTT_SUCCESS) {
                tr_warning(" failed to publish message with %u.", pub_status);
                if (errorCount++ > 10) A_OK = false;
            }

        }
        if (currentLightLevel != lastlightLevel) {
            snprintf(message, 64, "%d", currentLightLevel );
            lastlightLevel = currentLightLevel;
            publish.pPayload = message;
            publish.payloadLength = strlen(message);

            /* Publish the message. */
            sprintf( topic,"%s/lightLevel", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);

            publish.pTopicName = topic;
            publish.topicNameLength = strlen(topic);

            tr_info("Publishing telemetry message: %s", message);
            auto pub_status = IotMqtt_PublishSync(connection, &publish,
                                              /* flags */ 0, /* timeout ms */ MQTT_TIMEOUT_MS);
            if (pub_status != IOT_MQTT_SUCCESS) {
                tr_warning(" failed to publish message with %u.", pub_status);
                if (errorCount++ > 10) A_OK = false;
            }

        }
#ifdef USING_LEDKEY8
        sprintf(displayBuffer, "T%d S%d ",currentTemperature/10, (int)setPoint);
//            tr_info("%s\r\n", displayBuffer);
#endif
    if (buttonPress) A_OK = false;
    }

    /* Close the MQTT connection. */
    IotMqtt_Disconnect(connection, 0);

    IotMqtt_Cleanup();
    IotSdk_Cleanup();

    tr_info("Done");
    while (true) {
        ThisThread::sleep_for(1s);
    }
    return 0;
}
