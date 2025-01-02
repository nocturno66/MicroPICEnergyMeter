#include <Arduino.h>
#include "display_utils.h"
#include "secrets.h"

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

    

    // Inicializar la pantalla e-paper
Serial.println("Inicializando GPIO...");
EPD_GPIOInit();
Serial.println("GPIO inicializado");

Serial.println("Inicializando Fast Mode 1...");
EPD_FastMode1Init();
Serial.println("Fast Mode 1 inicializado");

Serial.println("Limpiando pantalla...");
EPD_Display_Clear();
Serial.println("Pantalla limpiada");

Serial.println("Actualizando pantalla...");
EPD_Update();
Serial.println("Pantalla actualizada");

Serial.println("Limpiando caché...");
EPD_Clear_R26A6H();
Serial.println("Caché limpiada");


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
    // Reconectar a MQTT si es necesario
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop(); // Procesar mensajes entrantes de MQTT
    delay(10);
}
