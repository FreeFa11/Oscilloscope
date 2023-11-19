#include <Arduino.h>
#include <ArduinoOTA.h>
#include "freertos/FreeRTOS.h"


                                                                // Variables for User

#define ProbePin 34



                                                                // Variables for Programmer

static const BaseType_t procpu = 0;
static const BaseType_t appcpu = 1;

static uint8_t samples_required = 0;
hw_timer_t *timer_sample_rate = NULL;
TaskHandle_t serial_reader_taskhandle = NULL, sampler_serial_writer_taskhandle = NULL;
portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;





                                                                 // Functions

void IRAM_ATTR sample_request();
void serial_reader(void * param);
void sampler_serial_writer(void * param);


void setup()
{
  pinMode(ProbePin,INPUT);
  // Set pin funtion

  Serial.begin(921600);
  vTaskDelay(50/portTICK_PERIOD_MS);
  // 500 ms of delay

  timer_sample_rate = timerBegin(1,80,pdTRUE);
  timerAttachInterrupt(timer_sample_rate, &sample_request, pdTRUE);
  timerAlarmWrite(timer_sample_rate, 1000, pdTRUE);
  // The hardware timer tick rate is 1 us. 130 microseconds means 0.13 milliseconds of timer interval per sample.


  xTaskCreatePinnedToCore(sampler_serial_writer, "Sampler and Serial Writer", 4000, NULL, 1, &sampler_serial_writer_taskhandle, appcpu);
  vTaskSuspend(sampler_serial_writer_taskhandle);
  // This task is created to read data and send to serial port.
  xTaskCreatePinnedToCore(serial_reader, "Serial Reader", 4000, NULL, 1, &serial_reader_taskhandle, appcpu);
  // This task is created to read command from the Oscilloscope Program.

}

void loop()
{
  // This will probably be deleted.
    vTaskDelete(NULL);
}


                                                          // Tasks and Functions defined:

void IRAM_ATTR sample_request()
{
  // This indicates its time to sample new data. It is 8-bit U-Int, in case the data sampling is slower than handware timer.
  // => Expected value of this variable = 0/1

  portENTER_CRITICAL_ISR(&spinlock);
  samples_required += 1;
  portEXIT_CRITICAL_ISR(&spinlock);


}


void serial_reader(void * param)
{
  while(1)
  {
    if (Serial.available())
    {
      String command_input = Serial.readStringUntil('\n');
      // We'll be checking the Command.
      // => Expected values = START/STOP

      if (command_input == "START")
      {
        timerAlarmEnable(timer_sample_rate);
        // Start Hardware Interrupt
        vTaskResume(sampler_serial_writer_taskhandle);
        // Also resume the sampler_serial_writer
      }
      else if (command_input == "STOP")
      {
        timerAlarmDisable(timer_sample_rate);
        // Stop Hardware Interrupt
        vTaskSuspend(sampler_serial_writer_taskhandle);
        // Also suspend the sampler_serial_writer
      }
    }

    vTaskDelay(10/portTICK_PERIOD_MS);
  }
}


void sampler_serial_writer(void * param)
{
  static uint16_t data;
  // Static local variable as continuous data sampling is required.

  while(1)
  {
    vTaskDelay(1/portTICK_PERIOD_MS);

    if (samples_required)
    {
      // Serial.println(samples_required);
      data = analogRead(ProbePin);
      Serial.println(data);

      // => Expected values = 0/1
      portENTER_CRITICAL_ISR(&spinlock);
      samples_required -= 1;
      portEXIT_CRITICAL_ISR(&spinlock);
      // Everytime a new data is sampled, the number of required samples should be decreased.

    }

  }
}




