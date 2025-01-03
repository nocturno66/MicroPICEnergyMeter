#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
#include "fondo.h"
#include "enchufe.h"
#include "fuego.h"
#include "exportar.h"
#include "importar.h"
#include <PubSubClient.h>
#include "secrets.h"

extern uint8_t ImageBW[27200];

// Variables relacionadas con los datos del JSON
extern unsigned int v_produccion;
extern unsigned int v_consumo;
extern unsigned int v_exporimpor;
extern unsigned int v_sentido;
extern unsigned int v_tempext;
extern unsigned int v_tempint;
extern unsigned int v_humedad;
extern unsigned int v_particulas;
extern unsigned int v_litros;
extern float v_tempagua;
extern unsigned int v_modo;
extern unsigned int v_manual_automatico;
extern String fecha;
extern String hora;
extern unsigned int display;

extern PubSubClient client; // Declaración de client

// Funciones
void inicializa_display();
void actualiza_display();
void obtenerFechaHora(String fechaCompleta, String &fecha, String &hora);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
String httpGETRequest(const char* serverName);
void js_analysis();


#endif // DISPLAY_UTILS_H
