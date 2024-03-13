#include <Arduino.h>
#include <ArduinoOTA.h>
#include "freertos/FreeRTOS.h"



// Variables for User
#define ProbePin 34


// Variables
static const BaseType_t procpu = 0;
static const BaseType_t appcpu = 1;

hw_timer_t *timer_sample_rate = NULL;
TaskHandle_t serial_reader_taskhandle = NULL, sampler_serial_writer_taskhandle = NULL;
// portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

// Functions
void IRAM_ATTR sample_request();
void serial_reader(void *param);
void sampler_serial_writer(void *param);


void setup()
{
  // Set pin funtion
  pinMode(ProbePin, INPUT);

  Serial.begin(921600);
  // 500 ms of delay
  vTaskDelay(50 / portTICK_PERIOD_MS);

  // The hardware timer for constant sample rate
  timer_sample_rate = timerBegin(1, 80, pdTRUE);
  timerAttachInterrupt(timer_sample_rate, &sample_request, pdTRUE);
  timerAlarmWrite(timer_sample_rate, 500, pdTRUE);

  // Task to Sample Data
  xTaskCreatePinnedToCore(sampler_serial_writer, "Sampler and Serial Writer", 4000, NULL, 1, &sampler_serial_writer_taskhandle, appcpu);
  vTaskSuspend(sampler_serial_writer_taskhandle);

  // Task to Read Command
  xTaskCreatePinnedToCore(serial_reader, "Serial Reader", 4000, NULL, 1, &serial_reader_taskhandle, appcpu);
}
void loop()
{
  // This will be deleted.
  vTaskDelete(NULL);
}

// Tasks and Functions defined:
void IRAM_ATTR sample_request()
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Gives notification to the Sampler Task
  vTaskNotifyGiveFromISR(sampler_serial_writer_taskhandle, &xHigherPriorityTaskWoken);

  // Context Switching
  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}

void serial_reader(void *param)
{
  while (1)
  {
    if (Serial.available())
    {
      String command_input = Serial.readStringUntil('\n');
      // We'll be checking the Command.
      // => Expected values = START/STOP

      if (command_input == "START")
      {
        vTaskResume(sampler_serial_writer_taskhandle);
        // Resume the sampler_serial_writer
        timerAlarmEnable(timer_sample_rate);
        // Start Hardware Interrupt
      }
      else if (command_input == "STOP")
      {
        vTaskSuspend(sampler_serial_writer_taskhandle);
        // Suspend the sampler_serial_writer
        timerAlarmDisable(timer_sample_rate);
        // Stop Hardware Interrupt
      }
    }

    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}

void IRAM_ATTR sampler_serial_writer(void *param)
{
  static uint16_t data;
  // Static local variable as continuous data sampling is required.

  while (1)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // pdTrue clears the notification count

    data = analogRead(ProbePin);
    Serial.println(data);
  }
}
