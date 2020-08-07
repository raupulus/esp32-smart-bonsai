# esp32-smart-bonsai

Proyecto para automatizar el cuidado básico de un bonsai controlando humedad ambiente, humedad en la tierra, riego y luz recibida.

## Dependencias platformio

Las siguientes dependencias son necesarias para poder compilar el proyecto

- Adafruit_SSD1306
- Adafruit_VEML6070
- DHT11

## Hardware utilizado

- ESP32
- Módulo para reloj en tiempo real → DS1307 Tiny RTC
- Pantalla LCD → SSD1306
- Sensor UV → VEML6070
- Módulo para relés
- Sensor de temperatura y humedad → DHT11
- Sensor analógico genérico para humedad
- Sensor analógico para tierra con anticorrosión: Capacitive Soil Moisture Sensor v1.2
- Batería de movil 2,6Ah 3,7v de lition (Sony Xperia Z3 Compact)