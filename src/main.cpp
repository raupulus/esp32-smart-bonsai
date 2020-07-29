#include <Arduino.h>

void setup() {
  Serial.begin(115200);

  /*
  * Get ADC value for pin
  * */
  //analogRead(36);
  
  /*
  * Set the resolution of analogRead return values. Default is 12 bits (range from 0 to 4096).
  * If between 9 and 12, it will equal the set hardware resolution, else value will be shifted.
  * Range is 1 - 16
  *
  * Note: compatibility with Arduino SAM
  */
  //analogReadResolution(12);

  /*
  * Sets the sample bits and read resolution
  * Default is 12bit (0 - 4095)
  * Range is 9 - 12
  * */
  analogSetWidth(12);

  /*
  * Set number of cycles per sample
  * Default is 8 and seems to do well
  * Range is 1 - 255
  * */
  analogSetCycles(8);

  /*
  * Set number of samples in the range.
  * Default is 1
  * Range is 1 - 255
  * This setting splits the range into
  * "samples" pieces, which could look
  * like the sensitivity has been multiplied
  * that many times
  * */
  analogSetSamples(1);

  /*
  * Set the divider for the ADC clock.
  * Default is 1
  * Range is 1 - 255
  * */
  analogSetClockDiv(1);

  /*
  * Set the attenuation for all channels
  * Default is 11db
  * */
  analogSetAttenuation(ADC_11db); //ADC_0db, ADC_2_5db, ADC_6db, ADC_11db

  /*
  * Set the attenuation for particular pin
  * Default is 11db
  * */
  //analogSetPinAttenuation(36, ADC_0db); //ADC_0db, ADC_2_5db, ADC_6db, ADC_11db

  /*
  * Get value for HALL sensor (without LNA)
  * connected to pins 36(SVP) and 39(SVN)
  * */
  //hallRead();

  /*
  * Non-Blocking API (almost)
  *
  * Note: ADC conversion can run only for single pin at a time.
  *       That means that if you want to run ADC on two pins on the same bus,
  *       you need to run them one after another. Probably the best use would be
  *       to start conversion on both buses in parallel.
  * */

  /*
  * Attach pin to ADC (will also clear any other analog mode that could be on)
  * */
  adcAttachPin(36);
  adcAttachPin(39);
  adcAttachPin(34);
  adcAttachPin(35);
  adcAttachPin(32);
  adcAttachPin(33);

  /*
  * Start ADC conversion on attached pin's bus
  * */
  adcStart(36);
  adcStart(39);
  adcStart(34);
  adcStart(35);
  adcStart(32);
  adcStart(33);

  /*
  * Check if conversion on the pin's ADC bus is currently running
  * */
  //adcBusy(uint8_t pin);

  /*
  * Get the result of the conversion (will wait if it have not finished)
  * */
  //adcEnd(uint8_t pin);

  /**
  * When VDD_A is 3.3V:
  *
  * - 0dB attenuaton (ADC_ATTEN_DB_0) gives full-scale voltage 1.1V
  * - 2.5dB attenuation (ADC_ATTEN_DB_2_5) gives full-scale voltage 1.5V
  * - 6dB attenuation (ADC_ATTEN_DB_6) gives full-scale voltage 2.2V
  * - 11dB attenuation (ADC_ATTEN_DB_11) gives full-scale voltage 3.9V (see note below)
  *
  * @note The full-scale voltage is the voltage corresponding to a maximum reading (depending on ADC1 configured
  * bit width, this value is: 4095 for 12-bits, 2047 for 11-bits, 1023 for 10-bits, 511 for 9 bits.)
  *
  * @note At 11dB attenuation the maximum voltage is limited by VDD_A, not the full scale voltage.
  */  
}

void loop() {
  Serial.println(analogRead(36)); //VP
  delay(1000);
  Serial.println(analogRead(39)); //VP
  delay(1000);
  Serial.println(analogRead(34)); //VP
  delay(1000);
  Serial.println(analogRead(35)); //VP
  delay(1000);
  Serial.println(analogRead(32)); //VP
  delay(1000);
  Serial.println(analogRead(33)); //VP
  delay(1000);
}