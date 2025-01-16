#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
#include <PubSubClient.h>
#include "secrets.h"

extern uint8_t ImageBW[27200];

// Variables relacionadas con los datos del JSON
extern unsigned int v_produccion;
extern unsigned int v_consumo;
extern unsigned int v_margen;
extern String fecha;
extern String hora;

extern PubSubClient client; // Declaración de client

// Funciones
void actualiza_display_0();
void actualiza_display_1();
void actualiza_display_2();
void actualiza_display();
void obtenerFechaHora(String fechaCompleta, String &fecha, String &hora);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void inicializa_display();
void EPD_ShowGauge(uint8_t percentage);
void EPD_DrawBarGraph(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t bar_width, uint16_t bar_spacing, float energy_values[24], uint8_t tipo_rango);
float integraEnergia(int consumoActual,String horaActual, String horaAnterior);
void mantenerHoraAnterior(String fecha, String hora, String &horaAnterior, String &fechaAnterior);
void simbolo_wifi(int estado);
void simbolo_mqtt(int estado);
void EPD_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color, uint8_t hueco_relleno);

#endif // DISPLAY_UTILS_H
