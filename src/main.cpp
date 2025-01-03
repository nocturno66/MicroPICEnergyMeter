#include <Arduino.h>
#include "display_utils.h"
#include "secrets.h"

int ciclos = 0;

void setup() {
    Serial.begin(115200); // Inicializar el puerto serie

    // Conectar a la red WiFi
    WiFi.begin(ssid, password);
    Serial.println("Conectando a WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConexión WiFi establecida");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    // Inicializar MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);

    inicializa_display();

    // Inicializar variables
    v_produccion = 0;
    v_consumo = 0;
    v_exporimpor = 0;
    v_sentido = 0;
    v_tempext = 0;
    v_tempint = 0;
    v_humedad = 0;
    v_particulas = 0;
    v_litros = 0;
    v_tempagua = 0.0;
    v_modo = 0;
    v_manual_automatico = 0;
    Serial.print("actualiza el display");
    // Mostrar pantalla inicial
    actualiza_display();

}

void loop() {
    switch (display) {
        case 0:
            // Wall Panel Home Assistant
            // Reconectar a MQTT si es necesario
            if (!client.connected()) {
                reconnectMQTT();
            }
            client.loop(); // Procesar mensajes entrantes de MQTT
            break;
        case 1:
            // OpenWeatherMap
            if (ciclos == 0) {
                ciclos = 6000;            
                actualiza_display(); // Update weather display
                js_analysis();   // Parse weather data
                actualiza_display(); // Update weather display
            } else {
                ciclos--;
            }
            break;
        case 2:
            if (ciclos == 0) {
                ciclos = 6000;                         
                actualiza_display(); // Update weather display
            } else {
                ciclos--;
            }
        default:
            break;
    }

    // el display puede tomar valores entre 0 y 2. Cuando se detecte una pulsación del pin UP subirá, y cuando se detecte una pulsación del pin DOWN bajará
    if (digitalRead(6) == 0) {
        Serial.print ("Display tipo:");
        display++;
        if (display > 2) {
            display = 2;
        }
        ciclos = 0;
        Serial.println(display);
        delay(500);
    }
    if (digitalRead(4) == 0) {
        Serial.print ("Display tipo:");
        display--;
        if (display < 0) {
            display = 0;
        }
        ciclos = 0;
        Serial.println(display);
        delay(500);
    }
    delay(10);
}