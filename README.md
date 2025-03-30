# MicroPIC Energy Meter

Monitoriza en tiempo real el consumo eléctrico de tu vivienda y evita que salte el limitador de potencia de tu compañía distribuidora.  
Compatible con Home Assistant mediante MQTT y configurable directamente desde la pantalla.


## 📦 Características principales

- Visualización en tiempo real de consumo y producción solar
- Medidor de carga del limitador con alerta por LED
- Configuración desde la interfaz: idioma, Wi-Fi, MQTT, potencia contratada y umbrales
- Dos modos de visualización histórica: últimas 24 horas y acumulado del día
- Compatible con **Arduino**
- Basado en pantalla LCD de 2,9" de **Elecrow** y microcontrolador **ESP32**

## 📷 Vistas del sistema

- Pantalla principal: Aguja de potencia actual, indicador LED de aviso
- Histograma de las últimas 24 h
- Gráfico del acumulado del día
- Menú de configuración accesible desde los pulsadores físicos

## 🧰 Requisitos

- ESP32 (incluido en la pantalla Elecrow 2.9")
- Pantalla LCD Elecrow 2.9” con botones integrados
- Home Assistant funcionando con sensores de consumo y producción
- Broker MQTT (por ejemplo, Mosquitto)

## 🔌 Conexiones hardware

- Alimentación 5 V al ESP32 (USB o GPIO)
- Conexión Wi-Fi estable
- Broker MQTT accesible desde la red local

## 🛠️ Instalación

### Arduino

1. Clona este repositorio y abre el sketch en el IDE de Arduino
2. Instala las dependencias necesarias (ver sección siguiente)
3. Compila y sube al ESP32
4. Configura los parámetros de red y MQTT desde la pantalla

## 📡 Home Assistant

Este proyecto incluye un complemento para Home Assistant que:

- Lee las entidades de consumo y producción
- Calcula los datos necesarios
- Los envía por MQTT al dispositivo cada X segundos

Puedes configurar fácilmente:

- Entidad de consumo (`sensor.power_consumption`)
- Entidad de producción (`sensor.solar_generation`)
- Frecuencia de envío
- Topic MQTT de destino

