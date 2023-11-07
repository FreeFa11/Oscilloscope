#include <Arduino.h>
#include "freertos/FreeRTOS.h"

#define ProbePin 36

TickType_t first_ticks, final_ticks;
  // Time counter


void ReadWrite(int n_bytes)
{
  uint16_t data[n_bytes];

  for (int i = 0; i < n_bytes; i++)
  {
    data[i] = analogRead(ProbePin);
    Serial.println(data[i]);
  }


  final_ticks = xTaskGetTickCount();

  // Final printing of the total time
  Serial.print("The total Time taken: ");
  float time_ms = (final_ticks-first_ticks)/portTICK_PERIOD_MS;
  Serial.print(time_ms);
}


void setup()
{

  pinMode(ProbePin,INPUT);
  Serial.begin(115200);

}

void loop()
{


  while (Serial.available())
  {
    first_ticks = xTaskGetTickCount();
    // Time counter got started

    String serial_input = Serial.readStringUntil('\n');
    // This should'nt take as much time as readString which has default timeout of about 1 Second

    if (serial_input.startsWith("B"))
    {
      int n_bytes = serial_input.substring(1).toInt();
      ReadWrite(n_bytes);
    } 

  }

}


