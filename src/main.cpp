#include <Arduino.h>
#include <string>
#include <api.cpp>
#include <logos.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <Wire.h>
#include "Adafruit_VEML6070.h"
#include <driver/i2c.h>
//#include <debug.h>

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
#define TIME_TO_SLEEP 120      /* Time ESP32 will go to sleep (in seconds) */
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
const int PIN_SOIL_MOISTURE_A0 = 39;
const int PIN_SOIL_MOISTURE_A1 = 35;
const int PIN_SOIL_MOISTURE_A2 = 33;
const int PIN_SOIL_MOISTURE_A3 = 34;

// Constante para forzar DEBUG, sin subir datos a la API ni dormir esp32
const bool DEBUG = false;
const int DEBUG_HOT_MODE_INIT_PIN = 27; // Pin por el que recibe la señal para iniciar DEBUG en funcionamiento.
bool DEBUG_HOT_MODE = false;            // Indica si se ha activado el modo debug.

// DECLARO CONSTANTES
const int THRESHOLD_VAPORIZER_AIR_HUMIDITY = 65;     // Umbral de humedad máxima para vaporizador
const int THRESHOLD_VAPORIZER_TEMPERATURE = 35;      // Umbral de Temperatura máxima para vaporizar agua
const int THRESHOLD_MIN_SOIL_MOISTURE_HUMIDITY = 40; // Umbral de humedad en suelo para regar en %
const int DURATION_MOTOR_WATER = 5000;               // Duración del motor de riego en ms
const int THRESHOLD_SOIL_MOISURE_MAX = 3900;         // Umbral de resistencia para humedad en suelo máxima, 0%
const int THRESHOLD_SOIL_MOISURE_MIN = 2100;         // Umbral de resistencia para humedad en suelo mínima, 100%

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

void debug(String message)
{
    if (DEBUG || DEBUG_HOT_MODE)
    {
        Serial.println(message);
    }
}

/*
 * Realiza la conexión al wifi en caso de no estar conectado.
 */
void wifiConnect()
{
    if (DEBUG || DEBUG_HOT_MODE)
    {
        return;
    }

    if (upload_to_api && (WiFi.status() != WL_CONNECTED))
    {
        debug("Conectando al WiFi..");

        WiFi.begin(AP_NAME, AP_PASSWORD);

        delay(500);

        if (WiFi.status() == WL_CONNECTED)
        {
            debug("Se ha conectado al wifi correctamente..");
        }
    }

    delay(2000);
}

/**
 * Activa todo el circuito de energía.
 */
void powerOn()
{
    delay(100);
    digitalWrite(ENERGY, HIGH);
    digitalWrite(ENERGY_HIGH, HIGH);
    delay(100);
    digitalWrite(LED_ON, HIGH);
    delay(1000);
}

/**
 * Desactiva todo el circuito de energía.
 */
void powerOff()
{
    delay(100);
    digitalWrite(LED_ON, LOW);
    delay(100);
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

    delay(100);

    // Configuro pines digitales
    pinMode(ENERGY, OUTPUT);
    pinMode(ENERGY_HIGH, OUTPUT);
    pinMode(LED_ON, OUTPUT);
    pinMode(WATER_PUMP, OUTPUT);
    pinMode(VAPORIZER, OUTPUT);
    pinMode(SENSOR_WATER, INPUT);
    pinMode(LED_NEED_WATER, OUTPUT);
    pinMode(DEBUG_HOT_MODE_INIT_PIN, INPUT);

    digitalWrite(ENERGY, HIGH);
    digitalWrite(ENERGY_HIGH, HIGH);
    digitalWrite(LED_ON, HIGH);
    digitalWrite(LED_NEED_WATER, HIGH);
    digitalWrite(WATER_PUMP, LOW);
    digitalWrite(VAPORIZER, LOW);

    delay(1000);

    if (DISPLAY_ENABLED)
    {
        display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    /*
    * Set the resolution of analogRead return values. Default is 12 bits (range from 0 to 4095).
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
    //analogSetPinAttenuation(PIN_SOIL_MOISTURE_A0, ADC_0db);

    // Establezco atenuación para el resto de los sensores a 3,9v
    //analogSetPinAttenuation(analog4Pin, ADC_11db);

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
    adcAttachPin(PIN_SOIL_MOISTURE_A0);
    adcAttachPin(PIN_SOIL_MOISTURE_A1);
    adcAttachPin(PIN_SOIL_MOISTURE_A2);
    adcAttachPin(PIN_SOIL_MOISTURE_A3);

    /*
    * Start ADC conversion on attached pin's bus
    * */
    //adcStart(PIN_SOIL_MOISTURE_A0);

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
    wifiConnect();

    // Inicializo la lectura del sensor VEML6070
    if (VEML6070_ENABLED)
    {
        debug("Inicializando sensor UV VEML6070");
        uv.begin(VEML6070_1_T);

        delay(300);
    }

    // Inicializo pantalla oled ssd1306 - Address 0x3D for 128x64
    if (DISPLAY_ENABLED)
    {
        debug("Inicializando pantalla SSD1306");

        while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        {
            debug("SSD1306 allocation failed");
            delay(10000);
        }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(23, 0);
        display.cp437(true); // Para activar carácteres raros en ASCII https://elcodigoascii.com.ar/
        display.drawBitmap(0, 0, logo3, 128, 64, 1);
        display.display();

        delay(1000);
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

        debug("Could not find a valid BME280 sensor, check wiring or try a different address!");

        delay(5000);

        while (!bme.begin(BME_ADDRESS))
        {
            debug("Intenando conectar al Sensor BME");
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
int calcSoilMoisture(int res)
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
    analog1LastValue = analogRead(PIN_SOIL_MOISTURE_A0);
    soil_humidity_1 = calcSoilMoisture(analog1LastValue);

    delay(1000);

    analog2LastValue = analogRead(PIN_SOIL_MOISTURE_A1);
    soil_humidity_2 = calcSoilMoisture(analog2LastValue);

    delay(1000);

    analog3LastValue = analogRead(PIN_SOIL_MOISTURE_A2);
    soil_humidity_3 = calcSoilMoisture(analog3LastValue);

    delay(1000);

    analog4LastValue = analogRead(PIN_SOIL_MOISTURE_A3);
    soil_humidity_4 = calcSoilMoisture(analog4LastValue);

    delay(1000);
}

/**
 * Imprime los datos de las lecturas por serial.
 */
void printResumeBySerial()
{
    if (!DEBUG && !DEBUG_HOT_MODE)
    {
        return;
    }

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
 * Muestra por la pantalla animación para la lectura de datos.
 */
void displayShowAnimation()
{
    display.clearDisplay();
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
}

/**
 * Imprime los datos de las lecturas por la pantalla externa.
 */
void displayShowResume()
{
    if (!DISPLAY_ENABLED)
    {
        return;
    }

    debug("Mostrando datos por la pantalla");

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

/**
 * Recibe un porcentaje de humedad en tierra y devuelve si está por encima
 * del umbral de humedad mínimo declarado para el suelo.
 */
bool getSoilMoistureNeedWater(float soil_humidity)
{
    return bool(soil_humidity < THRESHOLD_MIN_SOIL_MOISTURE_HUMIDITY);
}

void uploadDataToApi(String PLANT_ID, float soilMoisureRaw, int soilPorcent)
{
    wifiConnect();

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
                        ",\"need_water\":" + getSoilMoistureNeedWater(float(soilPorcent)) +
                        "}";

        debug("Parámetros json: ");
        debug(params);

        //http.begin("https://api.fryntiz.dev/smartplant/register/add-json");
        http.begin((String)API_DOMAIN + ":" + (String)API_PORT + "/" + (String)API_PATH);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", API_TOKEN_BEARER);
        http.addHeader("Accept", "*/*");

        // Realiza la subida a la API
        int httpCode = http.POST(params);

        // Respuesta de la API
        auto response = http.getString();

        debug("Stream:");

        if (DEBUG || DEBUG_HOT_MODE)
        {
            Serial.println(http.getStream());

            debug("Response:");
            Serial.println(response);

            debug("Código de respuesta de la API: ");
            Serial.println(httpCode);
        }

        debug("Ruta de la api: ");
        debug((String)API_DOMAIN + ":" + (String)API_PORT + "/" + (String)API_PATH);

        // Indica que ha terminado de transmitirse el post.
        http.end();
    }
    else
    {
        debug("No se ha conectado al WIFI, no se inicia la subida a la API");
    }

    delay(300);
}

/**
 * Comprueba si se detecta agua.
 */
bool getWaterTank()
{
    full_water_tank = digitalRead(SENSOR_WATER);

    return bool(full_water_tank);
}

/**
 * Enciende el motor de riego solo cuando el sensor no detecta humedad.
 */
void waterPump()
{
    // Compruebo tanque de agua.
    getWaterTank();

    // Enciende el motor cuando la humedad del suelo es menor al 35%
    if (getSoilMoistureNeedWater(soil_humidity_1))
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
    // Compruebo tanque de agua.
    getWaterTank();

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

void readAllSensors()
{
    // Leo todos los pines analógicos.
    readAnalogicSensors();

    // Leo sensores por i2c
    readTemperature();
    readHumidity();
    readPressure();
    readLight();

    // Compruebo tanque de agua.
    getWaterTank();

    need_water = getSoilMoistureNeedWater(soil_humidity_1);
}

void loop()
{
    // Comprobando si se ha pulsado iniciar en modo debug
    if (digitalRead(DEBUG_HOT_MODE_INIT_PIN))
    {
        DEBUG_HOT_MODE = true;
    }

    // Enciendo todo el circuito de corriente.
    powerOn();

    delay(2000);

    if (need_water)
    {
        digitalWrite(LED_NEED_WATER, HIGH);
    }

    debug("");
    debug("---------------------------------------");
    debug("Comienza el loop");

    // Compruebo si está conectado a la red Wireless
    wifiConnect();

    // Muestra animación indicando que se leerán los datos.
    displayShowAnimation();

    // Lee todos los sensores.
    readAllSensors();

    // Muestra resumen de las lecturas por pantalla.
    displayShowResume();

    // Riega si es necesario.
    waterPump();

    // Muestro los datos por pantalla.
    displayShowResume();

    // Vaporiza si es necesario.
    vaporizer();

    // Muestro los datos por pantalla.
    displayShowResume();

    // Subo los datos a la API
    if (upload_to_api && !DEBUG && !DEBUG_HOT_MODE)
    {
        delay(300);
        uploadDataToApi((String)PLANT_ID_1, analog1LastValue, soil_humidity_1);
        uploadDataToApi((String)PLANT_ID_2, analog2LastValue, soil_humidity_2);
        uploadDataToApi((String)PLANT_ID_3, analog3LastValue, soil_humidity_3);
        uploadDataToApi((String)PLANT_ID_4, analog4LastValue, soil_humidity_4);
    }

    // Habilito y establezco hibernación para ahorrar baterías.
    bootCount = bootCount + 1;

    // Muestra animación indicando que se leerán los datos.
    displayShowAnimation();

    // Leo de nuevo los sensores tras los eventos anteriores.
    readAllSensors();

    // Muestro los datos por pantalla.
    displayShowResume();

    // Muestro los datos por Serial si está en modo DEBUG o DEBUG_HOT_MODE.
    printResumeBySerial();

    debug("Termina el loop");
    debug("---------------------------------------");
    debug("");
    debug("Contador de veces despierto: ");

    if (DEBUG || DEBUG_HOT_MODE)
    {
        Serial.print(bootCount);
    }

    // Reestablezco marcas de riego.
    waterPump_status = false;
    vaporizer_status = false;
    full_water_tank = false;

    if (need_water)
    {
        digitalWrite(LED_NEED_WATER, HIGH);
    }

    delay(2000);

    if (DEBUG || DEBUG_HOT_MODE)
    {
        delay(5000);
    }
    else
    {
        // Apago todo el circuito de corriente.
        powerOff();

        // Duerme el ESP32 durante el tiempo establecido
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
    }
}