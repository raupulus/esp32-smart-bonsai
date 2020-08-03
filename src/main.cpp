#include <Arduino.h>
#include "WiFi.h"
#include "DHT.h"

#include <Wire.h>
#include "Adafruit_VEML6070.h"
#include <driver/i2c.h>

// Pantalla OLED ssd1306
#include <Adafruit_SSD1306.h>


// Pin usado para el sensor de temperatura y humedad DHT.
#define DHTPIN 18
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Declaro variables de sensores.
float temperature = 0.0;
float humidity = 0.0;
float uv_quantity = 0.0;
float uv_index = 0.0;

// Declaro los pines analógicos.
const int analog1Pin = 36;
const int analog2Pin = 39;
const int analog3Pin = 34;
const int analog4Pin = 35;
const int analog5Pin = 32;
const int analog6Pin = 33;

// Declaro funciones para almacenar el último valor de los pines analógicos.
float analog1LastValue = 0;
float analog2LastValue = 0;
float analog3LastValue = 0;
float analog4LastValue = 0;
float analog5LastValue = 0;
float analog6LastValue = 0;

// Declaro los pines digitales
const int WATER_PUMP = 13;  // Bomba de agua
const int VAPORIZER = 15;   // Vaporizador de agua

// Datos del Wireless
const char* AP_NAME = "wireless_ap_name";
const char* AP_PASSWORD = "mi_password";

// Instancio sensor para rayos UV
Adafruit_VEML6070 uv = Adafruit_VEML6070();

// Instancio pantalla ssd1306
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };



// Pines para i2c
const int I2C_SDA_PIN = 19;
const int I2C_SCL_PIN = 23;



void setup() {
  // Abro el puerto serial.
  Serial.begin(115200);

  // Establezco salida i2c personalizada.
  Wire.begin( I2C_SDA_PIN, I2C_SCL_PIN, 1000000 );

  display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT);

  /*
  * Set the resolution of analogRead return values. Default is 12 bits (range from 0 to 4096).
  * If between 9 and 12, it will equal the set hardware resolution, else value will be shifted.
  * Range is 1 - 16
  *
  * Note: compatibility with Arduino SAM
  */
  analogReadResolution(10);

  /*
  * Sets the sample bits and read resolution
  * Default is 12bit (0 - 4095)
  * Range is 9 - 12
  * */
  analogSetWidth(10);

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

  // Establezco atenuación de 1,1v para los sensores chirp 1.2
  //analogSetPinAttenuation(analog1Pin, ADC_0db);
  //analogSetPinAttenuation(analog2Pin, ADC_0db);
  //analogSetPinAttenuation(analog3Pin, ADC_0db);

  // Establezco atenuación para el resto de los sensores a 3,9v
  //analogSetPinAttenuation(analog4Pin, ADC_11db);
  //analogSetPinAttenuation(analog5Pin, ADC_11db);
  //analogSetPinAttenuation(analog6Pin, ADC_11db);

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

  delay(300);

  // Conectando al wifi
  Serial.println("Conectando al WiFi..");
  WiFi.begin(AP_NAME, AP_PASSWORD);

  delay(1000);
  /*
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando al WiFi..");
  }
  
  Serial.println("Conectado al Wifi con éxito");
  */

  // Configuro pines digitales
  pinMode(WATER_PUMP, OUTPUT);
  pinMode(VAPORIZER, OUTPUT);

  // Inicializo la lectura del sensor DHT11
  dht.begin();

  // Inicializo la lectura del sensor VEML6070
  uv.begin(VEML6070_1_T);


  // Inicializo pantalla oled ssd1306
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...
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
  Serial.println();
  Serial.println("----------------------");
  Serial.print("Value Analog GPIO 36 → ");
  Serial.println(analog1LastValue);
  delay(100);
  Serial.print("Value Analog GPIO 39 → ");
  Serial.println(analog2LastValue);
  delay(100);
  Serial.print("Value Analog GPIO 34 → ");
  Serial.println(analog3LastValue);
  delay(100);
  Serial.print("Value Analog GPIO 35 → ");
  Serial.println(analog4LastValue);
  delay(100);
  Serial.print("Value Analog GPIO 32 → ");
  Serial.println(analog5LastValue);
  delay(100);
  Serial.print("Value Analog GPIO 33 → ");
  Serial.println(analog6LastValue);

  // Temperatura
  Serial.print(F("Temperature → "));
  Serial.print(temperature);
  Serial.println(F("°C "));

  // Humedad
  Serial.print(F("Humidity → "));
  Serial.print(humidity);
  Serial.println(F("% "));

  // Luz - UV
  Serial.print(F("UV → "));
  Serial.println(uv_quantity);
  Serial.print(F("Index UV → "));
  Serial.println(uv_index);
  
  Serial.println("----------------------");
  Serial.println();

  delay(3000);
}

/**
 * Imprime los datos de las lecturas por la pantalla externa.
 */ 
void printResumeByDisplay() {

  Serial.println("Probando pantalla\n");

  int16_t i;

  display.clearDisplay(); // Clear display buffer

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, SSD1306_WHITE);
    display.display(); // Update screen with each newly-drawn line
    delay(1);
  }
  for(i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000); // Pause for 2 seconds
}

void uploadDataToApi() {
  // Compruebo si está conectado a la red antes de iniciar la subida.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Iniciando subida a la API");
    // TODO → Implementar acciones de subida a la API
  } else {  
    Serial.println("No se ha conectado al WIFI, no se inicia la subida a la API");
  }
}

/**
 * Enciende el motor de riego solo cuando el sensor no detecta humedad.
 */ 
void waterPump() {

  // TODO → Implementar umbral de riego en porcentaje

  if (analog1LastValue < 1000) {
    digitalWrite(WATER_PUMP, HIGH);
    Serial.println("Encendiendo motor de riego");
  } else {
    digitalWrite(WATER_PUMP, LOW);
    Serial.println("Motor de riego apagado");
  }
}

/**
 * Enciende el vaporizador de agua cuando se dan las condiciones necesarias.
 */ 
void vaporizer() {

  if ((humidity < 65) && (temperature < 30)) {
    delay(100);
    digitalWrite(VAPORIZER, HIGH);
    Serial.println("El vaporizador está apagado");
  } else {
    delay(100);
    Serial.println("El vaporizador está encendido");
    digitalWrite(VAPORIZER, LOW);
  }
}

/**
 * Obtiene la temperatura del sensor DHT11y la asocia en la variable.
 */
void readTemperature() {
  delay(250);
  float get_temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(get_temperature)) {
    Serial.println(F("Fallo al leer temperatura del sensor DHT11!"));
    return;
  }

  temperature = get_temperature;
}

/**
 * Obtiene la humedad del sensor DHT11 y la asocia en la variable.
 */
void readHumidity() {
  delay(250);

  float get_humidity = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(get_humidity)) {
    Serial.println(F("Fallo al leer humedad del sensor DHT11!"));
    return;
  }

  humidity = get_humidity;
}

void readLight() {
  Serial.print("UV light level: "); 
  Serial.println(uv.readUV());
  uv_quantity = uv.readUV();
}

void readClock() {

}

void setClock() {
  
}

void scanI2cSensors() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  delay(2000);
}

void loop() {
  // Establezco el reloj por i2c en su fecha y hora correcta.
  setClock();
  
  // Leo y almaceno timestamp de la lectura actual
  readClock();

  // Leo todos los pines analógicos.
  readAnalogicSensors();

  // Leo pines digitales
  readTemperature();
  readHumidity();
  readLight();

  // Compruebo si necesita regar.
  waterPump();

  // Compruebo si necesita encender el vaporizador.
  vaporizer();
  
  // Muestro los datos por Serial.
  printResumeBySerial();

  // Muestro los datos por pantalla.
  //printResumeByDisplay();

  // Subo los datos a la API
  //uploadDataToApi();

  // DEBUG
  //scanI2cSensors();
}