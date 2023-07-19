#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_http_client.h"
#include "connect_wifi.h"
#include <cJSON.h>

static const char *TAG = "HTTP_CLIENT";

#define HTTP_RESPONSE_BUFFER_SIZE 1024

char *response_data = NULL;
size_t response_len = 0;
bool all_chunks_received = false;


void get_dog_pic(const char *json_string)
{
   
    cJSON *root = cJSON_Parse(json_string);
    char *url = malloc(100);
    url = cJSON_GetObjectItemCaseSensitive(root, "url")->valuestring;
    printf("RANDOM DOG URL: %s\n", url);
    
    cJSON_Delete(root);
    free(response_data);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Resize the buffer to fit the new chunk of data
            response_data = realloc(response_data, response_len + evt->data_len);
            memcpy(response_data + response_len, evt->data, evt->data_len);
            response_len += evt->data_len;
            break;
        case HTTP_EVENT_ON_FINISH:
            all_chunks_received = true;
            ESP_LOGI("RandomDogAPI", "Received data: %s", response_data);
            get_dog_pic(response_data);
            break;
        default:
            break;
    }
    return ESP_OK;
}


void randomdog_api_http(void *pvParameters)
{

    esp_http_client_config_t config = {
        .url = "https://random.dog/woof.json",
        .method = HTTP_METHOD_GET,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200)
        {
            ESP_LOGI(TAG, "Message sent Successfully");
        }
        else
        {
            ESP_LOGI(TAG, "Message sent Failed");
        }
    }
    else
    {
        ESP_LOGI(TAG, "Message sent Failed");
    }
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	connect_wifi();
	if (wifi_connect_status)
	{
		xTaskCreate(&randomdog_api_http, "randomdog_api_http", 8192, NULL, 6, NULL);
	}
}