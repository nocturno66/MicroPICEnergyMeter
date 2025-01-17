#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "EPD.h"
#include <PubSubClient.h>
#include "secrets.h"

extern uint8_t ImageBW[27200];

// Variables relacionadas con los datos del JSON
extern unsigned int v_produccion;
extern unsigned int v_consumo;
extern unsigned int v_capacidad;
extern String fecha;
extern String hora;

extern PubSubClient client; // Declaración de client

// Funciones

void obtenerFechaHora(String fechaCompleta, String &fecha, String &hora);

void inicializa_display();
void EPD_ShowGauge(uint8_t percentage);
void EPD_DrawBarGraph(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t bar_width, uint16_t bar_spacing, float energy_values[24], uint8_t tipo_rango);
float integraEnergia(int consumoActual,String horaActual, String horaAnterior);
void mantenerHoraAnterior(String fecha, String hora, String &horaAnterior, String &fechaAnterior);
void simbolo_wifi(int estado);
void simbolo_mqtt(int estado);
void EPD_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color, uint8_t hueco_relleno);
void rotate_point(float cx, float cy, float angle, float *x, float *y);
void Draw_Isosceles_Triangle(uint16_t x, uint16_t y, uint16_t base, uint16_t height, float rotation_angle, uint8_t color, uint8_t fill);
int point_in_triangle(float px, float py, float x1, float y1, float x2, float y2, float x3, float y3);
uint8_t EPD_GetPixel (int x, int y);
void EPD_InvertPoint (int x, int y);
void invierte_circulo(int x, int y, int r);

void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();

void actualiza_display_0();
void actualiza_display_1();
void actualiza_display_2();
void actualiza_display();

#endif // DISPLAY_UTILS_H
