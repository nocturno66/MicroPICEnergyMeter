#include <Arduino.h>
#include "display_utils.h"
#include "secrets.h"
#include <Preferences.h>

// Instancia de Preferences
Preferences preferences;

int ciclos = 0;
extern unsigned int display;
extern int hora_actual;
extern int alerta_capacidad;

// Variables a almacenar
unsigned int v_potencia_contratada = 0;
unsigned int v_produccion_max = 0;
unsigned int v_capacidad_min = 0;
unsigned int v_consumo_max = 0;
extern Language currentLanguage ;

// Claves para cada variable
#define KEY_POTENCIA "potencia"
#define KEY_PRODUCCION "produccion"
#define KEY_CAPACIDAD "capacidad"
#define KEY_IDIOMA "idioma"

// Función para inicializar la NVS
void initNVS() {
    if (!preferences.begin("config", false)) {
        Serial.println("Error al inicializar la NVS");
    } else {
        Serial.println("NVS inicializada correctamente");
    }
}

// Función para leer las variables de la NVS
void readFromNVS() {
    v_potencia_contratada = preferences.getUInt(KEY_POTENCIA, v_potencia_contratada);
    v_produccion_max = preferences.getUInt(KEY_PRODUCCION, v_produccion_max);
    v_capacidad_min = preferences.getUInt(KEY_CAPACIDAD, v_capacidad_min);
    // leer el idioma de la NVS
    currentLanguage = (Language)preferences.getUInt(KEY_IDIOMA, currentLanguage);

    Serial.println("Valores leídos de la NVS:");
    Serial.printf("Potencia contratada: %u\n", v_potencia_contratada);
    Serial.printf("Producción máxima: %u\n", v_produccion_max);
    Serial.printf("Capacidad mínima: %u\n", v_capacidad_min);
    Serial.printf("Idioma: %u\n", currentLanguage);
}

// Función para guardar las variables en la NVS
void saveToNVS() {
    preferences.putUInt(KEY_POTENCIA, v_potencia_contratada);
    preferences.putUInt(KEY_PRODUCCION, v_produccion_max);
    preferences.putUInt(KEY_CAPACIDAD, v_capacidad_min);
    // guardar el idioma en la NVS
    preferences.putUInt(KEY_IDIOMA, currentLanguage);
    Serial.println("Valores guardados en la NVS");
}

// Función para cerrar la NVS al finalizar
void closeNVS() {
    preferences.end();
    Serial.println("NVS cerrada");
}

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
    simbolo_mqtt(0);
        // Conectar a la red WiFi
    WiFi.begin(ssid, password);
    Serial.println("Conectando a WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        //Serial.print(".");
        Serial.print(estado);
        simbolo_wifi(estado);
        EPD_DisplayImage(ImageBW);
        EPD_PartUpdate();
        estado=!estado;
    }
    Serial.println("\nConexión WiFi establecida");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    // Inicializar MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);

    // Inicializar NVS
    initNVS();

    // Leer valores de NVS
    readFromNVS();
    v_consumo_max = v_produccion_max + v_potencia_contratada;

    // Modificar valores como ejemplo
    //v_potencia_contratada = 4600;
    //v_produccion_max = 5000;
    //v_capacidad_min = 1000;
    
    // imprimir los valores de las 3 variables en el debug
    Serial.printf("Potencia contratada: %u\n", v_potencia_contratada);
    Serial.printf("Producción máxima: %u\n", v_produccion_max);
    Serial.printf("Capacidad mínima: %u\n", v_capacidad_min);

        // Guardar los valores actualizados en NVS
    //saveToNVS();

    // Cerrar la NVS si ya no la necesitas
    closeNVS();
    UpdateLanguageTexts();
    display++;
}
void loop() {
    static unsigned long lastPressTime = 0;
    static bool longPressDetected = false;

    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop(); // Procesar mensajes entrantes de MQTT

    if (digitalRead(PIN_MENU) == 0) {
        unsigned long currentPressTime = millis();

        if (lastPressTime == 0) {
            // Primera detección de pulsación
            lastPressTime = currentPressTime;
        } else if (!longPressDetected && (currentPressTime - lastPressTime > 2000)) {
            // Detecta pulsación larga (> 2 segundos)
            display = 4;
            Serial.println("Display tipo: 4 (pulsación larga)");
            actualiza_display();
            longPressDetected = true;
        }
    } else {
        if (lastPressTime != 0 && !longPressDetected) {
            // Detecta pulsación corta
            display++;
            if (display > 3) {
                display = 1;
            }
            Serial.print("Display tipo: ");
            Serial.println(display);
            actualiza_display();
        }
        // Restablece el estado
        lastPressTime = 0;
        longPressDetected = false;
    }

    if (alerta_capacidad) {
        if (ciclos == 20) {
            invierte_circulo(249, 81, 44);

            EPD_DisplayImage(ImageBW);
            EPD_PartUpdate();
            Serial.print("@");
            ciclos = 0;
        }
        ciclos++;
    }
    delay(10);
}
