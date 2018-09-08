#pragma once

#include <PJON.h>
#include <PJONDefines.h>

#ifndef ESP32
#error "shouldn't happen"
#endif

// ESP includes
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "rom/ets_sys.h"
#include "rom/crc.h"

static const char *TAG = "espnow";

/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_STATION_MODE
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define ESPNOW_MAX_PACKET   250
#define ESPNOW_QUEUE_SIZE           6

static uint8_t espnow_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define IS_BROADCAST_ADDR(addr) (memcmp(addr, espnow_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)


typedef enum {
    ESPNOW_SEND_CB,
    ESPNOW_RECV_CB,
} espnow_event_id_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} espnow_packet_t;

typedef union {
    espnow_event_send_cb_t send_cb;
    espnow_packet_t recv_cb;
} espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    espnow_event_id_t id;
    espnow_event_info_t info;
} espnow_event_t;

enum {
    ESPNOW_DATA_BROADCAST,
    ESPNOW_DATA_UNICAST,
    ESPNOW_DATA_MAX,
};


static uint8_t last_mac[ESP_NOW_ETH_ALEN];
static TaskHandle_t pjon_task_h = NULL, espnow_recv_task_h = NULL;
static espnow_packet_t *espnow_received = NULL;
static xQueueHandle espnow_queue = NULL;


static esp_err_t espnow_event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi started");
            break;
        default:
            break;
    }
    return ESP_OK;
}


/* WiFi should start before using ESPNOW */
static esp_err_t espnow_wifi_init(void) {
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(espnow_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* In order to simplify example, channel is set after WiFi started.
     * This is not necessary in real application if the two devices have
     * been already on the same channel.
     */
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    return ESP_OK;
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {

    // The only thing we do in the send callback is unblock the
    // other thread which blocks after posting data to the MAC


    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    xTaskNotifyGive(pjon_task_h);
    return;
}

static esp_err_t add_peer(uint8_t mac_addr[ESP_NOW_ETH_ALEN]) {

    if (esp_now_is_peer_exist(mac_addr)){
        return ESP_OK;
    }

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = (esp_now_peer_info_t *) malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);
    return ESP_OK;

}


static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {
    espnow_event_t evt;
    espnow_packet_t *recv_cb = &evt.info.recv_cb;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = (uint8_t *) malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

/* task that handles the recv callback */
static void espnow_recv_task(void *pvParameter) {
    espnow_event_t evt;

    while (xQueueReceive(espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        switch (evt.id) {
            case ESPNOW_RECV_CB: {
                /* Notification that incoming data has been received by the MAC layer */

                espnow_packet_t *recv_cb = &evt.info.recv_cb;

                /* Wait until we're unblocked by the recv function */
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                //Update last mac received from
                memcpy(last_mac, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
                espnow_received = recv_cb;

                break;
            }
            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;
        }
    }
}

static esp_err_t espnow_init() {

    if (espnow_queue != NULL) {
        return ESP_FAIL;
    }

    espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
    if (espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *) CONFIG_ESPNOW_PMK));

    /* Add broadcast peer information to peer list. */
    add_peer(espnow_broadcast_mac);

    /* create the task */
    xTaskCreate(espnow_recv_task, "espnow_recv_task", 1024, NULL, 4, &espnow_recv_task_h);

    return ESP_OK;
}


static void clear_received() {
    if (espnow_received != NULL) {
        free(espnow_received->data);
        espnow_received = NULL;
    }
    /* Unblock the receive function */
    xTaskNotifyGive(espnow_recv_task_h);
}



class ENHelper {

    uint32_t _magic_header;

public:

    void add_node_mac(uint8_t mac_addr[ESP_NOW_ETH_ALEN]) {
        ESP_ERROR_CHECK(add_peer(mac_addr));
    }

    bool begin() {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        pjon_task_h = xTaskGetCurrentTaskHandle();

        ESP_ERROR_CHECK(espnow_wifi_init());
        ESP_ERROR_CHECK(espnow_init());

        return true;
    }

    uint16_t receive_string(uint8_t *string, uint16_t max_length) {

        // see if there's any received data waiting

        if (espnow_received == NULL) {
            clear_received();
//            ESP_LOGD(TAG, "No data");
            return PJON_FAIL; //no data waiting
        }

        if (espnow_received->data_len > 0) {
            uint32_t header = 0;
            uint16_t len;
            memcpy((uint8_t * )(&header), espnow_received->data, 4);

            if (header != _magic_header) {
                clear_received();
                ESP_LOGE(TAG, "magic mismatch");
                return PJON_FAIL; // Not an expected packet
            }
            len = espnow_received->data_len - 4;

            if (len >  max_length) {
                ESP_LOGE(TAG, "buffer overflow - %d bytes but max is %d", len, max_length);
                clear_received();
                return PJON_FAIL;
            }

            memcpy(string, espnow_received->data + 4, len);
            clear_received();
            return len;
        }
        clear_received();
        return PJON_FAIL;
    }

    void send_string(uint8_t *string, uint16_t length, uint8_t dest_mac[ESP_NOW_ETH_ALEN]) {
        uint8_t packet[ESPNOW_MAX_PACKET];

        if (length + 4 > ESPNOW_MAX_PACKET) {
            ESP_LOGE(TAG, "Packet send error - too long");
            return;
        }

        memcpy(packet, (uint8_t*)(&_magic_header), 4);
        memcpy(packet+4, string, length);

        if (esp_now_send(dest_mac, packet, length+4) != ESP_OK) {
            ESP_LOGE(TAG, "Send error");
        } else {
            // Wait for notification that the data has been received by the MAC
//            ESP_LOGI(TAG,"waiting for send to be confirmed");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//            vTaskDelay(2/portTICK_RATE_MS);
//            ulTaskNotifyTake(pdTRUE, 5 / portTICK_RATE_MS);
        }
    }

    void send_response(uint8_t response) {
        send_string(&response, 1, last_mac);
    }

    void send_string(uint8_t *string, uint16_t length) {
        // Broadcast
        send_string(string, length, espnow_broadcast_mac);
    }

    void set_magic_header(uint32_t magic_header) { _magic_header = magic_header; }

    void get_sender(uint8_t *ip) {
        memcpy(ip, last_mac, ESP_NOW_ETH_ALEN);
    }
};

