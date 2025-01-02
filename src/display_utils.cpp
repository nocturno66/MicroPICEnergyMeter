#include "display_utils.h"
#include "secrets.h"

uint8_t ImageBW[27200];

// Variables relacionadas con los datos del JSON
unsigned int v_produccion = 0;
unsigned int v_consumo = 0;
unsigned int v_exporimpor = 0;
unsigned int v_sentido = 0;
unsigned int v_tempext = 0;
unsigned int v_tempint = 0;
unsigned int v_humedad = 0;
unsigned int v_particulas = 0;
unsigned int v_litros = 0;
float v_tempagua = 0.0;
unsigned int v_modo = 0;
unsigned int v_manual_automatico = 0;
String fecha = "";
String hora = "";

WiFiClient espClient;
PubSubClient client(espClient);

void actualiza_display() {
    char buffer[40];

    Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
    Paint_Clear(WHITE);

    EPD_ShowPicture(0, 0, 792, 272, gImage_fondo, WHITE);

    if (v_modo == 0) {
        EPD_ShowPicture(625, 120, 64, 52, gImage_enchufe, WHITE);
    } else {
        EPD_ShowPicture(625, 105, 64, 83, gImage_fuego, WHITE);
    }

    if (v_sentido == 0) {
        EPD_ShowPicture(100, 170, 64, 40, gImage_exportar, WHITE);
    } else {
        EPD_ShowPicture(100, 170, 64, 40, gImage_importar, WHITE);
    }

    snprintf(buffer, sizeof(buffer), "%s", fecha.c_str());
    EPD_ShowString(10, 1, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%s", hora.c_str());
    EPD_ShowString(150, 1, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u KWh", v_produccion);
    EPD_ShowString(120, 50, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u KWh", v_consumo);
    EPD_ShowString(40, 130, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u KWh", v_exporimpor);
    EPD_ShowString(120, 220, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u`C", v_tempext);
    EPD_ShowString(385, 25, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u`C", v_tempint);
    EPD_ShowString(385, 130, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u %%", v_humedad);
    EPD_ShowString(385, 182, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u %%", v_particulas);
    EPD_ShowString(385, 220, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%u litros", v_litros);
    EPD_ShowString(650, 20, buffer, 24, BLACK);

    snprintf(buffer, sizeof(buffer), "%3.1f`C", v_tempagua);
    EPD_ShowString(650, 50, buffer, 24, BLACK);

    if (v_manual_automatico == 0) {
        EPD_ShowString(650, 80, "Manual", 24, BLACK);
    } else {
        EPD_ShowString(650, 80, "Automatico", 24, BLACK);
    }

    EPD_Display(ImageBW);
    EPD_PartUpdate();
}

void obtenerFechaHora(String fechaCompleta, String &fecha, String &hora) {
    int dia = fechaCompleta.substring(8, 10).toInt();
    int mes = fechaCompleta.substring(5, 7).toInt();
    int ano = fechaCompleta.substring(0, 4).toInt();

    int horaUTC = fechaCompleta.substring(11, 13).toInt();
    int minutos = fechaCompleta.substring(14, 16).toInt();
    int segundos = fechaCompleta.substring(17, 19).toInt();

    bool esHorarioDeVerano = false;
    if (mes > 3 && mes < 10) {
        esHorarioDeVerano = true;
    } else if (mes == 3) {
        int ultimoDomingoMarzo = 31 - (5 * ano / 4 + 4) % 7;
        if (dia > ultimoDomingoMarzo || (dia == ultimoDomingoMarzo && horaUTC >= 2)) {
            esHorarioDeVerano = true;
        }
    } else if (mes == 10) {
        int ultimoDomingoOctubre = 31 - (5 * ano / 4 + 1) % 7;
        if (dia < ultimoDomingoOctubre || (dia == ultimoDomingoOctubre && horaUTC < 2)) {
            esHorarioDeVerano = true;
        }
    }

    int offset = esHorarioDeVerano ? 2 : 1;
    horaUTC += offset;

    if (horaUTC >= 24) {
        horaUTC -= 24;
        dia++;
        if ((mes == 1 || mes == 3 || mes == 5 || mes == 7 || mes == 8 || mes == 10 || mes == 12) && dia > 31) {
            dia = 1;
            mes++;
        } else if ((mes == 4 || mes == 6 || mes == 9 || mes == 11) && dia > 30) {
            dia = 1;
            mes++;
        } else if (mes == 2 && ((ano % 4 == 0 && ano % 100 != 0) || (ano % 400 == 0)) && dia > 29) {
            dia = 1;
            mes++;
        } else if (mes == 2 && dia > 28) {
            dia = 1;
            mes++;
        }
        if (mes > 12) {
            mes = 1;
            ano++;
        }
    }

    fecha = (dia < 10 ? "0" : "") + String(dia) + "/" +
            (mes < 10 ? "0" : "") + String(mes) + "/" +
            String(ano);

    hora = (horaUTC < 10 ? "0" : "") + String(horaUTC) + ":" +
           (minutos < 10 ? "0" : "") + String(minutos) + ":" +
           (segundos < 10 ? "0" : "") + String(segundos);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char jsonPayload[length + 1];
    memcpy(jsonPayload, payload, length);
    jsonPayload[length] = '\0';

    JSONVar myObject = JSON.parse(jsonPayload);
    if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Error: JSON parsing failed!");
        return;
    }

    if (myObject.hasOwnProperty("pro")) {
        v_produccion = int(myObject["pro"]);
    }
    if (myObject.hasOwnProperty("con")) {
        v_consumo = int(myObject["con"]);
    }
    if (myObject.hasOwnProperty("exi")) {
        v_exporimpor = int(myObject["exi"]);
    }
    if (myObject.hasOwnProperty("sen")) {
        v_sentido = int(myObject["sen"]);
    }
    if (myObject.hasOwnProperty("tex")) {
        v_tempext = int(myObject["tex"]);
    }
    if (myObject.hasOwnProperty("tin")) {
        v_tempint = int(myObject["tin"]);
    }
    if (myObject.hasOwnProperty("hum")) {
        v_humedad = int(myObject["hum"]);
    }
    if (myObject.hasOwnProperty("par")) {
        v_particulas = int(myObject["par"]);
    }
    if (myObject.hasOwnProperty("lit")) {
        v_litros = int(myObject["lit"]);
    }
    if (myObject.hasOwnProperty("tag")) {
        v_tempagua = (double)myObject["tag"];
    }
    if (myObject.hasOwnProperty("mod")) {
        v_modo = (bool)myObject["mod"] ? 1 : 0;
    }
    if (myObject.hasOwnProperty("mau")) {
        v_manual_automatico = (bool)myObject["mau"] ? 1 : 0;
    }
    if (myObject.hasOwnProperty("fec")) {
        obtenerFechaHora(myObject["fec"], fecha, hora);
    }

    actualiza_display();
}

void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Intentando conectar a MQTT...");
        if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
            Serial.println("Conectado a MQTT");
            client.subscribe(mqtt_topic); // Suscribirse al tópico
            Serial.print("Suscrito al tópico: ");
            Serial.println(mqtt_topic);
        } else {
            Serial.print("Falló la conexión a MQTT, estado: ");
            Serial.println(client.state());
            Serial.println("Reintentando en 5 segundos...");
            delay(5000);
        }
    }
}

