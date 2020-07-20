#include "EthernetInterface.h"
#include "NTPClient.h"
#include "aws_credentials.h"
#include "mbed.h"
#include "mbed_trace.h"
#include "mbedtls/debug.h"
#include "ctime"
#include "awsPublish.h"
#include "sensorThread.h"
#include <string>
extern "C" {
// sdk initialization
#include "iot_init.h"
// mqtt methods
#include "iot_mqtt.h"
}

/* globals */
Thread displayThreadHandle;
Thread sensorThreadHandle;


DigitalOut blueLED(LED2);
DigitalOut greenLED(LED1);
DigitalOut redLED(LED3);
// debugging facilities
#define TRACE_GROUP "Main"
static Mutex trace_mutex;

float setPoint = 20.5;
bool A_OK = true;

static void trace_mutex_lock() { trace_mutex.lock(); }
static void trace_mutex_unlock() { trace_mutex.unlock(); }
extern "C" void aws_iot_puts(const char *msg) {
  trace_mutex_lock();
  puts(msg);
  trace_mutex_unlock();
}

static volatile bool buttonPress = false;

/*
 * Callback function called when the button1 is clicked.
 */
void btn1_rise_handler() { buttonPress = true; }

EthernetInterface net;
NTPClient ntp(&net);
#define MQTT_TIMEOUT_MS 15000

// subscription event handler

static void on_message_received(void *pCallbackContext,
                                IotMqttCallbackParam_t *pCallbackParam) {
  auto wait_sem = static_cast<Semaphore *>(pCallbackContext);
  char *payload = (char *)pCallbackParam->u.message.info.pPayload;
  auto payloadLen = pCallbackParam->u.message.info.payloadLength;
  printf("from topic:%s; msg: %.*s\r\n",
         pCallbackParam->u.message.info.pTopicName, payloadLen, payload);
  //    if (strcmp())
  setPoint = std::stof(payload);
  //  if (strncmp("Warning", payload, 7) != 0) {
  //    tr_info("Temperature Set Point %.*s !", payloadLen, payload);

  blueLED = !blueLED;
  wait_sem->release();
}
typedef enum {
  CMD_sendTemperature,
  CMD_sendIPAddress,
  CMD_sendSetPoint,
  CMD_sendLightLevel,
  CMD_sendRelativeHumidity,
  CMD_sendMode,
  CMD_sendDelta,
  CMD_sendAnnounce1,
  CMD_sendAnnounce2,
} command_t;
typedef struct {
  command_t cmd;
  float value; /* eg ADC result of measured voltage */
} msg_t;
static Queue<msg_t, 16> queue;
static MemoryPool<msg_t, 16> mpool;

void awsSendUpdateTemperature(float temperature) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendTemperature;
    message->value = temperature;
    queue.put(message);
  }
}
void awsSendAnnounce1(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendAnnounce1;
    message->value = 0;
    queue.put(message);
  }
}
void awsSendAnnounce2(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendAnnounce2;
    message->value = 0;
    queue.put(message);
  }
}
void awsSendIPAddress(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendIPAddress;
    message->value = 0;
    queue.put(message);
  }
}
void awsSendUpdateSetPoint(float setPoint) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendSetPoint;
    message->value = setPoint;
    queue.put(message);
  }
}
void awsSendUpdateDelta(float delta) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendDelta;
    message->value = delta;
    queue.put(message);
  }
}
void awsSendUpdateMode(int controlMode) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendMode;
    message->value = controlMode;
    queue.put(message);
  }
}
void awsSendUpdateLight(int lighLlevel) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendLightLevel;
    message->value = lighLlevel;
    queue.put(message);
  }
}
void awsSendUpdateHumid(int relHumidity) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendRelativeHumidity;
    message->value = relHumidity;
    queue.put(message);
  }
}
void awsSendUpdateIPAddress(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendIPAddress;
    message->value = 0;
    queue.put(message);
  }
}

int main() {
  
  printf("Started System\r\n");

  int pubCount = 0;


  mbed_trace_mutex_wait_function_set(
      trace_mutex_lock); // only if thread safety is needed
  mbed_trace_mutex_release_function_set(
      trace_mutex_unlock); // only if thread safety is needed
  mbed_trace_init();

  net.connect();
  SocketAddress eth;
  net.get_ip_address(&eth);
  tr_info("IP address: %s",
          eth.get_ip_address() ? eth.get_ip_address() : "None");

  // Set the correct time
  time_t now = ntp.get_timestamp() + 3600;
  set_time(now);
  char timeStr[20];
  //strftime(timeStr, 20, "%I:%M%p", now);
  printf("Connected to Network: IP is: %s at %s\r\n", eth.get_ip_address(), ctime(&now));
  // Enable button 1
  InterruptIn btn1(BUTTON1);
  btn1.rise(btn1_rise_handler);
  
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
      .hostname = MBED_CONF_APP_AWS_ENDPOINT, .port = 8883};
  network_info.u.setup.pNetworkCredentialInfo = {
      .rootCA = aws::credentials::rootCA,
      .clientCrt = aws::credentials::clientCrt,
      .clientKey = aws::credentials::clientKey};

  IotMqttConnectInfo_t connect_info = IOT_MQTT_CONNECT_INFO_INITIALIZER;
  connect_info.awsIotMqttMode = true; // we are connecting to aws servers
  connect_info.pClientIdentifier = MBED_CONF_APP_AWS_CLIENT_IDENTIFIER;
  connect_info.clientIdentifierLength =
      strlen(MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);

  IotMqttConnection_t connection = IOT_MQTT_CONNECTION_INITIALIZER;
  auto connect_status =
      IotMqtt_Connect(&network_info, &connect_info,
                      /* timeout ms */ MQTT_TIMEOUT_MS, &connection);
  if (connect_status != IOT_MQTT_SUCCESS) {
    tr_error("AWS Sdk: Connection to the MQTT broker failed with %u",
             connect_status);
    return -1;
  }
  printf("Connected to MQTT(AWS)\r\n");

  // - Subscribe to sdkTest/sub
  //   On message
  /* Set the members of the subscription. */
  static char topic[50]; // MBED_CONF_APP_AWS_MQTT_TOPIC;
  Semaphore wait_sem{/* count */ 0, /* max_count */ 1};
  sprintf(topic, "%s/setPoint", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);

  IotMqttSubscription_t subscription = IOT_MQTT_SUBSCRIPTION_INITIALIZER;
  subscription.qos = IOT_MQTT_QOS_1;
  subscription.pTopicFilter = topic;
  subscription.topicFilterLength = strlen(topic);
  subscription.callback.function = on_message_received;
  subscription.callback.pCallbackContext = &wait_sem;

  /* Subscribe to the topic using the blocking SUBSCRIBE
   * function. */
  auto sub_status =
      IotMqtt_SubscribeSync(connection, &subscription,
                            /* subscription count */ 1, /* flags */ 0,
                            /* timeout ms */ MQTT_TIMEOUT_MS);
  if (sub_status != IOT_MQTT_SUCCESS) {
    tr_error("AWS Subscribe failed with : %u", sub_status);
  }
  printf("Subscribed\r\n");
  printf("Starting Sensor readings\r\n");
  sensorThreadHandle.start(sensorThread);
  /* Set the members of the publish info. */
  IotMqttPublishInfo_t publish = IOT_MQTT_PUBLISH_INFO_INITIALIZER;
  publish.qos = IOT_MQTT_QOS_1;
  publish.retryLimit = 3;
  publish.retryMs = 1000;
  //    for (uint32_t i = 0; i < 10; i++) {
  bool doPublish = false;
  char buffer[256];
  char update[20];

  int errorCount = 0;

  while (A_OK) {
    wait_sem.try_acquire_for(100ms);
    //    if (readThem == 1) {

    //    readThem = (readThem + 1) % 10;
    while (!queue.empty()) {
      //      if (wait_sem.try_acquire_for(50ms)) {
      //        break;
      //      }
      ThisThread::sleep_for(50ms);
      // Messages can be rejected if sent too close

      osEvent evt = queue.get(0);

      if (evt.status == osEventMessage) {
        msg_t *message = (msg_t *)evt.value.p;
        //        printf("%d", message->cmd);
        switch (message->cmd) {
        case CMD_sendTemperature:
          doPublish = true;
          sprintf(topic, "%s/currentTemp", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%d.%d", (int)message->value,
                  (int)((message->value) * 10) % 10);
#ifdef USING_LEDKEY8
          sprintf(displayBuffer, "T%d S%d ", message->value, (int)setPoint);
//            tr_info("%s\r\n", displayBuffer);
#endif
          break;
        case CMD_sendIPAddress:
          doPublish = true;
          sprintf(topic, "%s/IPAddress", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%s", eth.get_ip_address());
          break;
        case CMD_sendAnnounce1:
          doPublish = true;
          sprintf(topic, "%s/I_am", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%s", "The AR Thang");
          break;
        case CMD_sendAnnounce2:
          doPublish = true;
          sprintf(topic, "%s/Announce", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%f", message->value);
          break;
        case CMD_sendSetPoint:
          doPublish = true;
          sprintf(topic, "%s/setPoint", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%d.%d", (int)message->value,
                  (int)((message->value) * 10) % 10);
          break;
        case CMD_sendDelta:
          doPublish = true;
          sprintf(topic, "%s/delta", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%d.%d", (int)message->value,
                  (int)((message->value) * 10) % 10);
          break;
        case CMD_sendLightLevel:
          doPublish = true;
          sprintf(topic, "%s/lightLevel", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%d%c", (int)message->value, 0x25);
          break;
        case CMD_sendRelativeHumidity:
          doPublish = true;
          sprintf(topic, "%s/relHumidity", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);
          sprintf(update, "%d%c", (int)message->value, 0x25);
          break;
        case CMD_sendMode:
          doPublish = true;
          if ((message->value) >= 1)
            sprintf(update, "Heat");
          else if ((message->value) <= -1)
            sprintf(update, "Cool");
          else
            sprintf(update, "Off");
          sprintf(topic, "%s/controlMode", MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);

          break;
        }
        mpool.free(message);
      }
      if (doPublish) {
        pubCount++;
        publish.pPayload = update;
        publish.payloadLength = strlen(update);

        /* Publish the message. */
        //        sprintf(topic, "%s/lightLevel",
        //        MBED_CONF_APP_AWS_CLIENT_IDENTIFIER);

        publish.pTopicName = topic;
        publish.topicNameLength = strlen(topic);

        tr_info("Publishing telemetry message: %s : %s", topic, update);
        auto pub_status = IotMqtt_PublishSync(connection, &publish,
                                              /* flags */ 0,
                                              /* timeout ms */ MQTT_TIMEOUT_MS);
        if (pub_status != IOT_MQTT_SUCCESS) {
          tr_warning(" failed to publish message with %u.", pub_status);
          if (errorCount++ > 10)
            A_OK = false;
        }
        doPublish = false;
      }
    }
    if (buttonPress)
      A_OK = false;
  }

  /* Close the MQTT connection. */
  IotMqtt_Disconnect(connection, 0);

  IotMqtt_Cleanup();
  IotSdk_Cleanup();
  printf("Done and shut down on %s... Published %d Messages\r\n", ctime(&now), pubCount);
  tr_info("Done");

  while (true) {
    ThisThread::sleep_for(1s);
    blueLED = !blueLED;
  }
  return 0;
}
