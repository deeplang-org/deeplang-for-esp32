/* 
Author: chinesebear
Email: swubear@163.com
Website: http://chinesebear.github.io
Date: 2020/11/3
Description: deepvm is a language VM for deeplang(www.deeplang.org).
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_spiffs.h"
#include "deep_common.h"
#include "dstp.h"


#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

#define ECHO_TXD0  (GPIO_NUM_1)
#define ECHO_RXD0  (GPIO_NUM_3)

#define ECHO_TXD1  (GPIO_NUM_23)
#define ECHO_RXD1  (GPIO_NUM_22)


#define ECHO_TXD2  (GPIO_NUM_21)
#define ECHO_RXD2  (GPIO_NUM_19)

#define BUF_SIZE (1024)


static void uart0Init (void) {
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0,  ECHO_TXD0, ECHO_RXD0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
}

static void spiffsInit(void)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            deep_printf ("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            deep_printf ("Failed to find SPIFFS partition");
        } else {
            deep_printf ("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        deep_printf ("Failed to get SPIFFS partition information (%s)\r\n", esp_err_to_name(ret));
    } else {
        deep_printf ("Partition size: total: %d, used: %d\r\n", total, used);
    }
}

static void testSpiffs (void)
{
    debug ("Begin spiffs test\r\n");
    FILE* f = fopen("/spiffs/index.dp", "w");
    if (f == NULL) {
        deep_printf ("Failed to open file for writing");
        return;
    }
    const char *welcome= "welcome to deepvm";
    fwrite(welcome, 1, strlen (welcome),f);
    fclose(f);

    f = fopen("/spiffs/index.dp", "r");
    if (f == NULL) {
        deep_printf ("Failed to open file for writing");
        return;
    }
    char buf[64] = {0};
    fread (buf, 1, strlen(welcome), f);
    deep_printf ("read:%s\r\n", buf);
    fclose(f);
    debug ("End spiffs test\r\n");
}

static void deepvm_uart_process_task(void *arg)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        if(len > 0) {
            for (int i = 0; i < len; i++) {
                deep_dstp_datain (data[i]);
            }
        } else {
            vTaskDelay (10);
        }
    }
}

static void deepvm_dstp_task (void *arg) {
    while (1) {
        deep_dstp_process ();
    }
}

void app_main(void)
{
    uart0Init ();
    spiffsInit();
    testSpiffs();
    /* Print chip information */
    /* conflict with spiffs*/
    // esp_chip_info_t chip_info;
    // esp_chip_info(&chip_info);
    // deep_printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
    //             CHIP_NAME,
    //             chip_info.cores,
    //             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    //             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    // deep_printf("silicon revision %d, ", chip_info.revision);
    // deep_printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
    //             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    // deep_printf ("There are %d ms per tick\r\n",portTICK_PERIOD_MS);

    /* Deepvm start */
    deep_printf ("Deepvm for deeplang 0.1\r\n");
    deep_printf ("Deepvm includes parser, wasm vm, event manager, uart file manager\r\n");
    xTaskCreate(deepvm_uart_process_task, "deepvm_uart_process_task", 4096, NULL, 10, NULL);
    xTaskCreate(deepvm_dstp_task, "deepvm_dstp_task", 4096, NULL, 12, NULL);
}