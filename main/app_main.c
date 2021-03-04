/*
HomeKit supported Night Light with ESP32 board.

Copyright (C) 2021  Darshan Prajapati

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_log.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

#include <iot_button.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

#include "night_light.h"

static const char *TAG = "Night Light";

#define NIGHT_LIGHT_TASK_PRIORITY  1
#define NIGHT_LIGHT_TASK_STACKSIZE 4 * 1024
#define NIGHT_LIGHT_TASK_NAME      "night_light"

/* Reset network credentials if button is pressed for more than 3 seconds and then released */
#define RESET_NETWORK_BUTTON_TIMEOUT        2

/* Reset to factory if button is pressed and held for more than 10 seconds */
#define RESET_TO_FACTORY_BUTTON_TIMEOUT     5

/* The button "Boot" will be used as the Reset button for the example */
#define RESET_GPIO  GPIO_NUM_0

/**
 * @brief The network reset button callback handler.
 * Useful for testing the Wi-Fi re-configuration feature of WAC2
 */
static void reset_network_handler(void* arg)
{
    hap_reset_network();
}
/**
 * @brief The factory reset button callback handler.
 */
static void reset_to_factory_handler(void* arg)
{
    hap_reset_to_factory();
}

/**
 * The Reset button  GPIO initialization function.
 * Same button will be used for resetting Wi-Fi network as well as for reset to factory based on
 * the time for which the button is pressed.
 */
static void reset_key_init(uint32_t key_gpio_pin)
{
    button_handle_t handle = iot_button_create(key_gpio_pin, BUTTON_ACTIVE_LOW);
    iot_button_add_on_release_cb(handle, RESET_NETWORK_BUTTON_TIMEOUT, reset_network_handler, NULL);
    iot_button_add_on_press_cb(handle, RESET_TO_FACTORY_BUTTON_TIMEOUT, reset_to_factory_handler, NULL);
}

/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int night_light_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
}

/*
 * An optional HomeKit Event handler which can be used to track HomeKit
 * specific events.
 */
static void night_light_hap_event_handler(void* arg, esp_event_base_t event_base, int event, void *data)
{
    switch(event) {
        case HAP_EVENT_PAIRING_STARTED :
            ESP_LOGI(TAG, "Pairing Started");
            break;
        case HAP_EVENT_PAIRING_ABORTED :
            ESP_LOGI(TAG, "Pairing Aborted");
            break;
        case HAP_EVENT_CTRL_PAIRED :
            ESP_LOGI(TAG, "Controller %s Paired. Controller count: %d",
                        (char *)data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_UNPAIRED :
            ESP_LOGI(TAG, "Controller %s Removed. Controller count: %d",
                        (char *)data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_CONNECTED :
            ESP_LOGI(TAG, "Controller %s Connected", (char *)data);
            break;
        case HAP_EVENT_CTRL_DISCONNECTED :
            ESP_LOGI(TAG, "Controller %s Disconnected", (char *)data);
            break;
        case HAP_EVENT_ACC_REBOOTING : {
            char *reason = (char *)data;
            ESP_LOGI(TAG, "Accessory Rebooting (Reason: %s)",  reason ? reason : "null");
            break;
        }
        default:
            /* Silently ignore unknown events */
            break;
    }
}

/* Callback for handling writes on the Light Bulb Service
 */
static int night_light_write(hap_write_data_t write_data[], int count,
        void *serv_priv, void *write_priv)
{
    int i, ret = HAP_SUCCESS;
    hap_write_data_t *write;

    for (i = 0; i < count; i++)
    {
        write = &write_data[i];

        /* Setting a default error value */
        *(write->status) = HAP_STATUS_VAL_INVALID;

        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON))
        {
            ESP_LOGI(TAG, "Received Write for Light %s", write->val.b ? "On" : "Off");

            if (night_light_set_on(write->val.b) == 0)
            {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_BRIGHTNESS))
        {
            ESP_LOGI(TAG, "Received Write for Light Brightness %d", write->val.i);

            if (night_light_set_brightness(write->val.i) == 0)
            {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_HUE))
        {
            ESP_LOGI(TAG, "Received Write for Light Hue %f", write->val.f);

            if (night_light_set_hue(write->val.f) == 0)
            {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_SATURATION))
        {
            ESP_LOGI(TAG, "Received Write for Light Saturation %f", write->val.f);

            if (night_light_set_saturation(write->val.f) == 0)
            {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        }
        else
        {
            *(write->status) = HAP_STATUS_RES_ABSENT;
        }
        /* If the characteristic write was successful, update it in hap core
         */
        if (*(write->status) == HAP_STATUS_SUCCESS)
        {
            hap_char_update_val(write->hc, &(write->val));
        }
        else
        {
            /* Else, set the return value appropriately to report error */
            ret = HAP_FAIL;
        }
    }

    return ret;
}

/*The main thread for handling the Fan Accessory */
static void night_light_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    /* Configure HomeKit core to make the Accessory name (and thus the WAC SSID) unique,
     * instead of the default configuration wherein only the WAC SSID is made unique.
     */
    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_NAME;
    hap_set_config(&hap_cfg);

    /* Initialize the HAP core */
    hap_init(HAP_TRANSPORT_WIFI);

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
        .name = "Night Light",
        .manufacturer = "DGP Inc.",
        .model = "NightLight01",
        .serial_num = "001122334455",
        .fw_rev = "1.0.0",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = night_light_identify,
        .cid = HAP_CID_LIGHTING,
    };
    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    if (!accessory)
    {
		ESP_LOGE(TAG, "Failed to create accessory");
		goto light_err;
    }

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Create the Light Bulb Service. Include the "name" since this is a user visible service  */
    service = hap_serv_lightbulb_create(false);

    if (!service) {
        ESP_LOGE(TAG, "Failed to create LightBulb Service");
        goto light_err;
    }

    /* Add the optional characteristic to the Light Bulb Service */
    int ret = hap_serv_add_char(service, hap_char_name_create("My Light"));
    ret |= hap_serv_add_char(service, hap_char_brightness_create(0));
    ret |= hap_serv_add_char(service, hap_char_hue_create(360));
    ret |= hap_serv_add_char(service, hap_char_saturation_create(10));

    if (ret != HAP_SUCCESS) {
        ESP_LOGE(TAG, "Failed to add optional characteristics to LightBulb");
        goto light_err;
    }
    /* Set the write callback for the service */
    hap_serv_set_write_cb(service, night_light_write);

    /* Add the Light Bulb Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    /* Register a common button for reset Wi-Fi network and reset to factory.
     */
    reset_key_init(RESET_GPIO);

    /* Initialize the Light Bulb Hardware */
    night_light_init();

    /* Query the controller count (just for information) */
    ESP_LOGI(TAG, "Accessory is paired with %d controllers",
                hap_get_paired_controller_count());

    /* TODO: Do the actual hardware initialization here */

    /* For production accessories, the setup code shouldn't be programmed on to
     * the device. Instead, the setup info, derived from the setup code must
     * be used. Use the factory_nvs_gen utility to generate this data and then
     * flash it into the factory NVS partition.
     *
     * By default, the setup ID and setup info will be read from the factory_nvs
     * Flash partition and so, is not required to set here explicitly.
     *
     * However, for testing purpose, this can be overridden by using hap_set_setup_code()
     * and hap_set_setup_id() APIs, as has been done here.
     */
#ifdef CONFIG_EXAMPLE_USE_HARDCODED_SETUP_CODE
    /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
    hap_set_setup_code(CONFIG_EXAMPLE_SETUP_CODE);
    /* Unique four character Setup Id. Default: ES32 */
    hap_set_setup_id(CONFIG_EXAMPLE_SETUP_ID);
#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, true, cfg.cid);
#else
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, false, cfg.cid);
#endif
#endif

    /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

    /* Initialize Wi-Fi */
    app_wifi_init();

    /* Register an event handler for HomeKit specific events.
     * All event handlers should be registered only after app_wifi_init()
     */
    esp_event_handler_register(HAP_EVENT, ESP_EVENT_ANY_ID, &night_light_hap_event_handler, NULL);

    /* After all the initializations are done, start the HAP core */
    hap_start();
    /* Start Wi-Fi */
    app_wifi_start(portMAX_DELAY);
    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
    vTaskDelete(NULL);

    light_err:
        hap_acc_delete(accessory);
        vTaskDelete(NULL);
}

void app_main()
{
    xTaskCreate(night_light_thread_entry, NIGHT_LIGHT_TASK_NAME, NIGHT_LIGHT_TASK_STACKSIZE, NULL, NIGHT_LIGHT_TASK_PRIORITY, NULL);
}

