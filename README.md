# esp32-smart-bonsai

Proyecto para controlar el cuidado de varias plantas.

Controla la humedad del ambiente, humedad en tierra de cada planta, el riego y la luz recibida.

Se tomará en cuenta para regar o humidificar el ambiente los parámetros registrados en las plantas.

Actualmente solo contempla 1 planta, pero se prepara el código para en el futuro facilitar controlar más.

## Dependencias platformio

Las siguientes dependencias son necesarias para poder compilar el proyecto

- Adafruit_SSD1306
- Adafruit_VEML6070
- ArduinoJson
- Adafruit_BME280

Si la instalamos desde platformio, sería suficiente con abrir el proyecto ahí ya que están definidas en el archivo platformio.ini precisamente para evitar este inconveniente de lidiar con dependencias.

## Entorno de variables para WIFI y API

Tenemos que copiar el archivo de ejemplo **api.cpp.example** al mismo nivel
de directorio y llamarlo simplemente **api.cpp** modificando posteriormente
las variables privadas de acceso a nuestro router wireless personal y la api
que tengamos en uso.

En caso de no tener o no querer utilizarlo, establecer la variable 
**upload_to_api** en **false** y las demás variables vacías tal como están en el
archivo de ejemplo actualmente.

## Hardware utilizado

- ESP32 Lite Lolin32, versión reducida con gestión de carga para batería en puerto integrado.
- Pantalla LCD → SSD1306
- Sensor UV → VEML6070
- Sensor de temperatura, humedad ambiente y presión → Bosh BME280
- 2 Módulo para relés protegidos, uno para la bomba de agua y otro para el vaporizador de agua.
- 2 Módulos mofset para alimentar circuito de 5v y 3v.
- Sensor analógico para tierra con anticorrosión: Capacitive Soil Moisture Sensor v1.2
- Batería de teléfono, tablet o cualquiera de lition 3,7v y más de 2 amperios.
- Interruptor cisterna para el tanque de agua. Se puede puentear si no se quiere utilizar.
- Vaporizador de agua.

## Esquemas de Pines

El siguiente esquema de pines es el que utilizo por defecto para el smart bonsai.

### Pines analógicos

- 36 → Sensor de humedad en tierra (En la versión actual, solo se utiliza este)
- 39 → Sensor de humedad en tierra (No usado, planteado para futuro)
- 35 → Sensor de humedad en tierra (No usado, planteado para futuro)
- 32 → Sensor de humedad en tierra (No usado, planteado para futuro)

## Pines digitales

- 22 → Indicador LED para mostrar que está trabajando en lecturas.
- 18 → Bomba de agua.
- 5 → Vaporizador.
- 17 → Alimentación del circuito.
- 16 → Sensor para el tanque de agua

### Pines para i2c

- 19 → SDA
- 23 → SCL
