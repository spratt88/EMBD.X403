#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "esp_ota_ops.h"

#include "extflash.h"
#include "filesystem.h"

#define BUFFER_SIZE 256

#define TAG "OTA"
SemaphoreHandle_t ota_semaphore;

typedef struct binary_data_t {
    unsigned long size;
    unsigned long remaining_size;
    void *data;
} binary_data_t;


esp_err_t validate_image_header(const esp_partition_t *update)
{
  const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
  const esp_partition_t *running_partition = esp_ota_get_running_partition();
  esp_app_desc_t running_desc;
  esp_app_desc_t update_desc;
  esp_err_t err;

  if (boot_partition != running_partition) {
    ESP_LOGW(TAG, "Configured OTA boot partition at address 0x%08lx, but running from address 0x%08lx", 
    boot_partition->address, running_partition->address);
    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  ESP_LOGI(TAG, "Running partition %s (address 0x%08lx) type %d subtype %d", 
  running_partition->label, running_partition->address, running_partition->type, running_partition->subtype);

  if ((err = esp_ota_get_partition_description(running_partition, &running_desc)) == ESP_OK) {
    ESP_LOGI(TAG, "Running app name %s version %s (compiled %s %s with idf %s)", 
    running_desc.project_name, running_desc.version, running_desc.date, running_desc.time, running_desc.idf_ver);
  } else {
    ESP_LOGW(TAG, "Unable to query running app description");
  }
  ESP_LOGI(TAG, "Writing to partition %s (address 0x%08lx) type %d subtype %d", 
  update->label, update->address, update->type, update->subtype);
  
  if ((err = esp_ota_get_partition_description(update, &update_desc)) == ESP_OK) {
    ESP_LOGI(TAG, "Overwriting app name %s version %s (compiled %s%s with idf %s)", 
    update_desc.project_name, update_desc.version, update_desc.date, update_desc.time, update_desc.idf_ver);
  } else {
    ESP_LOGI(TAG, "Overwriting app not valid (blank region or corrupted)");
  }

  if (strcmp(running_desc.version, update_desc.version) == 0)
  {
    ESP_LOGW(TAG, "NEW VERSION IS THE SAME AS CURRENT VERSION. ABORTING");
    return ESP_FAIL;
  }
  return ESP_OK;
}

size_t fpread(void *buffer, size_t size, size_t nitems, size_t offset, FILE *fp) {
    if (fseek(fp, offset, SEEK_SET) != 0)
        return 0;
    return fread(buffer, size, nitems, fp);
}


void try_update() {
    esp_ota_handle_t update_handle = NULL;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    ESP_LOGI(TAG, "esp_begin result = %d, handle = %ld", err, update_handle);

    binary_data_t data;
    FILE *file = fopen(MOUNT_POINT"/update.bin", "rb");
    if (file == NULL) {
      ESP_LOGE(TAG, "file not found");
      return;
    }
    //Get file length
    fseek(file, 0, SEEK_END);
    data.size = ftell(file);
    data.remaining_size = data.size;
    fseek(file, 0, SEEK_SET);
    ESP_LOGI(TAG, "size %lu", data.size);
    data.data = (char *) malloc(BUFFER_SIZE);
    while (data.remaining_size > 0) {
        size_t size = data.remaining_size <= BUFFER_SIZE ? data.remaining_size : BUFFER_SIZE;
        fpread(data.data, size, 1,
               data.size - data.remaining_size, file);
        err = esp_ota_write(update_handle, data.data, size);
        if (data.remaining_size <= BUFFER_SIZE) {
            break;
        }
        data.remaining_size -= BUFFER_SIZE;
    }

    if (validate_image_header(update_partition) != ESP_OK)
    {
      ESP_LOGE(TAG, "validate_image_header failed");
      esp_ota_mark_app_valid_cancel_rollback();
      esp_ota_abort(update_handle);
      return;
    }

    ESP_LOGI(TAG, "Ota result = %d", err);
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}

void run_ota(void *params)
{
  ESP_ERROR_CHECK(nvs_flash_init());
  //ESP_ERROR_CHECK(esp_netif_init());
  //ESP_ERROR_CHECK(esp_event_loop_create_default());

  while( true ){
    xSemaphoreTake(ota_semaphore, portMAX_DELAY);
    ESP_LOGI(TAG, "Invoking OTA");

    try_update();
  }
}

void on_button_pushed(void *params)
{
  xSemaphoreGiveFromISR(ota_semaphore, pdFALSE);
}

void app_main(void)
{
  printf("hay I'm a new feature\n");
  const esp_partition_t *running_partition = esp_ota_get_running_partition();
  esp_app_desc_t running_partition_description;
  esp_ota_get_partition_description(running_partition, &running_partition_description);
  printf("current firmware version is: %s\n", running_partition_description.version);

  // file system
  if( filesystem_init( true )){
      ESP_LOGE( TAG, "error file system init" );
  }

  // button zero is the startup button on the board
  gpio_config_t gpioConfig = {
      .pin_bit_mask = 1ULL << GPIO_NUM_0,
      .mode = GPIO_MODE_DEF_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLUP_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE};

  gpio_config( &gpioConfig );
  gpio_install_isr_service( 0 );
  gpio_isr_handler_add( GPIO_NUM_0, on_button_pushed, NULL );

  ota_semaphore = xSemaphoreCreateBinary();
  xTaskCreate(run_ota, "run_ota", 1024 * 8, NULL, 2, NULL);
}
