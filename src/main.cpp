#include <Arduino.h>
#include "display_utils.h"
#include "secrets.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>  
#include <main.h>  
#include "Micropic.h"


WebServer server(80);


// Instancia de Preferences
Preferences preferences;

int ciclos = 0;
extern unsigned int display;
extern int hora_actual;
extern int alerta_capacidad;
extern String mens_connecting, mens_connected, mens_failed, mens_mqtt_connecting, mens_mqtt_connected, mens_mqtt_failed;
extern String mens_abrirwifimovil, mens_navegador;

// Variables a almacenar
unsigned int v_potencia_contratada = 0;
unsigned int v_produccion_max = 0;
unsigned int v_capacidad_min = 0;
unsigned int v_consumo_max = 0;
bool connection = NO_CAMBIO;
extern Language currentLanguage ;
String ssid, password, mqtt_server, mqtt_topic, mqtt_user, mqtt_password;

// Claves para cada variable
#define KEY_POTENCIA "potencia"
#define KEY_PRODUCCION "produccion"
#define KEY_CAPACIDAD "capacidad"
#define KEY_IDIOMA "idioma"
#define KEY_SSID "ssid"
#define KEY_PASSWORD "password"
#define KEY_MQTT_SERVER "mqtt_server"
#define KEY_MQTT_TOPIC "mqtt_topic"
#define KEY_MQTT_USER "mqtt_user"
#define KEY_MQTT_PASSWORD "mqtt_password"   
#define KEY_CAMBIAR "cambiar"

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
    ssid = preferences.getString(KEY_SSID, "");
    password = preferences.getString(KEY_PASSWORD, "");
    mqtt_server = preferences.getString(KEY_MQTT_SERVER, "");
    mqtt_topic = preferences.getString(KEY_MQTT_TOPIC, "");
    mqtt_user = preferences.getString(KEY_MQTT_USER, "");
    mqtt_password = preferences.getString(KEY_MQTT_PASSWORD, "");
    connection = (Connection)preferences.getBool(KEY_CAMBIAR, connection);
}

// Función para guardar las variables en la NVS
void saveToNVS() {
    preferences.putUInt(KEY_POTENCIA, v_potencia_contratada);
    preferences.putUInt(KEY_PRODUCCION, v_produccion_max);
    preferences.putUInt(KEY_CAPACIDAD, v_capacidad_min);
    // guardar el idioma en la NVS
    preferences.putUInt(KEY_IDIOMA, currentLanguage);
    preferences.putString(KEY_SSID, ssid);
    preferences.putString(KEY_PASSWORD, password);
    preferences.putString(KEY_MQTT_SERVER, mqtt_server);
    preferences.putString(KEY_MQTT_TOPIC, mqtt_topic);
    preferences.putString(KEY_MQTT_USER, mqtt_user);
    preferences.putString(KEY_MQTT_PASSWORD, mqtt_password);
    preferences.putBool(KEY_CAMBIAR, connection);

    Serial.println("Valores guardados en la NVS");
}

// Función para cerrar la NVS al finalizar
void closeNVS() {
    preferences.end();
    Serial.println("NVS cerrada");
}



void setupWiFi() {
    int estado=1;
    


    if ((ssid.isEmpty()) || (connection == CONFIGURAR)) {
        Serial.println("No se encontraron credenciales WiFi guardadas. Iniciando modo AP...");
        setupAccessPoint();
    } else {
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.println("Conectando a WiFi...");
        EPD_ShowString(0, 112, mens_connecting.c_str(), BLACK, 12);
        EPD_DisplayImage(ImageBW);
        EPD_PartUpdate();
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(100);
            Serial.print(".");
            simbolo_wifi(estado);
            EPD_DisplayImage(ImageBW);
            EPD_PartUpdate();
            estado=!estado;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConectado a WiFi: " + ssid);
            EPD_ShowString(0, 112, mens_connected.c_str(), BLACK, 12);
            EPD_DisplayImage(ImageBW);
            EPD_PartUpdate();
        } else {
            Serial.println("\nNo se pudo conectar a WiFi. Iniciando modo AP...");
            EPD_ShowString(0, 112, mens_failed.c_str(), BLACK, 12);
            EPD_DisplayImage(ImageBW);
            EPD_PartUpdate();
            setupAccessPoint();
        }
    }

    //setupAccessPoint();
    while (WiFi.status() != WL_CONNECTED) 
        server.handleClient();
}



void setupAccessPoint() {
    String apSSID = "MicroPIC_EnergyMeter";
    String apPassword = "12345678";
    EPD_ShowString(0, 102, mens_abrirwifimovil.c_str(), BLACK, 12);
    EPD_ShowString(0, 114, mens_navegador.c_str(), BLACK, 12);
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    IPAddress IP = WiFi.softAPIP();
    Serial.println("AP iniciado");
    Serial.print("IP del AP: ");
    Serial.println(IP);

    server.on("/", HTTP_GET, []() {
        // Escanear redes WiFi disponibles
        int n = WiFi.scanNetworks();
        String wifiOptions = "";
        for (int i = 0; i < n; i++) {
            String ssid = WiFi.SSID(i);
            wifiOptions += "<option value='" + ssid + "'>" + ssid + "</option>";
        }
        String html = "<html><head><style>"
                      "body { font-family: Arial, sans-serif; font-size: 18px; margin: 0; padding: 0; text-align: center; }"
                      "h1 { background-color: #690000; color: white; padding: 20px; }"
                      "form { margin: 50px auto; width: 300px; }"
                      "input[type=text], input[type=password] { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 4px; }"
                      "input[type=submit] { background-color: #690000; color: white; padding: 10px; border: none; border-radius: 4px; cursor: pointer; }"
                      "input[type=submit]:hover { background-color: #690000; }"
                      "</style></head><body>"
                      "<h1>Configuraci&oacute;n/Setup</h1>"
                      MicroPIC_color;

        html += "' style='width:400px; height:130px; margin: 20px auto;'>";
        html += "<div style='font-size: 48px; font-weight: bold; margin: 20px auto;'>MQTT ENERGY METER</div>";
        html += "<form action='/save' method='POST'>"
            "<fieldset style='border: 1px solid #ccc; padding: 10px; margin-bottom: 20px;'>"
            "<legend style='font-weight: bold;'>WiFi</legend>"
            "SSID: <select name='ssid'>" + wifiOptions + "</select><br>"
              "Contrase&ntilde;a/Password: <input type='password' name='password' value='" + password + "'><br>"
              "</fieldset>"
              "<fieldset style='border: 1px solid #ccc; padding: 10px;'>"
              "<legend style='font-weight: bold;'>MQTT</legend>"
              "Server: <input type='text' name='mqtt_server' value='" + mqtt_server + "'><br>"
              "Topic: <input type='text' name='mqtt_topic' value='" + mqtt_topic + "'><br>"
              "User: <input type='text' name='mqtt_user' value='" + mqtt_user + "'><br>"
              "Password: <input type='password' name='mqtt_password' value='" + mqtt_password + "'><br>"
              "</fieldset>"
            "<br><br><br>"
            "<input type='submit' value='Guardar/Save'>"
            "</form></body></html>";

        server.send(200, "text/html", html);
    });

    server.on("/save", HTTP_POST, []() {
        if (server.hasArg("ssid") && server.hasArg("password")) {
            initNVS();
            ssid = server.arg("ssid");
            password = server.arg("password");
            mqtt_server = server.arg("mqtt_server");
            mqtt_topic = server.arg("mqtt_topic");
            mqtt_user = server.arg("mqtt_user");
            mqtt_password = server.arg("mqtt_password");
            connection = NO_CAMBIO;
            saveToNVS();
            closeNVS();
            server.send(200, "text/html", "<html><head><style>"
            "body { font-family: Arial, sans-serif; font-size: 18px; margin: 0; padding: 0; text-align: center; }"
            "h1 { background-color: #690000; color: white; padding: 20px; }"
            "</style></head><body>"
            "<h1>Guardado. Reiniciando el dispositivo.<br>Saved. Restarting the device.</h1>"
            "</body></html>");


            delay(1000);
            ESP.restart();
        } else {
            server.send(400, "text/html", "<html><head><style>"
            "body { font-family: Arial, sans-serif; font-size: 18px; margin: 0; padding: 0; text-align: center; }"
            "h1 { background-color: #690000; color: white; padding: 20px; }"
            "</style></head><body>"
            "<h1>Error: Faltan parámetros.<br>Error: Missing parameters.</h1>"
            "</body></html>");
        }
    });

    server.begin();
}

void setup() {
    Serial.begin(115200); // Inicializar el puerto serie
    int estado=1;
    
    inicializa_display();
    
    // Inicializar variables
    v_produccion = 0;
    v_consumo = 0;
    v_capacidad = 0;
    // Inicializar NVS
    initNVS();

    // Leer valores de NVS
    readFromNVS();
    UpdateLanguageTexts();

    Serial.print("actualiza el display");
    // Mostrar pantalla inicial
    actualiza_display();
    simbolo_mqtt(0);
    setupWiFi();
    Serial.println("\nConexión WiFi establecida");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    // Inicializar MQTT
    Serial.print("Conectando a MQTT server...");
    Serial.println(mqtt_server);
    Serial.print("Topic: ");
    Serial.println(mqtt_topic);
    EPD_ShowString(0, 112, mens_mqtt_connecting.c_str(), BLACK, 12);
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
    client.setServer(mqtt_server.c_str(), 1883);
    client.setCallback(mqttCallback);

    
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
    server.handleClient();

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
