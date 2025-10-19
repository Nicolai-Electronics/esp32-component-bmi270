// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT

#include "bmi270_interface.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include "bmi2_defs.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"

static const char* TAG = "BMI270 interface";

static i2c_master_bus_handle_t bmi270_bus       = NULL;
static uint8_t                 bmi270_address   = 0;
static SemaphoreHandle_t       bmi270_semaphore = NULL;
static i2c_master_dev_handle_t bmi270_device    = NULL;

static void claim_i2c_bus(void) {
    if (bmi270_semaphore != NULL) {
        xSemaphoreTake(bmi270_semaphore, portMAX_DELAY);
    }
}

static void release_i2c_bus(void) {
    if (bmi270_semaphore != NULL) {
        xSemaphoreGive(bmi270_semaphore);
    }
}

void bmi2_set_i2c_configuration(i2c_master_bus_handle_t bus, uint8_t address, SemaphoreHandle_t semaphore) {
    bmi270_bus       = bus;
    bmi270_address   = address;
    bmi270_semaphore = semaphore;
}

BMI2_INTF_RETURN_TYPE bmi2_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t len, void* intf_ptr) {
    if (bmi270_device == NULL) {
        ESP_LOGE(TAG, "Device is NULL");
        return BMI2_E_COM_FAIL;
    }

    // printf("I2C read: reg_addr=0x%02X, len=%" PRIu32 "\n", reg_addr, len);

    claim_i2c_bus();
    esp_err_t res = i2c_master_transmit_receive(bmi270_device, &reg_addr, 1, reg_data, len, 100);
    release_i2c_bus();

    if (res != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(res));
    }

    return (res == ESP_OK) ? BMI2_OK : BMI2_E_COM_FAIL;
}

BMI2_INTF_RETURN_TYPE bmi2_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t len, void* intf_ptr) {
    if (bmi270_device == NULL) {
        ESP_LOGE(TAG, "Device is NULL");
        return BMI2_E_COM_FAIL;
    }

    // printf("I2C write: reg_addr=0x%02X, len=%" PRIu32 "\n", reg_addr, len);

    if (len > 127) {
        ESP_LOGE(TAG, "Write length exceeds maximum of 127 bytes (%" PRIu32 ")", len);
        return BMI2_E_COM_FAIL;
    }

    uint8_t write_buffer[128] = {0};
    write_buffer[0]           = reg_addr;
    memcpy(&write_buffer[1], reg_data, len);

    claim_i2c_bus();
    esp_err_t res = i2c_master_transmit(bmi270_device, write_buffer, len + 1, 100);
    release_i2c_bus();

    if (res != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(res));
    }

    return (res == ESP_OK) ? BMI2_OK : BMI2_E_COM_FAIL;
}

BMI2_INTF_RETURN_TYPE bmi2_spi_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t len, void* intf_ptr) {
    return BMI2_E_COM_FAIL;
}

BMI2_INTF_RETURN_TYPE bmi2_spi_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t len, void* intf_ptr) {
    return BMI2_E_COM_FAIL;
}

void bmi2_delay_us(uint32_t period, void* intf_ptr) {
    ets_delay_us(period);
}

int8_t bmi2_interface_init(struct bmi2_dev* bmi, uint8_t intf) {
    if (bmi == NULL) {
        ESP_LOGE(TAG, "bmi is NULL");
        return BMI2_E_NULL_PTR;
    }
    if (intf != BMI2_I2C_INTF) {
        ESP_LOGE(TAG, "Only I2C interface is supported");
        return BMI2_E_COM_FAIL;
    }

    bmi->intf     = BMI2_I2C_INTF;
    bmi->read     = bmi2_i2c_read;
    bmi->write    = bmi2_i2c_write;
    bmi->delay_us = bmi2_delay_us;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = bmi270_address,
        .scl_speed_hz    = 400000,
    };

    esp_err_t res = i2c_master_bus_add_device(bmi270_bus, &dev_cfg, &bmi270_device);
    if (res != ESP_OK) {
        return BMI2_E_COM_FAIL;
    }

    return BMI2_OK;
}

void bmi2_error_codes_print_result(int8_t rslt) {
    if (rslt != BMI2_OK) {
        ESP_LOGE(TAG, "BMI2 error: %d", rslt);
    }
}
