#include <Arduino.h>
#include <string>
#include <api.cpp>
#include <logos.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <Wire.h>
#include "Adafruit_VEML6070.h"
#include <driver/i2c.h>

#ifndef upload_to_api
#define upload_to_api false
#define AP_NAME ""
#define AP_PASSWORD ""
#define API_DOMAIN ""
#define API_PORT ""
#define API_PATH ""
#define API_TOKEN_BEARER ""
#define DEVICE_ID ""
#define PLANT_ID_1 ""
#define PLANT_ID_2 ""
#define PLANT_ID_3 ""
#define PLANT_ID_4 ""

#endif

// Parámetros para el modo hibernación
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
//#define TIME_TO_SLEEP  108000    /* Time ESP32 will go to sleep (in seconds) */
#define TIME_TO_SLEEP 60 /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0;

// Pantalla OLED ssd1306
#include <Adafruit_SSD1306.h>

// Importo librería para sensor BME280
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
float BME_ADDRESS = 0x76;
bool BME_ENABLED = true; // Indica si se habilita el sensor

// Instancio sensor para rayos UV
bool VEML6070_ENABLED = true;
Adafruit_VEML6070 uv = Adafruit_VEML6070();

// Instancio pantalla ssd1306
bool DISPLAY_ENABLED = true; // Indica si habilita la pantalla
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pines para i2c
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// Declaro los pines digitales
const int LED_ON = 2;         // Pin para indicar que está encendido el circuito.
const int ENERGY = 17;        // Alimenta la energía de todo el circuito de 3,3v
const int ENERGY_HIGH = 16;   // Alimenta la energía de todo el circuito de 5v
const int WATER_PUMP = 18;    // Bomba de agua
const int VAPORIZER = 19;     // Vaporizador de agua
const int SENSOR_WATER = 23;  // Sensor para el tanque de agua
const int LED_NEED_WATER = 4; // Led para avisar que se necesita agua.

// Declaro los pines analógicos para lectura de humedad del suelo.
const int analog1Pin = 39;
const int analog2Pin = 35;
const int analog3Pin = 33;
const int analog4Pin = 34;

// Variable para forzar DEBUG
const bool DEBUG = false;

// TODO → Implementar modo debug para depurar en caliente lecturas por display
bool DEBUG_HOT_MODE = false;            // Indica si se ha activado el modo debug
const int DEBUG_HOT_MODE_INIT_PIN = 27; // Pin por el que recibe la señal

// DECLARO CONSTANTES
const int THRESHOLD_VAPORIZER_AIR_HUMIDITY = 65;  // Umbral de humedad máxima para vaporizador
const int THRESHOLD_VAPORIZER_TEMPERATURE = 35;   // Umbral de Temperatura máxima para vaporizar agua
const int THRESHOLD_WATERPUMP_SOIL_HUMIDITY = 40; // Umbral de humedad en suelo para regar en %
const int DURATION_MOTOR_WATER = 6000;            // Duración del motor de riego en ms
const int THRESHOLD_SOIL_MOISURE_MAX = 3800;      // Umbral de resistencia para humedad en suelo máxima, 0%
const int THRESHOLD_SOIL_MOISURE_MIN = 2400;      // Umbral de resistencia para humedad en suelo mínima, 100%

// Declaro variables de sensores.
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0; // Presión atmosférica
float uv_quantity = 0.0;
float uv_index = 0.0;
float soil_humidity_1 = 0.0;
float soil_humidity_2 = 0.0;
float soil_humidity_3 = 0.0;
float soil_humidity_4 = 0.0;
bool waterPump_status = false; // Indica si se ha regado en esta iteración del loop.
bool vaporizer_status = false; // Indica si se ha vaporizado en esta iteración del loop.
bool full_water_tank = false;  // Indica si el tanque tiene agua.
bool need_water = false;       // Indica si es necesario regar, para encender led avisando

// Declaro funciones para almacenar el último valor de los pines analógicos.
float analog1LastValue = 0;
float analog2LastValue = 0;
float analog3LastValue = 0;
float analog4LastValue = 0;

// TODO → Bolean indicando si se ha leido bien, resetear al final del loop y subir a la api null si está mal leído

/*
void scanI2cSensors()
{
    byte error, address;
    int nDevices;
    Serial.println("Scanning...");
    nDevices = 0;
    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
            {
                Serial.print("0");
            }
            Serial.println(address, HEX);
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            if (address < 16)
            {
                Serial.print("0");
            }
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
    {
        Serial.println("No I2C devices found\n");
    }
    else
    {
        Serial.println("done\n");
    }
    delay(2000);
}
*/

/*
 * Realiza la conexión al wifi en caso de no estar conectado.
 */
void wifiConnect()
{
    if (upload_to_api && (WiFi.status() != WL_CONNECTED))
    {
        if (DEBUG || DEBUG_HOT_MODE)
        {
            Serial.println("Conectando al WiFi..");
        }

        WiFi.begin(AP_NAME, AP_PASSWORD);

        delay(500);

        if (WiFi.status() == WL_CONNECTED)
        {
            if (DEBUG || DEBUG_HOT_MODE)
            {
                Serial.println("Se ha conectado al wifi correctamente..");
            }
        }
    }
}

/**
 * Activa todo el circuito de energía.
 */
void powerOn()
{
    delay(1000);
    digitalWrite(ENERGY, HIGH);
    digitalWrite(ENERGY_HIGH, HIGH);
    delay(200);
    digitalWrite(LED_ON, HIGH);
    delay(5000);
}

/**
 * Desactiva todo el circuito de energía.
 */
void powerOff()
{
    delay(5000);
    digitalWrite(LED_ON, LOW);
    delay(200);
    digitalWrite(ENERGY, LOW);
    digitalWrite(ENERGY_HIGH, LOW);
    delay(1000);
}

void setup()
{
    // Delay para prevenir posible cuelgue al despertar de hibernación.
    delay(500);

    // Abro el puerto serial.
    Serial.begin(115200);

    // Establezco salida i2c personalizada.
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    delay(1000);

    // Configuro pines digitales
    pinMode(ENERGY, OUTPUT);
    pinMode(ENERGY_HIGH, OUTPUT);
    pinMode(LED_ON, OUTPUT);
    pinMode(WATER_PUMP, OUTPUT);
    pinMode(VAPORIZER, OUTPUT);
    pinMode(SENSOR_WATER, INPUT);
    pinMode(LED_NEED_WATER, OUTPUT);

    digitalWrite(ENERGY, HIGH);
    digitalWrite(ENERGY_HIGH, HIGH);
    digitalWrite(LED_ON, HIGH);
    digitalWrite(WATER_PUMP, LOW);
    digitalWrite(VAPORIZER, LOW);

    delay(3000);

    if (DISPLAY_ENABLED)
    {
        display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    //scanI2cSensors();

    /*
    * Set the resolution of analogRead return values. Default is 12 bits (range from 0 to 4096).
    * If between 9 and 12, it will equal the set hardware resolution, else value will be shifted.
    * Range is 1 - 16
    *
    * Note: compatibility with Arduino SAM
    */
    analogReadResolution(12);

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
    //analogSetCycles(8);

    /*
    * Set number of samples in the range - Default is 1
    * Range is 1 - 255
    * This setting splits the range into "samples" pieces, which could look
    * like the sensitivity has been multiplied that many times
    * */
    //analogSetSamples(20);

    /*
    * Set the divider for the ADC clock - Default is 1
    * Range is 1 - 255
    */
    //analogSetClockDiv(1);

    /*
    * Set the attenuation for all channels - Default is 11db
    */
    analogSetAttenuation(ADC_11db); //ADC_0db, ADC_2_5db, ADC_6db, ADC_11db

    /*
    * Set the attenuation for particular pin - Default is 11db
    */
    //analogSetPinAttenuation(36, ADC_0db); //ADC_0db, ADC_2_5db, ADC_6db, ADC_11db

    // Establezco atenuación de 1,1v para los sensores chirp 1.2
    //analogSetPinAttenuation(analog1Pin, ADC_0db);

    // Establezco atenuación para el resto de los sensores a 3,9v
    //analogSetPinAttenuation(analog4Pin, ADC_11db);

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

    /*
    * Start ADC conversion on attached pin's bus
    * */
    //adcStart(analog1Pin);
    //adcStart(analog2Pin);

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
    if (!DEBUG)
    {
        wifiConnect();
        delay(500);
    }

    // Inicializo la lectura del sensor VEML6070
    if (VEML6070_ENABLED)
    {
        Serial.println("Inicializando sensor UV VEML6070");
        uv.begin(VEML6070_1_T);

        delay(300);
    }

    // Inicializo pantalla oled ssd1306 - Address 0x3D for 128x64
    if (DISPLAY_ENABLED)
    {
        if (DEBUG || DEBUG_HOT_MODE)
        {
            Serial.println("Inicializando pantalla SSD1306");
        }

        while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        {
            Serial.println(F("SSD1306 allocation failed"));
            delay(10000);
        }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(23, 0);
        display.cp437(true); // Para activar carácteres raros en ASCII https://elcodigoascii.com.ar/
        display.drawBitmap(0, 0, logo3, 128, 64, 1);
        display.display();

        delay(2000);
    }

    // Inicializo la lectura del sensor BME280
    if (BME_ENABLED && !bme.begin(BME_ADDRESS))
    {
        if (DISPLAY_ENABLED)
        {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(23, 0);
            display.println("BME280 ERROR!!!");
            display.display();
        }

        Serial.println(F("Could not find a valid BME280 sensor, check wiring or "
                         "try a different address!"));

        delay(5000);

        while (!bme.begin(BME_ADDRESS))
        {
            Serial.println(F("Intenando conectar al Sensor BME"));
            delay(10000);
        }
    }

    delay(300);
}

/**
 * Calcula el porcentaje de humedad en la tierra.
 * A partir del umbral de resistencia máximo y mínimo, tomo estos como 
 * valores de referencia para calcular el porcentaje de agua que hay en la
 * tierra y devuelvo el porcentaje relativo dentro del rango de trabajo.
 * En seco, el sensor capacitivo puede rondar los 2000 de mínimo (no llega a 0)
 */
int calcSoilMoisure(int res)
{
    int max = THRESHOLD_SOIL_MOISURE_MAX; // Max resistencia es 4095
    int min = THRESHOLD_SOIL_MOISURE_MIN;

    int diff = (max - min);
    int prop = (diff / 100);
    int calc = 100 - ((res - min) / prop);

    if ((res <= min) || (calc >= 100))
    {
        return 100;
    }

    if ((res >= max) || (calc <= 0))
    {
        return 0;
    }

    return calc;
}

/**
 * Lee todos los sensores analógicos y los almacena.
 */
void readAnalogicSensors()
{

    delay(3000);

    // Sensor para la humedad de la tierra.
    analog1LastValue = analogRead(analog1Pin);
    soil_humidity_1 = calcSoilMoisure(analog1LastValue);

    delay(1000);

    analog2LastValue = analogRead(analog2Pin);
    soil_humidity_2 = calcSoilMoisure(analog2LastValue);

    delay(1000);

    analog3LastValue = analogRead(analog3Pin);
    soil_humidity_3 = calcSoilMoisure(analog3LastValue);

    delay(1000);

    analog4LastValue = analogRead(analog4Pin);
    soil_humidity_4 = calcSoilMoisure(analog4LastValue);

    delay(1000);
}

/**
 * Imprime los datos de las lecturas por serial.
 */
void printResumeBySerial()
{
    Serial.println();
    Serial.println("----------------------");
    Serial.print("Soil Moisure Analog 0 → ");
    Serial.println(analog1LastValue);
    delay(100);
    Serial.print("Soil Moisure Analog 1 → ");
    Serial.println(analog2LastValue);
    delay(100);
    Serial.print("Soil Moisure Analog 2 → ");
    Serial.println(analog3LastValue);
    delay(100);
    Serial.print("Soil Moisure Analog 3 → ");
    Serial.println(analog4LastValue);
    delay(100);

    // Temperatura
    Serial.print(F("Temperature → "));
    Serial.print(temperature);
    Serial.println(F("°C "));

    // Humedad
    Serial.print(F("Humidity → "));
    Serial.print(humidity);
    Serial.println(F("% "));

    // Presión Atmosférica
    Serial.print(F("Pressure → "));
    Serial.print(pressure);
    Serial.println(F("b "));

    // Luz - UV
    Serial.print(F("UV → "));
    Serial.println(uv_quantity);

    // Porcentaje de humedad en tierra
    Serial.print(F("Soil Moisure Porcent 0 → "));
    Serial.println(soil_humidity_1);
    Serial.print(F("Soil Moisure Porcent 1 → "));
    Serial.println(soil_humidity_2);
    Serial.print(F("Soil Moisure Porcent 2 → "));
    Serial.println(soil_humidity_3);

    Serial.print(F("Soil Moisure Porcent 3 → "));
    Serial.println(soil_humidity_4);

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WLAN → ON");
    }
    else
    {
        Serial.println("WLAN → OFF");
    }

    // Bomba de agua
    Serial.print(F("Bomba de agua → "));
    Serial.println(waterPump_status ? "on" : "off");

    // Vaporizador
    Serial.print(F("Vaporizador → "));
    Serial.println(vaporizer_status ? "on" : "off");

    // Tanque de agua lleno
    Serial.print(F("Tanque de agua lleno → "));
    Serial.println(full_water_tank ? "Si" : "No");

    Serial.println("----------------------");
    Serial.println();

    delay(200);
}

/**
 * Imprime los datos de las lecturas por la pantalla externa.
 */
void printResumeByDisplay()
{
    if (!DISPLAY_ENABLED)
    {
        return;
    }

    Serial.println("Mostrando datos por la pantalla\n");

    // Limpio el buffer de la pantalla.
    display.clearDisplay();

    // Muestro logotipos como animación de carga
    //display.clearDisplay();
    display.drawBitmap(0, 0, logo, 128, 64, 1);
    display.display();

    delay(150);

    display.clearDisplay();
    display.drawBitmap(0, 0, logo2, 128, 64, 1);
    display.display();

    delay(150);

    display.clearDisplay();
    display.drawBitmap(0, 0, logo3, 128, 64, 1);
    display.display();

    delay(150);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(23, 0);
    display.println("ULTIMA LECTURA");

    // Pines Analógicos
    display.print("A0: ");
    display.print((int)analog1LastValue);
    display.print(" | A1: ");
    display.println((int)analog2LastValue);
    display.print("A2: ");
    display.print((int)analog3LastValue);
    display.print(" | A3: ");
    display.println((int)analog4LastValue);

    // Temperatura
    display.print(F("Tem: "));
    display.print(temperature);
    display.print(F("C"));

    // Humedad
    display.print(F(" Hum: "));
    display.print((int)humidity);
    display.println(F("%"));

    // Luz - UV
    display.print(F("UV: "));
    display.print((int)uv_quantity);

    if (WiFi.status() == WL_CONNECTED)
    {
        display.println(" WLAN: ON");
    }
    else
    {
        display.println(" WLAN: OFF");
    }

    // Bomba de agua
    display.print(F("Water: "));
    display.print(waterPump_status ? "on" : "off");

    // Vaporizador
    display.print(F(" Vap: "));
    display.println(vaporizer_status ? "on" : "off");

    if (WiFi.status() == WL_CONNECTED)
    {
        display.print("IP: ");
        display.println(WiFi.localIP());
    }

    display.display();
}

void uploadDataToApi(String PLANT_ID, float soilMoisureRaw, int soilPorcent)
{
    if (!PLANT_ID || PLANT_ID == "")
    {
        Serial.println("No se ha configurado: " + PLANT_ID);
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Iniciando subida a la API");
        HTTPClient http;

        // Parámetros a enviar
        String params = "{\"plant_id\":" + PLANT_ID +
                        ",\"hardware_device_id\":" + (String)DEVICE_ID +
                        ",\"pressure\":" + (String)pressure +
                        ",\"uv\":" + (String)uv_quantity +
                        ",\"temperature\":" + (String)temperature +
                        ",\"humidity\":" + (String)humidity +
                        ",\"soil_humidity_raw\":" + (String)soilMoisureRaw +
                        ",\"soil_humidity\":" + (String)soilPorcent +
                        ",\"full_water_tank\":" + (String)full_water_tank +
                        ",\"waterpump_enabled\":" + (String)waterPump_status +
                        ",\"vaporizer_enabled\":" + (String)vaporizer_status +
                        "}";

        Serial.println("Parámetros json: ");
        Serial.println(params);

        //http.begin("https://api.fryntiz.dev/smartplant/register/add-json");
        http.begin((String)API_DOMAIN + ":" + (String)API_PORT + "/" + (String)API_PATH);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", API_TOKEN_BEARER);
        http.addHeader("Accept", "*/*");

        // Realiza la subida a la API
        int httpCode = http.POST(params);

        // Respuesta de la API
        auto response = http.getString();

        Serial.println("Stream:");
        Serial.println(http.getStream());
        Serial.println("Response:");
        Serial.println(response);

        Serial.println("Código de respuesta de la API: ");
        Serial.println(httpCode);

        Serial.println("Ruta de la api: ");
        Serial.println((String)API_DOMAIN + ":" + (String)API_PORT + "/" + (String)API_PATH);

        // Indica que ha terminado de transmitirse el post.
        http.end();
    }
    else
    {
        Serial.println("No se ha conectado al WIFI, no se inicia la subida a la API");
    }
}

/**
 * Comprueba si se detecta agua.
 */
void getWaterTank()
{
    full_water_tank = digitalRead(SENSOR_WATER);
}

/**
 * Enciende el motor de riego solo cuando el sensor no detecta humedad.
 */
void waterPump()
{
    // Enciende el motor cuando la humedad del suelo es menor al 35%
    if (soil_humidity_1 < THRESHOLD_WATERPUMP_SOIL_HUMIDITY)
    {

        // Enciendo led indicando que necesita agua
        need_water = true;
        digitalWrite(LED_NEED_WATER, HIGH);

        // Compruebo si hay agua para poder regar.
        if (full_water_tank)
        {
            delay(100);

            digitalWrite(WATER_PUMP, HIGH);
            Serial.println("Encendiendo motor de riego");
            waterPump_status = true;

            // Motor de riego durante 4 segundos y se detiene.
            delay(DURATION_MOTOR_WATER);
            digitalWrite(WATER_PUMP, LOW);
            Serial.println("Apagando motor de riego");

            // Apago led indicando que necesita agua
            need_water = false;
            digitalWrite(LED_NEED_WATER, LOW);
        }
        else
        {
            Serial.println("No se riega, no se detecta agua");
            digitalWrite(WATER_PUMP, LOW);
            waterPump_status = false;
        }
    }
    else
    {
        Serial.println("Motor de riego apagado");
        digitalWrite(WATER_PUMP, LOW);
        waterPump_status = false;

        // Apago led indicando que necesita agua
        need_water = false;
        digitalWrite(LED_NEED_WATER, LOW);
    }
}

/**
 * Enciende el vaporizador de agua cuando se dan las condiciones necesarias.
 */
void vaporizer()
{
    // Enciende el vaporizador cuando la humedad y temperatura no sobrepasan el umbral predefinido.
    if ((humidity < THRESHOLD_VAPORIZER_AIR_HUMIDITY) && (temperature < THRESHOLD_VAPORIZER_TEMPERATURE))
    {
        // Compruebo si hay agua para poder regar.
        if (full_water_tank)
        {
            delay(100);
            Serial.println("El vaporizador está encendido");
            digitalWrite(VAPORIZER, HIGH);
            vaporizer_status = true;

            // Vaporizador durante 10 segundos y se detiene.
            delay(10000);
            digitalWrite(VAPORIZER, LOW);
            Serial.println("Apagando vaporizador");
        }
        else
        {
            digitalWrite(VAPORIZER, LOW);
            vaporizer_status = false;
            Serial.println("No se vaporiza, no se detecta agua");
        }
    }
    else
    {
        Serial.println("El vaporizador está apagado");
        digitalWrite(VAPORIZER, LOW);
        vaporizer_status = false;
    }
}

/**
 * Obtiene la temperatura del sensor BME280 y la asocia en la variable.
 */
void readTemperature()
{
    if (!BME_ENABLED)
    {
        return;
    }

    delay(250);

    float get_temperature = bme.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(get_temperature))
    {
        Serial.println(F("Fallo al leer temperatura del sensor BME280!"));
        return;
    }

    temperature = get_temperature;
}

/**
 * Obtiene la humedad del sensor BME280 y la asocia en la variable.
 */
void readHumidity()
{
    if (!BME_ENABLED)
    {
        return;
    }

    delay(250);

    float get_humidity = bme.readHumidity();

    // Check if any reads failed and exit early (to try again).
    if (isnan(get_humidity))
    {
        Serial.println(F("Fallo al leer humedad del sensor BME280!"));
        return;
    }

    humidity = get_humidity;
}

/**
 * Obtiene la presión del sensor BME280 y la asocia en la variable.
 */
void readPressure()
{
    if (!BME_ENABLED)
    {
        return;
    }

    delay(250);

    float get_pressure = bme.readPressure() / 100;

    // Check if any reads failed and exit early (to try again).
    if (isnan(get_pressure))
    {
        Serial.println(F("Fallo al leer presión del sensor BME280!"));
        return;
    }

    pressure = get_pressure;
}

/**
 * Obtiene el índice UV desde el sensor VEML6070
 */
void readLight()
{
    if (!VEML6070_ENABLED)
    {
        if (DEBUG || DEBUG_HOT_MODE)
        {
            Serial.println(F("No está habilitado el sensor VEML6070!"));
        }

        return;
    }

    float get_uv = uv.readUV();

    // Check if any reads failed and exit early (to try again).
    if (isnan(get_uv))
    {
        if (DEBUG || DEBUG_HOT_MODE)
        {
            Serial.println(F("Fallo al leer luz del sensor VEML6070!"));
        }

        return;
    }

    uv_quantity = get_uv;
}

void loop()
{
    // Enciendo todo el circuito de corriente.
    powerOn();

    // TODO → Comprobar si hay entrada para modo debug y hacer toggle de DEBUG_HOT_MODE

    if (digitalRead(DEBUG_HOT_MODE_INIT_PIN))
    {
        DEBUG_HOT_MODE = !DEBUG_HOT_MODE;
    }

    delay(2000);

    if (need_water)
    {
        digitalWrite(LED_NEED_WATER, HIGH);
    }

    if (DEBUG || DEBUG_HOT_MODE)
    {
        Serial.println("");
        Serial.println("---------------------------------------");
        Serial.println("Comienza el loop");
    }

    if (DISPLAY_ENABLED)
    {
        display.clearDisplay();
        display.drawBitmap(0, 0, logo3, 128, 64, 1);
        display.display();
    }

    if (!DEBUG && !DEBUG_HOT_MODE)
    {
        // Compruebo si está conectado a la red Wireless
        wifiConnect();

        delay(5000);
    }

    // Leo todos los pines analógicos.
    readAnalogicSensors();

    // Leo sensores por i2c
    readTemperature();
    readHumidity();
    readPressure();
    readLight();

    // Compruebo tanque de agua.
    getWaterTank();

    // Compruebo si necesita regar.
    waterPump();

    // Compruebo tanque de agua.
    getWaterTank();

    // Compruebo si necesita encender el vaporizador.
    vaporizer();

    // Muestro los datos por pantalla.
    printResumeByDisplay();

    // Subo los datos a la API
    if (upload_to_api && !DEBUG && !DEBUG_HOT_MODE)
    {
        uploadDataToApi((String)PLANT_ID_1, analog1LastValue, soil_humidity_1);
        uploadDataToApi((String)PLANT_ID_2, analog2LastValue, soil_humidity_2);
        uploadDataToApi((String)PLANT_ID_3, analog3LastValue, soil_humidity_3);
        uploadDataToApi((String)PLANT_ID_4, analog4LastValue, soil_humidity_4);
    }

    // Habilito y establezco hibernación para ahorrar baterías.
    bootCount = bootCount + 1;

    // Muestro los datos por Serial.
    if (DEBUG || DEBUG_HOT_MODE)
    {
        printResumeBySerial();

        Serial.println("Termina el loop");
        Serial.println("---------------------------------------");
        Serial.println("");

        Serial.print("Contador de veces despierto: ");
        Serial.println(bootCount);
    }

    // Reestablezco marcas de riego.
    waterPump_status = false;
    vaporizer_status = false;
    full_water_tank = false;

    // Leo todos los pines analógicos.
    readAnalogicSensors();

    // Leo sensores por i2c
    readTemperature();
    readHumidity();
    readPressure();
    readLight();

    need_water = soil_humidity_1 < THRESHOLD_WATERPUMP_SOIL_HUMIDITY;

    if (need_water)
    {
        digitalWrite(LED_NEED_WATER, HIGH);
    }

    delay(10000);

    // Apago todo el circuito de corriente.
    powerOff();

    if (DEBUG || DEBUG_HOT_MODE)
    {
        delay(5000);
    }
    else
    {
        // Duerme el ESP32 durante el tiempo establecido
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
    }
}