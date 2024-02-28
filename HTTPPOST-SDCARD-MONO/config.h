#pragma once

#include <freertos/FreeRTOS.h>
#include <driver/i2s.h>

// sample rate for the system
#define SAMPLE_RATE 32000
#define _OPEN_SYS_ITOA_EXT
#define USE_I2S_MIC_INPUT 1

// I2S Microphone Settings
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_26
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_14
#define I2S_MIC_SERIAL_DATA GPIO_NUM_21

// sdcard
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_CLK GPIO_NUM_18
#define PIN_NUM_MOSI GPIO_NUM_23
#define PIN_NUM_CS GPIO_NUM_5

// i2s config for using the internal ADC
extern i2s_config_t i2s_adc_config;
// i2s config for reading from of I2S
extern i2s_config_t i2s_mic_Config;
// i2s microphone pins
extern i2s_pin_config_t i2s_mic_pins;
// i2s speaker pins
extern i2s_pin_config_t i2s_speaker_pins;
