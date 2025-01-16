#include "display_utils.h"
#include "secrets.h"

#include "EPD.h"
#include <math.h>
#include "sol.h"
#include "enchufe.h"
#include "escala.h"
#include "flecha.h"
#include "wificon.h"

#define DIA_ACTUAL 0
#define ULTIMAS_24h 1


//uint8_t ImageBW[27200];

// Variables relacionadas con los datos del JSON
unsigned int v_produccion = 0;
unsigned int v_consumo = 0;
unsigned int v_margen = 0;

unsigned int v_potencia_contratada = 4600;
unsigned int v_produccion_max = 5000;
unsigned int v_consumo_max = v_produccion_max + v_potencia_contratada;

String fecha = "";
String hora = "";
String horaAnterior ="00:00:00";
String fechaAnterior = "01/01/2025";

// IDIOMA ESPAÑOL
String titulo0 = " ENERGIA ULTIMAS 24 HORAS "; // 26 caracteres
String titulo1 = "       ENERGIA HOY        "; // 26 caracteres
String titulo2 = "       TIEMPO REAL        "; // 26 caracteres
//                012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
// Define variables related to JSON data
String jsonBuffer;
int httpResponseCode;
JSONVar myObject;

unsigned int display = 0;   // '0' for Wall Panel Home Assistant, '1' for openweather

WiFiClient espClient;
PubSubClient client(espClient);


//float produccion_acumulada[24] = {0}; // Producción acumulada por hora
//float consumo_acumulado[24] = {0};    // Consumo acumulado por hora
float produccion_acumulada[24] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 300.0, 1000.0, 2000.0, 2800.0, 3200.0, 3500.0, 3500.0, 3200.0, 2500.0, 1800.0, 900.0, 300.0, 0.0, 0.0, 0.0, 0.0}; // Producción acumulada por hora
float consumo_acumulado[24] = {200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 500.0, 2000.0, 2500.0, 2500.0, 2800.0, 2800.0, 2800.0, 2800.0, 2800.0, 2500.0, 2000.0, 1800.0, 1500.0, 1500.0, 1500.0, 1500.0, 1500.0}; // Consumo acumulado por hora
int hora_anterior = -1; // Para detectar cambios de hora
int hora_actual = -1;


void inicializa_display() {
    // Set the screen power pin to output mode and set it to high level to turn on the power
    pinMode(7, OUTPUT);  // Set GPIO 7 to output mode
    digitalWrite(7, HIGH);  // Set GPIO 7 to high level to turn on the power

    pinMode(1, INPUT);  // Set GPIO 1 to input mode EXIT
    pinMode(2, INPUT);  // Set GPIO 2 to input mode MENU
    pinMode(4, INPUT);  // Set GPIO 4 to input mode DOWN
    pinMode(5, INPUT);  // Set GPIO 5 to input mode CONF
    pinMode(6, INPUT);  // Set GPIO 6 to input mode UP

    EPD_Init();
    EPD_Clear(0, 0, 296, 128, WHITE);
    EPD_ALL_Fill(WHITE);
    EPD_Update();
    EPD_Clear_R26H();
}

void clear_all()
{
  EPD_Init();
  EPD_Clear(0, 0, 296, 128, WHITE);
  EPD_ALL_Fill(WHITE);
  EPD_Update();
  EPD_Clear_R26H();
}

void simbolo_wifi(int estado) {
    if (estado)
        EPD_ShowPicture(287, 0, 8, 8, gImage_wifi, BLACK);
    else
        EPD_ShowPicture(287, 0, 8, 8, gImage_blanca, BLACK);
}

void simbolo_mqtt(int estado) {
    if (estado)
        EPD_ShowPicture(278, 0, 8, 8, gImage_mqtt, BLACK);
    else
        EPD_ShowPicture(278, 0, 8, 8, gImage_blanca, BLACK);
}

void encabezado(String titulo) {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%10s %8s %s", fecha.c_str(),hora.c_str(),titulo.c_str());
    EPD_ShowString(0, 0, buffer, BLACK, 12);
    simbolo_wifi(1);
    simbolo_mqtt(1);

}


void actualiza_display_2() {
    char buffer[50];

        EPD_Clear(0, 0, 296, 128, WHITE);

        encabezado(titulo2);
        
        snprintf(buffer, sizeof(buffer), "Produccion");
        EPD_ShowString(0, 44, buffer, BLACK, 16);

        snprintf(buffer, sizeof(buffer), "%u Wh", v_produccion);
        EPD_ShowString(0, 62, buffer, BLACK, 16);

        snprintf(buffer, sizeof(buffer), "Consumo");
        EPD_ShowString(0, 88, buffer, BLACK, 16);
        snprintf(buffer, sizeof(buffer), "%u Wh", v_consumo);
        EPD_ShowString(0, 104, buffer, BLACK, 16);


        EPD_ShowGauge(100-v_margen*100/(v_potencia_contratada+v_produccion));
        
        snprintf(buffer, sizeof(buffer), "Margen:", v_margen);
        EPD_ShowString(150, 80, buffer, BLACK, 16);
        snprintf(buffer, sizeof(buffer), "%uWh", v_margen);
        EPD_ShowString(140, 100, buffer, BLACK, 24);

        EPD_DisplayImage(ImageBW);
        EPD_PartUpdate();


}

void actualiza_display_0() {
    char buffer[50];
    int x = 103;
    int y1 = 12;
    int y2 = 71;
    int ancho = 192;
    int alto = 48;
    int anchocolumna = (ancho-24)/24;

    float prod_total, cons_total;

    prod_total = 0.0;
    cons_total = 0.0;
    for (int i = 0; i < 24; i++) {
        prod_total += produccion_acumulada[i];
        cons_total += consumo_acumulado[i];
    }

    //EPD_GPIOInit();
    EPD_Clear(0, 0, 296, 128, WHITE);
    encabezado(titulo0);
    
    EPD_ShowPicture(0, y1+10, 16, 16, gImage_sol, BLACK);
    snprintf(buffer, sizeof(buffer), "%7.1fKWh", prod_total/1000);
    EPD_ShowString(17, y1, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7.1fWh", produccion_acumulada[hora_actual]);
    EPD_ShowString(17, y1+12, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7uW", v_produccion);
    EPD_ShowString(17, y1+24, buffer, BLACK, 12);
    EPD_DrawRectangle(0, y1, 80, 36, BLACK, WHITE);
    EPD_ShowPicture(0, y1+38, 80, 5, gImage_escala, BLACK);
    int xflecha = v_produccion*77/v_produccion_max;
    if (xflecha > 77) xflecha = 77;
    if (xflecha < 0) xflecha = 0;
    EPD_ShowPicture(xflecha, y1+43, 8, 8, gImage_flecha, BLACK);
    
    EPD_ShowPicture(0, y2+10, 16, 22, gImage_enchufe, BLACK);
    snprintf(buffer, sizeof(buffer), "%7.1fKWh", cons_total/1000);
    EPD_ShowString(17, y2, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7.1fWh", consumo_acumulado[hora_actual]);
    EPD_ShowString(17, y2+12, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7uW", v_consumo);
    EPD_ShowString(17, y2+24, buffer, BLACK, 12);
    EPD_DrawRectangle(0, y2, 80, 36, BLACK, WHITE);
    EPD_ShowPicture(0, y2+38, 80, 5, gImage_escala, BLACK);
    xflecha = v_consumo*77/v_consumo_max;
    if (xflecha > 77) xflecha = 77;
    if (xflecha < 0) xflecha = 0;
    EPD_ShowPicture(xflecha, y2+43, 8, 8, gImage_flecha, BLACK);

    EPD_DrawBarGraph(x, y1, ancho, alto, anchocolumna, 1, produccion_acumulada, ULTIMAS_24h);  
    snprintf(buffer, sizeof(buffer), "-24h       -12h   %6.1fWh^", produccion_acumulada[hora_actual]);
    EPD_ShowString(x, y1+alto+1, buffer, BLACK, 8);
    EPD_DrawBarGraph(x, y2, ancho, alto, anchocolumna, 1, consumo_acumulado, ULTIMAS_24h);  
    snprintf(buffer, sizeof(buffer), "-24h       -12h   %6.1fWh^", consumo_acumulado[hora_actual]);
    EPD_ShowString(x, y2+alto+1, buffer, BLACK, 8);
    
    // Actualizar pantalla
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
}

void actualiza_display_1() {
    char buffer[50];
    int x = 103;
    int y1 = 12;
    int y2 = 71;
    int ancho = 192;
    int alto = 48;
    int anchocolumna = (ancho-24)/24;

    float prod_total, cons_total;

    prod_total = 0.0;
    cons_total = 0.0;
    for (int i = 0; i <= hora_actual; i++) {
        prod_total += produccion_acumulada[i];
        cons_total += consumo_acumulado[i];
    }

    //EPD_GPIOInit();
    EPD_Clear(0, 0, 296, 128, WHITE);
    encabezado(titulo1);
    
    EPD_ShowPicture(0, y1+10, 16, 16, gImage_sol, BLACK);
    snprintf(buffer, sizeof(buffer), "%7.1fKWh", prod_total/1000);
    EPD_ShowString(17, y1, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7.1fWh", produccion_acumulada[hora_actual]);
    EPD_ShowString(17, y1+12, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7uW", v_produccion);
    EPD_ShowString(17, y1+24, buffer, BLACK, 12);
    EPD_DrawRectangle(0, y1, 80, 36, BLACK, WHITE);
    EPD_ShowPicture(0, y1+38, 80, 5, gImage_escala, BLACK);
    int xflecha = v_produccion*77/v_produccion_max;
    if (xflecha > 77) xflecha = 77;
    if (xflecha < 0) xflecha = 0;
    EPD_ShowPicture(xflecha, y1+43, 8, 8, gImage_flecha, BLACK);
    
    EPD_ShowPicture(0, y2+10, 16, 22, gImage_enchufe, BLACK);
    snprintf(buffer, sizeof(buffer), "%7.1fKWh", cons_total/1000);
    EPD_ShowString(17, y2, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7.1fWh", consumo_acumulado[hora_actual]);
    EPD_ShowString(17, y2+12, buffer, BLACK, 12);
    snprintf(buffer, sizeof(buffer), "%7uW", v_consumo);
    EPD_ShowString(17, y2+24, buffer, BLACK, 12);
    EPD_DrawRectangle(0, y2, 80, 36, BLACK, WHITE);
    EPD_ShowPicture(0, y2+38, 80, 5, gImage_escala, BLACK);
    xflecha = v_consumo*77/v_consumo_max;
    if (xflecha > 77) xflecha = 77;
    if (xflecha < 0) xflecha = 0;
    EPD_ShowPicture(xflecha, y2+43, 8, 8, gImage_flecha, BLACK);

    EPD_DrawBarGraph(x, y1, ancho, alto, anchocolumna, 1, produccion_acumulada, DIA_ACTUAL);  
    snprintf(buffer, sizeof(buffer), "-24h       -12h   %6.1fWh^", produccion_acumulada[hora_actual]);
    EPD_ShowString(x, y1+alto+1, buffer, BLACK, 8);
    EPD_DrawBarGraph(x, y2, ancho, alto, anchocolumna, 1, consumo_acumulado, DIA_ACTUAL);  
    snprintf(buffer, sizeof(buffer), "-24h       -12h   %6.1fWh^", consumo_acumulado[hora_actual]);
    EPD_ShowString(x, y2+alto+1, buffer, BLACK, 8);
    
    // Actualizar pantalla
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
};
    

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

void actualiza_display() {
    switch (display) {
        case 0:
            actualiza_display_0();
            break;
        case 1:
            actualiza_display_1();
            break;
        case 2:
            actualiza_display_2();
            break;
    }
};

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
    if (myObject.hasOwnProperty("fec")) {
        obtenerFechaHora(myObject["fec"], fecha, hora);
        hora_actual = hora.toInt();  // Convertir la hora a int
    }
        // Actualizar acumulados por hora
    if (hora_actual != -1) {
        // Si se detecta un cambio de hora, reiniciar los acumulados para la nueva hora
        if (hora_actual != hora_anterior) {
            produccion_acumulada[hora_actual] = 0.0;
            consumo_acumulado[hora_actual] = 0.0;
            hora_anterior = hora_actual;
        }
    }

    consumo_acumulado[hora_actual] += integraEnergia(v_consumo,hora, horaAnterior);
    produccion_acumulada[hora_actual] += integraEnergia(v_produccion, hora, horaAnterior);
    mantenerHoraAnterior(fecha, hora, horaAnterior, fechaAnterior);

    v_margen = v_potencia_contratada+v_produccion-v_consumo;


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


void EPD_ShowGauge(uint8_t percentage) {
    // Validar el porcentaje entre 0 y 100
    if (percentage > 100) {
        percentage = 100;
    }

    // Dimensiones y posición del gauge
    uint16_t centerX = 176;   // Centro de la pantalla (296 / 2)
    uint16_t centerY = 127;   // Centro de la pantalla (256 / 2)
    uint16_t outerRadius = 120; // Radio exterior del gauge
    uint16_t innerRadius = 60;  // Radio interior del gauge
    uint16_t startAngle = 180;  // Ángulo de inicio (izquierda abajo)
    uint16_t endAngle = 0;      // Ángulo de fin (derecha abajo)

    // Calcular el ángulo correspondiente al porcentaje
    float percentageAngle = startAngle - (startAngle - endAngle) * (percentage / 100.0);

    // Recorrer cada píxel dentro de los límites del rectángulo que contiene el gauge
    for (int y = centerY - outerRadius; y <= centerY + outerRadius; y++) {
        for (int x = centerX - outerRadius; x <= centerX + outerRadius; x++) {
            // Calcular distancia al centro
            float dx = x - centerX;
            float dy = y - centerY;
            float distance = sqrt(dx * dx + dy * dy);

            // Verificar si el píxel está dentro del anillo del gauge
            if (distance >= innerRadius && distance <= outerRadius) {
                // Calcular el ángulo del píxel con respecto al centro
                float angle = atan2(-dy, dx) * 180.0 / PI;  // Convertir a grados
                if (angle < 0) angle += 360;

                // Verificar si el ángulo está dentro del rango a rellenar
                if (angle >= endAngle && angle <= startAngle) {
                    if (angle <= percentageAngle) {
                        EPD_DrawPoint(x, y, WHITE);  // Rellenar sector
                    } else {
                        EPD_DrawPoint(x, y, BLACK); // Rellenar el resto como fondo
                    }
                }
            }
        }
    }

    // Dibujar los bordes exteriores e interiores del gauge
    for (int angle = startAngle; angle >= endAngle; angle--) {
        float rad = angle * PI / 180.0;

        // Borde exterior
        uint16_t outerX = centerX + outerRadius * cos(rad);
        uint16_t outerY = centerY - outerRadius * sin(rad);
        EPD_DrawPoint(outerX, outerY, BLACK);

        // Borde interior
        uint16_t innerX = centerX + innerRadius * cos(rad);
        uint16_t innerY = centerY - innerRadius * sin(rad);
        EPD_DrawPoint(innerX, innerY, BLACK);
    }
}

void EPD_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color, uint8_t hueco_relleno) {
    // Dibujar el rectángulo
    for (uint16_t i = 0; i < width; i++) {
        for (uint16_t j = 0; j < height; j++) {
            if (hueco_relleno == 0) {
                EPD_DrawPoint(x + i, y + j, color);
            } else {
                if (i == 0 || i == width - 1 || j == 0 || j == height - 1) {
                    EPD_DrawPoint(x + i, y + j, color);
                }
            }
        }
    }
};


// Definir el array global para los promedios de energía (24 valores)
void EPD_DrawBarGraph(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t bar_width, uint16_t bar_spacing, float energy_values[24], uint8_t tipo_rango) {
    // x, y: Coordenadas del borde superior izquierdo de la gráfica
    // width: Ancho total de la gráfica
    // height: Altura total de la gráfica
    // bar_width: Ancho de cada barra
    // bar_spacing: Espacio entre las barras
    // energy_values: Array de valores para las 24 barras
    // hora_actual: Índice de la hora actual (0 a 23)

    // Número de barras (basado en el array recibido)
    uint16_t num_bars = 24;

    // Crear un array reorganizado para que la hora actual sea la última barra
    float adjusted_values[24];
    for (uint16_t i = 0; i < num_bars; i++) {
        if (tipo_rango == ULTIMAS_24h) {
            uint16_t new_index = (i + hora_actual + 1) % num_bars;
            adjusted_values[i] = energy_values[new_index];
        } else {
            adjusted_values[i] = energy_values[i];
        }
    }

    // Calcular el espacio total ocupado por una barra (ancho + separación)
    uint16_t total_bar_width = bar_width + bar_spacing;

    // Determinar el valor máximo
    float max_value = adjusted_values[0];
    for (uint16_t i = 0; i < num_bars; i++) {
        if (adjusted_values[i] > max_value) {
            max_value = adjusted_values[i];
        }
    }

    // Mostrar los valores máximo y mínimo en el eje Y
    char label[10];
    snprintf(label, sizeof(label), "%.1f", max_value / 1000);
    EPD_ShowString(x - 23, y, label, BLACK, 8); // Máximo

    snprintf(label, sizeof(label), "KWh");
    EPD_ShowString(x - 21, (2 * y + height) / 2 - 4, label, BLACK, 8); // Unidad

    snprintf(label, sizeof(label), "0.0");
    EPD_ShowString(x - 23, y + height - 8, label, BLACK, 8); // Mínimo

    // Dibujar las barras
    for (uint16_t i = 0; i < num_bars; i++) {
        if (((tipo_rango == DIA_ACTUAL) && (i<=hora_actual)) || (tipo_rango == ULTIMAS_24h)) {
            
            // Calcular la altura de la barra proporcional al valor
            float value = adjusted_values[i];
            if (value > max_value) {
                value = max_value;
            }
            uint16_t bar_height = (uint16_t)((value / max_value) * height);

            // Calcular la posición x de la barra
            uint16_t bar_x = x + i * total_bar_width;

            // Calcular la posición y de la barra
            uint16_t bar_y = y + (height - bar_height);

            // Dibujar la barra (patrón especial para la última barra)
            for (uint16_t bx = bar_x; bx < (bar_x + bar_width); bx++) {
                for (uint16_t by = bar_y; by < (bar_y + bar_height); by++) {
                    if (((i == num_bars - 1) && (tipo_rango == ULTIMAS_24h)) || ((i == hora_actual) && (tipo_rango == DIA_ACTUAL))) {
                        // Patrón gris (dibujar solo píxeles alternos)
                        if ((bx + by) % 2 == 0) {
                            EPD_DrawPoint(bx, by, BLACK);
                        }
                    } else {
                        // Barra completamente negra
                        EPD_DrawPoint(bx, by, BLACK);
                    }
                }
            }
        }
    }

    // Dibujar el borde del gráfico
    for (uint16_t i = 0; i <= width; i++) {
        //EPD_DrawPoint(x + i, y, BLACK);                 // Borde superior
        EPD_DrawPoint(x + i, y + height, BLACK);        // Borde inferior
    }
    for (uint16_t i = 0; i <= height; i++) {
        EPD_DrawPoint(x, y + i, BLACK);                 // Borde izquierdo
        //EPD_DrawPoint(x + width, y + i, BLACK);         // Borde derecho
    }
}




float integraEnergia(int consumoActual, String horaActual, String horaAnterior) {
    float energiaIntervalo;

    // Imprimir entradas iniciales
    Serial.println("=== Debug integraEnergia ===");
    Serial.print("Consumo actual: ");
    Serial.println(consumoActual);
    Serial.print("Hora actual: ");
    Serial.println(horaActual);
    Serial.print("Hora anterior: ");
    Serial.println(horaAnterior);

    // Convertir horaActual y horaAnterior a segundos desde el inicio del día
    int hActual, mActual, sActual;
    int hAnterior, mAnterior, sAnterior;

    sscanf(horaActual.c_str(), "%02d:%02d:%02d", &hActual, &mActual, &sActual);
    sscanf(horaAnterior.c_str(), "%02d:%02d:%02d", &hAnterior, &mAnterior, &sAnterior);

    // Imprimir horas convertidas
    Serial.print("Hora actual (h:m:s): ");
    Serial.print(hActual); Serial.print(":");
    Serial.print(mActual); Serial.print(":");
    Serial.println(sActual);

    Serial.print("Hora anterior (h:m:s): ");
    Serial.print(hAnterior); Serial.print(":");
    Serial.print(mAnterior); Serial.print(":");
    Serial.println(sAnterior);

    unsigned long segundosActual = hActual * 3600 + mActual * 60 + sActual;
    unsigned long segundosAnterior = hAnterior * 3600 + mAnterior * 60 + sAnterior;

    // Imprimir tiempo en segundos
    Serial.print("Segundos actuales: ");
    Serial.println(segundosActual);
    Serial.print("Segundos anteriores: ");
    Serial.println(segundosAnterior);

    // Calcular el tiempo transcurrido en segundos
    long deltaSegundos = segundosActual - segundosAnterior;
    if (deltaSegundos < 0) {
        // Si el tiempo actual es menor que el anterior, asumimos un cambio de día
        deltaSegundos += 24 * 3600;
        Serial.println("Cambio de día detectado. Ajustando deltaSegundos.");
    }

    if (deltaSegundos<3600) {


    Serial.print("Delta segundos: ");
    Serial.println(deltaSegundos);

    // Calcular la energía consumida en el intervalo (Wh)
    energiaIntervalo = (float(consumoActual) * deltaSegundos) / 3600.0;

    // Imprimir energía calculada en el intervalo
    Serial.print("Energía intervalo (Wh): ");
    Serial.println(energiaIntervalo);
    } else {
        energiaIntervalo =0;
    }
    // Devolver el nuevo consumo acumulado
    return energiaIntervalo;
}


void mantenerHoraAnterior(String fecha, String hora, String &horaAnterior, String &fechaAnterior) {
    // Si la fecha cambia, reiniciar horaAnterior a las 00:00:00 del nuevo día
    if (fecha != fechaAnterior) {
        fechaAnterior = fecha;
        horaAnterior = "00:00:00";
    } else {
        horaAnterior = hora;
    }
}
