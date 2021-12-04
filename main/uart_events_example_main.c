/* UART Events Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

// extra for mqtt
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

//#include "mqtt_com.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

static esp_mqtt_client_handle_t client;



//esp_mqtt_client_handle_t client();

void log_error_if_nonzero(const char * message, int error_code);
esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(void);
void mqtt_main();


static const char *TAG = "uart_events";

/**
 * This example shows how to use the UART driver to handle special UART events.
 *
 * It also reads data from UART0 directly, and echoes it to console.
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: on
 * - Pin assignment: TxD (default), RxD (default)
 */

#define EX_UART_NUM UART_NUM_1
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;




static void uart_event_send(void *pvParameters) 
{
    while (1)
    {
         // Test command
        char testCommand[] = {0x7E, 0x00, 0x0F, 0x17, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x41, 0x6B, 0x8C, 0x58, 0xFF, 0xFE, 0x02, 0x49, 0x53, 0x07};
        char testTestCommand[] = {0x7E, 0x0F, 0x17, 0x01 , 0x13 , 0xA2 , 0x40 , 0xE7 , 0x1A , 0xED , 0xFF , 0xFE , 0x02 , 0x49 , 0x53 , 0x69, 0x0a};

        printf("\nCommand sending\n");
        uart_write_bytes(EX_UART_NUM, testCommand, 19);
        printf(testCommand);
        printf("\nCommand send\n");
        ESP_LOGI(TAG, "Command send");

        //vTaskDelete(NULL);
        vTaskDelay(pdMS_TO_TICKS(10000));   
    }
    vTaskDelete(NULL);
}


static void uart_event_task(void *pvParameters)
{


    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);



    for(;;) {
        
        // Test command (moved to another function)
      //  char testCommand[] = {0x7E, 0x00, 0x0F, 0x17, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x41, 0x6B, 0x8C, 0x58, 0xFF, 0xFE, 0x02, 0x49, 0x53, 0x07};
        
        
        


        char stting[] = "hello\n";


        // moved to another function
      //  uart_write_bytes(EX_UART_NUM, testCommand, 19);
        //uart_write_bytes(EX_UART_NUM, testTestCommand, strlen(testTestCommand));
        //uart_write_bytes(EX_UART_NUM, stting, strlen(stting));

        printf("Hello!");

        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)) {

            printf("Info Receive\n");

            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM); 



            //ESP_LOGI(TAG, "uart content: %s", event.);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ESP_LOGI(TAG, "[DATA EVT]:");
                    //uart_write_bytes(UART_NUM_0, (const char*) dtmp, event.size);
                    //int n = strtol(dtmp, NULL, 16);
                    //printf("Number: %d\n", n);
                    for (int i = 0; i < event.size; i++)
                    {
                        printf("%c", dtmp[i]);
                    }

                    // Recived number
                    int waterQuality = dtmp[event.size - 1];
                    

                    char str[4];
                    sprintf(str, "%d", waterQuality);
                    printf("MQTT: Publising");
                    esp_mqtt_client_publish(client, "/topic/qos0", str, 4, 0, 0);
                    // mqtt setup
                    

                    // Send to mqtt

                    printf("Done printing\n");
                   // printf("\n%x\n", dtmp);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                    int pos = uart_pattern_pop_pos(EX_UART_NUM);
                    ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(EX_UART_NUM);
                    } else {
                        uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                        uint8_t pat[PATTERN_CHR_NUM + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                        ESP_LOGI(TAG, "read data: %s", dtmp);
                        ESP_LOGI(TAG, "read pat : %s", pat);
                    }
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}


// #include "mqtt_com.h"

static const char *TAGmqtt = "MQTT_EXAMPLE";
//static int numberToSend = 0;

void log_error_if_nonzero(const char * message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAGmqtt, "Last error %s: 0x%x", message, error_code);
    }
}

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAGmqtt, "MQTT_EVENT_CONNECTED");

            //char str[3];
            //sprintf(str, "%d", numberToSend);

           msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "Startop test", 0, 0, 0);
           ESP_LOGI(TAGmqtt, "sent publish successful, msg_id=%d", msg_id);

        //  msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        //  ESP_LOGI(TAGmqtt, "sent subscribe successful, msg_id=%d", msg_id);

        /*    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id); */
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAGmqtt, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAGmqtt, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
           // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "22", 0, 0, 0);
           // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAGmqtt, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAGmqtt, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAGmqtt, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAGmqtt, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAGmqtt, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

            }
            break;
        default:
            ESP_LOGI(TAGmqtt, "Other event id:%d", event->event_id);
            break;
    }
    printf ("MQTT: Done");
    return ESP_OK;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAGmqtt, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAGmqtt, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}


void app_main(void)
{
    // ------------------------------ MQTT ---------------------------

       //numberToSend = toSend;

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());


    // ------------------------------ END MQTT ---------------------------
    
    //esp_log_level_set(TAG, ESP_LOG_INFO);

    // Configure parameters of an UART driver,
    //  communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 100, &uart0_queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    //uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(UART_NUM_1, 23, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(UART_NUM_1, '+', PATTERN_CHR_NUM, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART_NUM_1, 100);

    //Create a task to handler UART event from ISR

    //mqtt_main(32);
  //  mqtt_main();

    printf("Testing!");

    xTaskCreate(uart_event_send, "uart_event_send", 2048, NULL, 12, NULL);
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);


    // start
    mqtt_app_start();

}
