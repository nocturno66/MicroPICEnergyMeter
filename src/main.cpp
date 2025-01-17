#include <Arduino.h>
#include "display_utils.h"
#include "secrets.h"

int ciclos = 0;
extern unsigned int display;
extern int hora_actual;
extern int alerta_capacidad;

void setup() {
    Serial.begin(115200); // Inicializar el puerto serie
    int estado=1;

    inicializa_display();

    // Inicializar variables
    v_produccion = 0;
    v_consumo = 0;
    v_capacidad = 0;

    Serial.print("actualiza el display");
    // Mostrar pantalla inicial
    actualiza_display();

        // Conectar a la red WiFi
    WiFi.begin(ssid, password);
    Serial.println("Conectando a WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        simbolo_wifi(estado);
        estado=!estado;
    }
    Serial.println("\nConexión WiFi establecida");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    // Inicializar MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop(); // Procesar mensajes entrantes de MQTT

        
    if (digitalRead(2) == 0) {
        Serial.print ("Display tipo:");
        display++;
        if (display > 2) {
            display = 0;
        }
        ciclos = 0;
        Serial.println(display);
        actualiza_display();
        delay(50);
    }
    /*if (digitalRead(4) == 0) {
        hora_actual--;
        if (hora_actual < 0) {
            hora_actual = 23;
        }
        ciclos = 0;
        Serial.println(display);
        actualiza_display();
        delay(50);
    }
    if (digitalRead(6) == 0) {
        hora_actual++;
        if (hora_actual > 23) {
            hora_actual = 0;
        }
        ciclos = 0;
        Serial.println(display);
        actualiza_display();
        delay(50);
    }*/
    if (alerta_capacidad) {
        if (ciclos==20) {
            invierte_circulo(249, 81, 44);

            EPD_DisplayImage(ImageBW);
            EPD_PartUpdate();
            Serial.print("@");
            ciclos=0;
        }
        ciclos++;
    }
    delay(10);
}
