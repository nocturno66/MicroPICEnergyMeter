#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "EPD.h"
#include <PubSubClient.h>
#include "secrets.h"

#define PIN_ENABLE  7
#define PIN_EXIT    1
#define PIN_MENU    2
#define PIN_DOWN    4
#define PIN_CONF    5
#define PIN_UP      6

#define LED         8

typedef enum {
    MENU_LANGUAGE = 0,
    MENU_POWER,
    MENU_MAX_PRODUCTION,
    MENU_LIMITER,
    MENU_CONNECTION,
    MENU_EXIT,
    MENU_OPTIONS_COUNT
} MenuOption;

typedef enum {
    LANGUAGE_SPANISH = 0,
    LANGUAGE_ENGLISH
} Language;

typedef enum {
    NO_CAMBIO = 0,
    CONFIGURAR
} Connection;

extern uint8_t ImageBW[27200];

// Variables relacionadas con los datos del JSON
extern unsigned int v_produccion;
extern unsigned int v_consumo;
extern int v_limitador;
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
void invierte_pantalla();

void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void sendMQTTMessage(const char* topic, const char* message);

void display_micropic();
void display_tiempo_real();
void display_hoy();
void display_ultimas24h();
void actualiza_display();

void display_menu();
void ProcessSelectedOption();
void UpdateMenuSelection(bool &editingValue, bool &exitMenu);
void DrawMenu(MenuOption selectedOption);
void UpdateLanguageTexts();



#endif // DISPLAY_UTILS_H
