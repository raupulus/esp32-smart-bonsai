#include <Arduino.h>
#include "WiFi.h"

// Declaro los pines analógicos.
const int analog1Pin = 36;
const int analog2Pin = 39;
const int analog3Pin = 34;
const int analog4Pin = 35;
const int analog5Pin = 32;
const int analog6Pin = 33;

// Declaro funciones para almacenar el último valor de los pines.
float analog1LastValue = 0;
float analog2LastValue = 0;
float analog3LastValue = 0;
float analog4LastValue = 0;
float analog5LastValue = 0;
float analog6LastValue = 0;

// Datos del Wireless
const char AP_NAME = 'wireless_ap_name';
const char AP_PASSWORD = 'mi_password';

void setup() {
  // Abro el puerto serial.
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
  adcAttachPin(analog1Pin);
  adcAttachPin(analog2Pin);
  adcAttachPin(analog3Pin);
  adcAttachPin(analog4Pin);
  adcAttachPin(analog5Pin);
  adcAttachPin(analog6Pin);

  /*
  * Start ADC conversion on attached pin's bus
  * */
  adcStart(analog1Pin);
  adcStart(analog2Pin);
  adcStart(analog3Pin);
  adcStart(analog4Pin);
  adcStart(analog5Pin);
  adcStart(analog6Pin);

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

  // Conectando al wifi
  WiFi.begin(AP_NAME, AP_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando al WiFi..");
  }
  
  Serial.println("Conectado al Wifi con éxito");
}

/**
 * Lee todos los sensores analógicos y los almacena.
 */ 
void readAnalogicSensors() {
  analog1LastValue = analogRead(analog1Pin);
  delay(100);
  analog2LastValue = analogRead(analog2Pin);
  delay(100);
  analog3LastValue = analogRead(analog3Pin);
  delay(100);
  analog4LastValue = analogRead(analog4Pin);
  delay(100);
  analog5LastValue = analogRead(analog5Pin);
  delay(100);
  analog6LastValue = analogRead(analog6Pin);
  delay(100);
}

/**
 * Imprime los datos de las lecturas por serial.
 */ 
void printResumeBySerial() {
  Serial.println(analog1LastValue);
  delay(100);
  Serial.println(analog2LastValue);
  delay(100);
  Serial.println(analog3LastValue);
  delay(100);
  Serial.println(analog4LastValue);
  delay(100);
  Serial.println(analog5LastValue);
  delay(100);
  Serial.println(analog6LastValue);
  delay(100);
}

/**
 * Imprime los datos de las lecturas por la pantalla externa.
 */ 
void printResumeByDisplay() {

}

void uploadDataToApi() {

}

void loop() {
  // Leo todos los pines analógicos.
  readAnalogicSensors();
  
  // Muestro los datos por Serial.
  printResumeBySerial();

  // Muestro los datos por pantalla.
  printResumeByDisplay();
}