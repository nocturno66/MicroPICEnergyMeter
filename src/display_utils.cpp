#include <Arduino_JSON.h>
#include <Arduino.h>
#include "secrets.h"
#include <Preferences.h>
#include "display_utils.h"
#include "EPD.h"
#include <math.h>
#include "sol.h"
#include "enchufe.h"
#include "escala.h"
#include "flecha.h"
#include "wificon.h"
#include "gauge.h"
#include "micropic.h"
#include "main.h"

#define DIA_ACTUAL 0
#define ULTIMAS_24h 1

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


/***************************************************************************************************************************/
/***************************************************************************************************************************/
/********************                           VARIABLES                ***************************************************/ 
/***************************************************************************************************************************/
/***************************************************************************************************************************/

// Variables relacionadas con los datos del JSON
unsigned int v_produccion = 0;
unsigned int v_consumo = 0;
int v_limitador = 0;
int alerta_capacidad=0, antigua_alerta_capacidad = 0;
bool alarma=false;

extern Preferences preferences;
extern unsigned int v_potencia_contratada;
extern unsigned int v_produccion_max;
extern unsigned int v_limitador_max;
extern String ssid, password, mqtt_server, mqtt_topic, mqtt_user, mqtt_password;
extern String mqtt_topic_CONF, mqtt_topic_DOWN, mqtt_topic_UP, mqtt_topic_MENU, mqtt_topic_EXIT, mqtt_topic_ALERT;
extern unsigned int v_consumo_max;

String fecha = "";
String hora = "";
String horaAnterior ="00:00:00";
String fechaAnterior = "01/01/2025";

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

// Variables globales
MenuOption currentOption = MENU_LANGUAGE;
bool optionSelected = false;
Language currentLanguage = LANGUAGE_SPANISH;
Connection currentConnection = NO_CAMBIO;

// Textos dinámicos
String menuLanguage, menuPower, menuMaxProduction, menuLimiter, menuConnection, menuExit;
String produccion, consumo, limitador, titulo0, titulo1, titulo2;
String mens_connecting, mens_connected, mens_failed, mens_mqtt_connecting, mens_mqtt_connected, mens_mqtt_failed;
String mens_abrirwifimovil, mens_navegador;

// Variables de tiempo para el movimiento por el menú
unsigned long lastPressTime = 0;
unsigned long repeatDelay = 500;    // Retraso inicial para comenzar la repetición (en milisegundos)
unsigned long repeatInterval = 50; // Intervalo entre repeticiones (en milisegundos)



/***************************************************************************************************************************/
/***************************************************************************************************************************/
/********************                           CÁLCULOS                 ***************************************************/ 
/***************************************************************************************************************************/
/***************************************************************************************************************************/

    

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

/***************************************************************************************************************************/
/***************************************************************************************************************************/
/********************                FUNCIONES GRÁFICAS                  ***************************************************/ 
/***************************************************************************************************************************/
/***************************************************************************************************************************/

void inicializa_display() {
    // Set the screen power pin to output mode and set it to high level to turn on the power
    pinMode(PIN_ENABLE, OUTPUT);  // Set GPIO 7 to output mode
    digitalWrite(PIN_ENABLE, HIGH);  // Set GPIO 7 to high level to turn on the power

    pinMode(PIN_EXIT, INPUT);  // Set GPIO 1 to input mode EXIT
    pinMode(PIN_MENU, INPUT);  // Set GPIO 2 to input mode MENU
    pinMode(PIN_DOWN, INPUT);  // Set GPIO 4 to input mode DOWN
    pinMode(PIN_CONF, INPUT);  // Set GPIO 5 to input mode CONF
    pinMode(PIN_UP, INPUT);  // Set GPIO 6 to input mode UP

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
    char buffer[3];
    if (estado)
        EPD_ShowPicture(287, 0, 8, 8, gImage_wifi, BLACK);
    else {
        EPD_ShowPicture(287, 0, 8, 8, gImage_blanca, BLACK);
    }
}

void simbolo_mqtt(int estado) {
    char buffer[3];
    if (estado)
        EPD_ShowPicture(278, 0, 8, 8, gImage_mqtt, BLACK);
    else{
        EPD_ShowPicture(278, 0, 8, 8, gImage_blanca, BLACK);
    }
}

void encabezado(String titulo) {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%10s %8s %s", fecha.c_str(),hora.c_str(),titulo.c_str());
    EPD_ShowString(0, 0, buffer, BLACK, 12);
    simbolo_wifi(1);
    simbolo_mqtt(1);
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

// Función auxiliar para rotar un punto alrededor de un centro
void rotate_point(float cx, float cy, float angle, float *x, float *y) {
    angle = angle * PI / 180.0; // Convertir a radianes
    float s = sin(angle);
    float c = cos(angle);

    // Trasladar el punto al origen
    float x_new = *x - cx;
    float y_new = *y - cy;

    // Aplicar la rotación
    float x_rot = x_new * c - y_new * s;
    float y_rot = x_new * s + y_new * c;

    // Trasladar el punto de vuelta
    *x = x_rot + cx;
    *y = y_rot + cy;
}

// Función para determinar si un punto está dentro de un triángulo usando vectores cruzados
int point_in_triangle(float px, float py, float x1, float y1, float x2, float y2, float x3, float y3) {
    float v0x = x3 - x1;
    float v0y = y3 - y1;
    float v1x = x2 - x1;
    float v1y = y2 - y1;
    float v2x = px - x1;
    float v2y = py - y1;

    float dot00 = v0x * v0x + v0y * v0y;
    float dot01 = v0x * v1x + v0y * v1y;
    float dot02 = v0x * v2x + v0y * v2y;
    float dot11 = v1x * v1x + v1y * v1y;
    float dot12 = v1x * v2x + v1y * v2y;

    float inv_denom = 1.0 / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * inv_denom;
    float v = (dot00 * dot12 - dot01 * dot02) * inv_denom;

    return (u >= 0) && (v >= 0) && (u + v < 1);
}

// Función para dibujar un triángulo isósceles rotado
void Draw_Isosceles_Triangle(uint16_t x, uint16_t y, uint16_t base, uint16_t height, float rotation_angle, uint8_t color, uint8_t fill) {
    // Calcular los vértices del triángulo isósceles sin rotación
    float x1 = x - base / 2.0; // Vértice izquierdo de la base
    float y1 = y;
    float x2 = x + base / 2.0; // Vértice derecho de la base
    float y2 = y;
    float x3 = x;              // Vértice superior (punta)
    float y3 = y - height;

    // Rotar los vértices alrededor del punto central del triángulo
    rotate_point(x, y, rotation_angle, &x1, &y1);
    rotate_point(x, y, rotation_angle, &x2, &y2);
    rotate_point(x, y, rotation_angle, &x3, &y3);

    if (fill) {
        // Dibujar el triángulo relleno comprobando cada punto dentro del área
        uint16_t min_x = (uint16_t)fmin(fmin(x1, x2), x3);
        uint16_t max_x = (uint16_t)fmax(fmax(x1, x2), x3);
        uint16_t min_y = (uint16_t)fmin(fmin(y1, y2), y3);
        uint16_t max_y = (uint16_t)fmax(fmax(y1, y2), y3);

        for (uint16_t py = min_y; py <= max_y; py++) {
            for (uint16_t px = min_x; px <= max_x; px++) {
                if (point_in_triangle(px, py, x1, y1, x2, y2, x3, y3)) {
                    EPD_DrawPoint(px, py, color);
                }
            }
        }
    } else {
        // Dibujar las líneas del triángulo
        EPD_DrawLine((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color); // Línea base
        EPD_DrawLine((uint16_t)x2, (uint16_t)y2, (uint16_t)x3, (uint16_t)y3, color); // Lado derecho
        EPD_DrawLine((uint16_t)x3, (uint16_t)y3, (uint16_t)x1, (uint16_t)y1, color); // Lado izquierdo
    }
}


uint8_t EPD_ReadPoint(uint16_t x, uint16_t y)
{
    uint8_t dat;
    uint16_t xpoint, ypoint;
    uint32_t Addr;

    // Transform coordinates based on the display orientation
    switch (USE_HORIZONTIAL)
    {
    case 0:
        xpoint = EPD_H - y - 1;
        ypoint = x;
        break;
    case 1:
        xpoint = x;
        ypoint = y;
        break;
    case 2:
        xpoint = y;
        ypoint = EPD_W - x - 1;
        break;
    case 3:
        xpoint = EPD_W - x - 1;
        ypoint = EPD_H - y - 1;
        break;
    default:
        return 0; // Return 0 if an invalid orientation is set
    }

    // Calculate the address of the pixel in the buffer
#if USE_HORIZONTIAL == 0 | USE_HORIZONTIAL == 2
    Addr = xpoint / 8 + ypoint * ((EPD_H % 8 == 0) ? (EPD_H / 8) : (EPD_H / 8 + 1));
#else
    Addr = xpoint / 8 + ypoint * ((EPD_W % 8 == 0) ? (EPD_W / 8) : (EPD_W / 8 + 1));
#endif

    // Read the byte from the buffer
    dat = ImageBW[Addr];

    // Check the specific bit for the pixel
    if (dat & (0x80 >> (xpoint % 8)))
    {
        return BLACK; // Return BLACK if the bit is set
    }
    else
    {
        return WHITE; // Return WHITE if the bit is not set
    }
}

void EPD_InvertPoint (int x, int y) {
    uint16_t x1 = x;
    uint16_t y1 = y;
    uint16_t x2 = x;
    uint16_t y2 = y;
    EPD_DrawPoint(x1, y1, !EPD_ReadPoint(x1, y1));
}

// función que invierte los píxeles de un círculo con centro en (x, y) y radio r
void invierte_circulo(int x, int y, int r) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                EPD_InvertPoint(x + dx, y + dy);
            }
        }
    }
}



/***************************************************************************************************************************/
/***************************************************************************************************************************/
/********************                DISTINTAS PANTALLAS                 ***************************************************/ 
/***************************************************************************************************************************/
/***************************************************************************************************************************/


void actualiza_display() {
    switch (display) {
        case 0:
            display_micropic();
            break;
        case 1:
            display_tiempo_real();
            break;
        case 2:
            display_hoy();
            break;
        case 3:
            display_ultimas24h();
            break;
        case 4:
            display_menu();
            break;
    }
};

void display_micropic() {
    EPD_Clear(0, 0, 296, 128, WHITE);
    EPD_ShowPicture(0, 0, 296, 128, gImage_Micropic, BLACK);
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
}

void display_tiempo_real() {
    char buffer[50];
    int angulo;
        EPD_Clear(0, 0, 296, 128, WHITE);
        encabezado(titulo2);
        snprintf(buffer, sizeof(buffer), "%u Wh", v_produccion);
        EPD_ShowString(0, 62, buffer, BLACK, 16);
        snprintf(buffer, sizeof(buffer), "Consumo");
        EPD_ShowString(0, 88, buffer, BLACK, 16);
        snprintf(buffer, sizeof(buffer), "%u Wh", v_consumo);
        EPD_ShowString(0, 104, buffer, BLACK, 16);
        EPD_ShowPicture(0, 36, 96, 90, gImage_gauge, BLACK);
        EPD_ShowPicture(102, 36, 96, 90, gImage_gauge, BLACK);
        EPD_ShowPicture(204, 36, 96, 90, gImage_gauge, BLACK);
        snprintf(buffer, sizeof(buffer), produccion.c_str());
        EPD_ShowString(14, 24, buffer, BLACK, 12);
        
        if (v_produccion_max==0) {
            snprintf(buffer, sizeof(buffer), "  0%%");
            angulo=-135;
            Serial.println("v_produccion_max=0, angulo=0");
            }
        else {
            snprintf(buffer, sizeof(buffer), "%3u%%", v_produccion*100/v_produccion_max);
            angulo = v_produccion*270/v_produccion_max-135;
            Serial.println("v_produccion_max!=0, angulo calculado");
            }
        Draw_Isosceles_Triangle(45, 81, 8, 40, angulo, BLACK, 1);
        
        EPD_ShowString(32, 98, buffer, BLACK, 12);
        snprintf(buffer, sizeof(buffer), "%5uW", v_produccion);
        EPD_ShowString(26, 109, buffer, BLACK, 12);
        snprintf(buffer, sizeof(buffer), consumo.c_str());
        EPD_ShowString(116, 24, buffer, BLACK, 12);
        if (v_consumo_max==0) {
            snprintf(buffer, sizeof(buffer), "  0%%");
            angulo=-135;
            Serial.println("v_consumo_max=0, angulo=0");
            }
        else {
            snprintf(buffer, sizeof(buffer), "%3u%%", v_consumo*100/v_consumo_max);
            angulo = v_consumo*270/v_consumo_max-135;
            Serial.println("v_consumo_max!=0, angulo calculado");
            }
        Draw_Isosceles_Triangle(147, 81, 8, 40, angulo, BLACK, 1);
        EPD_ShowString(132, 98, buffer, BLACK, 12);
        snprintf(buffer, sizeof(buffer), "%5uW", v_consumo);
        EPD_ShowString(128, 109, buffer, BLACK, 12);
        
        snprintf(buffer, sizeof(buffer), limitador.c_str());
        EPD_ShowString(218, 24, buffer, BLACK, 12);
        if (v_produccion+v_limitador_max==0) {
            snprintf(buffer, sizeof(buffer), "  0%%");
            angulo=135;
            }
        else {
            if (v_limitador>v_produccion+v_limitador_max) {
                angulo = 135;
                snprintf(buffer, sizeof(buffer), "100%%");
            } else {
                snprintf(buffer, sizeof(buffer), "%3u%%", v_limitador*100/(v_produccion+v_limitador_max));
                angulo = v_limitador*270/(v_produccion+v_limitador_max)-135;
            }
        }
        Draw_Isosceles_Triangle(249, 81, 8, 40, angulo, BLACK,1);
        EPD_ShowString(236, 98, buffer, BLACK, 12);
        snprintf(buffer, sizeof(buffer), "%5uW", v_limitador);
        EPD_ShowString(230, 109, buffer, BLACK, 12);
        EPD_DisplayImage(ImageBW);
        EPD_PartUpdate();
        
}


void display_hoy() {
    char buffer[50];
    int x = 103;
    int y1 = 12;
    int y2 = 71;
    int ancho = 192;
    int alto = 48;
    int anchocolumna = (ancho-24)/24;
    int xflecha;

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
    if (v_produccion_max==0) {
        xflecha = 0;
        }
    else {
        xflecha = v_produccion*77/v_produccion_max;
        if (xflecha > 77) xflecha = 77;
        if (xflecha < 0) xflecha = 0;
        }
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
    if (v_consumo_max==0) {
        int xflecha = 0;
        }
    else {
        int xflecha = v_consumo*77/v_consumo_max;
        if (xflecha > 77) xflecha = 77;
        if (xflecha < 0) xflecha = 0;
        }
    EPD_ShowPicture(xflecha, y2+43, 8, 8, gImage_flecha, BLACK);

    EPD_DrawBarGraph(x, y1, ancho, alto, anchocolumna, 1, produccion_acumulada, ULTIMAS_24h);  
    EPD_DrawBarGraph(x, y2, ancho, alto, anchocolumna, 1, consumo_acumulado, ULTIMAS_24h);  

    //snprintf(buffer, sizeof(buffer), "-24h       -12h   %6.1fWh^", produccion_acumulada[hora_actual]);
    snprintf(buffer, sizeof(buffer), "-24h        -12h          ^");
    EPD_ShowString(x, y1+alto+1, buffer, BLACK, 8);
    
    snprintf(buffer, sizeof(buffer), "-24h        -12h          ^");
    EPD_ShowString(x, y2+alto+1, buffer, BLACK, 8);
    
    // Actualizar pantalla
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
}

void display_ultimas24h() {
    char buffer[50];
    char potencia[10];
    int x = 103;
    int y1 = 12;
    int y2 = 71;
    int ancho = 192;
    int alto = 48;
    int anchocolumna = (ancho-24)/24;
    int posicion_insercion;
    int xflecha;

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
    if (v_produccion_max==0) {
        xflecha = 0;
        }
    else {
        xflecha = v_produccion*77/v_produccion_max;
        if (xflecha > 77) xflecha = 77;
        if (xflecha < 0) xflecha = 0;
        }
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
    if (v_consumo_max==0) {
        int xflecha = 0;
        }
    else {
        int xflecha = v_consumo*77/v_consumo_max;
        if (xflecha > 77) xflecha = 77;
        if (xflecha < 0) xflecha = 0;
        }
    EPD_ShowPicture(xflecha, y2+43, 8, 8, gImage_flecha, BLACK);

    EPD_DrawBarGraph(x, y1, ancho, alto, anchocolumna, 1, produccion_acumulada, DIA_ACTUAL);  
    EPD_DrawBarGraph(x, y2, ancho, alto, anchocolumna, 1, consumo_acumulado, DIA_ACTUAL);  
                                     
    snprintf(buffer, sizeof(buffer), "0h          12h          23h");
    EPD_ShowString(x, y1+alto+1, buffer, BLACK, 8);
    snprintf(buffer, sizeof(buffer), "0h          12h          23h");
    EPD_ShowString(x, y2+alto+1, buffer, BLACK, 8);
    // Actualizar pantalla
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
};

/***************************************************************************************************************************/
/***************************************************************************************************************************/
/********************                       MQTT                         ***************************************************/ 
/***************************************************************************************************************************/
/***************************************************************************************************************************/



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

    v_limitador = v_consumo-v_produccion;
    if (v_limitador<0)  v_limitador = 0;

    alerta_capacidad =  (v_limitador>v_limitador_max);
    if (antigua_alerta_capacidad != alerta_capacidad) {
        antigua_alerta_capacidad = alerta_capacidad;
        if (alerta_capacidad) {
            sendMQTTMessage(mqtt_topic_ALERT.c_str(),"ON");
            digitalWrite(LED, 1);
            alarma=true;
        }
        else {
            sendMQTTMessage(mqtt_topic_ALERT.c_str(),"OFF");
            digitalWrite(LED, 0);
            alarma=false;
        }
    };
    actualiza_display();
}

void reconnectMQTT() {
    if (!client.connected()) {
        //Serial.print("Intentando conectar a MQTT...");
        if (client.connect("ESP32Client", mqtt_user.c_str(), mqtt_password.c_str())) {
            Serial.println("Conectado a MQTT");
            client.subscribe(mqtt_topic.c_str()); // Suscribirse al tópico
            Serial.print("Suscrito al tópico: ");
            Serial.println(mqtt_topic.c_str());
            simbolo_mqtt(1);
            EPD_ShowString(0, 112, mens_mqtt_connected.c_str(), BLACK, 12);
            EPD_DisplayImage(ImageBW);
            EPD_PartUpdate();
        } else {
            EPD_ShowString(0, 112, mens_mqtt_failed.c_str(), BLACK, 12);
            EPD_DisplayImage(ImageBW);
            EPD_PartUpdate();
            Serial.print ("@");
        }
    }
}

void sendMQTTMessage(const char* topic, const char* message) {
    
    if (client.connected()) {
        if (client.publish(topic, message)) {
            Serial.println("Mensaje enviado correctamente:");
            Serial.println(message);
        } else {
            Serial.println("Error al enviar el mensaje.");
        }
    } else {
        Serial.println("No conectado al servidor MQTT. Reintentando...");
        reconnectMQTT(); // Asegúrate de tener esta función para reconectar
    }
}



void DrawMenu(MenuOption selectedOption, bool editingValue) {
    char buffer[10];
    // Limpia la pantalla
    EPD_Clear(0, 0, 296, 128, WHITE);

    // Muestra las opciones de menú
    EPD_ShowString(0, 0, "MicroPIC MQTT Energy Meter - SETUP", BLACK, 12);
    EPD_DrawLine(0, 14, 296, 14, BLACK);
    EPD_ShowString(10, 16, menuLanguage.c_str(), (selectedOption == MENU_LANGUAGE && !editingValue) ? WHITE : BLACK, 12);
    EPD_ShowString(170, 16, (currentLanguage == LANGUAGE_SPANISH) ? "Castellano" : "English", (selectedOption == MENU_LANGUAGE && editingValue) ? WHITE : BLACK, 12);

    EPD_ShowString(10, 28, menuPower.c_str(), (selectedOption == MENU_POWER && !editingValue) ? WHITE : BLACK, 12);
    sprintf(buffer, "%d", v_potencia_contratada);
    EPD_ShowString(170, 28, buffer, (selectedOption == MENU_POWER && editingValue) ? WHITE : BLACK, 12);

    EPD_ShowString(10, 40, menuMaxProduction.c_str(), (selectedOption == MENU_MAX_PRODUCTION && !editingValue) ? WHITE : BLACK, 12);
    sprintf(buffer, "%d", v_produccion_max);
    EPD_ShowString(170, 40, buffer, (selectedOption == MENU_MAX_PRODUCTION && editingValue) ? WHITE : BLACK, 12);

    EPD_ShowString(10, 52, menuLimiter.c_str(), (selectedOption == MENU_LIMITER && !editingValue) ? WHITE : BLACK, 12);
    sprintf(buffer, "%d", v_limitador_max);
    EPD_ShowString(170, 52, buffer, (selectedOption == MENU_LIMITER && editingValue) ? WHITE : BLACK, 12);

    EPD_ShowString(10, 64, menuConnection.c_str(), (selectedOption == MENU_CONNECTION && !editingValue) ? WHITE : BLACK, 12);
    EPD_ShowString(170, 64, (currentConnection == NO_CAMBIO) ? "No cambiar" : "Configurar", (selectedOption == MENU_CONNECTION && editingValue) ? WHITE : BLACK, 12);

    EPD_ShowString (10, 76, menuExit.c_str(), (selectedOption == MENU_EXIT) ? WHITE : BLACK, 12);

    // Actualizar pantalla
    EPD_DisplayImage(ImageBW);
    EPD_PartUpdate();
}


void UpdateMenuSelection(bool &editingValue, bool &exitMenu) {
    // Variables para almacenar el estado de las teclas
    static bool isRepeating = false; // Indica si ya estamos en modo de repetición

    // Tecla hacia arriba
    if (digitalRead(PIN_UP) == 0) {
        unsigned long currentTime = millis();
        if (lastPressTime == 0 || (isRepeating && currentTime - lastPressTime >= repeatInterval) ||
            (!isRepeating && currentTime - lastPressTime >= repeatDelay)) {
            
            // Realiza la acción de "tecla arriba"
            if (editingValue) {
                switch (currentOption) {
                    case MENU_LANGUAGE:
                        currentLanguage = (currentLanguage == LANGUAGE_SPANISH) ? LANGUAGE_ENGLISH : LANGUAGE_SPANISH;
                        UpdateLanguageTexts();
                        break;
                    case MENU_POWER:
                        v_potencia_contratada = min(v_potencia_contratada + 100, 99000);
                        break;
                    case MENU_MAX_PRODUCTION:
                        v_produccion_max = min(v_produccion_max + 100, 99000);
                        break;
                    case MENU_LIMITER:
                        v_limitador_max = min(v_limitador_max + 100, 99000);
                        break;
                    case MENU_CONNECTION:
                        currentConnection = (currentConnection == NO_CAMBIO) ? CONFIGURAR : NO_CAMBIO;
                        break;
                    default:
                        break;
                }
            } else {
                currentOption = (currentOption == MENU_LANGUAGE) ? (MenuOption)(MENU_OPTIONS_COUNT - 1) : (MenuOption)(currentOption - 1);
            }
            DrawMenu(currentOption, editingValue);
            lastPressTime = currentTime; // Actualiza el tiempo de la última acción
            isRepeating = true;          // Activa el modo de repetición
        }
    } else {
        isRepeating = false;   // Reinicia la repetición si la tecla no está presionada
        lastPressTime = 0;     // Reinicia el tiempo de la última acción
    }

    // Tecla hacia abajo
    if (digitalRead(PIN_DOWN) == 0) {
        unsigned long currentTime = millis();
        if (lastPressTime == 0 || (isRepeating && currentTime - lastPressTime >= repeatInterval) ||
            (!isRepeating && currentTime - lastPressTime >= repeatDelay)) {
            
            // Realiza la acción de "tecla abajo"
            if (editingValue) {
                switch (currentOption) {
                    case MENU_LANGUAGE:
                        currentLanguage = (currentLanguage == LANGUAGE_SPANISH) ? LANGUAGE_ENGLISH : LANGUAGE_SPANISH;
                        UpdateLanguageTexts();
                        break;
                    case MENU_POWER:
                        v_potencia_contratada = max(v_potencia_contratada - 100, 0);
                        break;
                    case MENU_MAX_PRODUCTION:
                        v_produccion_max = max(v_produccion_max - 100, 0);
                        break;
                    case MENU_LIMITER:
                        v_limitador_max = max(v_limitador_max - 100, 0);
                        break;
                    case MENU_CONNECTION:
                        currentConnection = (currentConnection == NO_CAMBIO) ? CONFIGURAR : NO_CAMBIO;
                        break;
                    default:
                        break;
                }
            } else {
                currentOption = (MenuOption)((currentOption + 1) % MENU_OPTIONS_COUNT);
            }
            DrawMenu(currentOption, editingValue);
            lastPressTime = currentTime; // Actualiza el tiempo de la última acción
            isRepeating = true;          // Activa el modo de repetición
        }
    } else {
        isRepeating = false;   // Reinicia la repetición si la tecla no está presionada
        lastPressTime = 0;     // Reinicia el tiempo de la última acción
    }

    // Lógica de confirmación o entrada en modo edición (sin cambios)
    if (digitalRead(PIN_MENU) == 0 && !editingValue) {
        if (currentOption == MENU_EXIT) {
            exitMenu = true;
        } else {
            editingValue = true;
        }
        while (digitalRead(PIN_MENU) == 0); // Espera a que se suelte el botón
        DrawMenu(currentOption, editingValue);
    }

    if (digitalRead(PIN_MENU) == 0 && editingValue) {
        if (currentOption == MENU_CONNECTION) {
            if (currentConnection == CONFIGURAR) {
                preferences.begin("config", false);
                preferences.putBool("cambiar", currentConnection);
                preferences.end();
                ESP.restart();
            }
        }
        editingValue = false;
        while (digitalRead(PIN_MENU) == 0); // Espera a que se suelte el botón
        DrawMenu(currentOption, editingValue);
    }
}


void display_menu() {
    Serial.println("Iniciando menú de configuración...");
    currentOption = MENU_LANGUAGE;
    // Dibuja el menú inicial
    UpdateLanguageTexts();
    bool editingValue = false;
    bool exitMenu = false;
    DrawMenu(currentOption, editingValue);
    // espera a que suelte la tecla CONF
    while (digitalRead(PIN_MENU) == 0);
    while (!exitMenu) {
        UpdateMenuSelection(editingValue, exitMenu);
        delay(100); // Para evitar lecturas continuas excesivas
    }
    initNVS();
    saveToNVS();
    //delay(1000); // Para evitar múltiples pulsaciones
    readFromNVS();
    //delay(1000); // Para evitar múltiples pulsaciones
    closeNVS();
    display = 1;
}

void UpdateLanguageTexts() {
    if (currentLanguage == LANGUAGE_SPANISH) {
        menuLanguage = "IDIOMA";
        menuPower = "POTENCIA CONTRATADA";
        menuMaxProduction = "PRODUCCION MAXIMA";
        menuLimiter = "LIMITADOR";
        menuConnection = "CONEXION WIFI/MQTT";
        menuExit = "SALIR DE CONFIGURACION";


        produccion = "PRODUCCION";
        consumo =    "  CONSUMO ";
        limitador =  " LIMITADOR ";

        titulo0 = "     ULTIMAS 24 HORAS     "; // 26 caracteres
        titulo1 = "           HOY            "; // 26 caracteres
        titulo2 = "       TIEMPO REAL        "; // 26 caracteres

        mens_connecting =     "              CONECTANDO A WIFI...                ";
        mens_connected =      "               CONECTADO A WIFI                   ";
        mens_failed =         "           FALLO AL CONECTAR A WIFI               ";
        mens_mqtt_connecting ="              CONECTANDO A MQTT...                ";
        mens_mqtt_connected = "               CONECTADO A MQTT                   ";
        mens_mqtt_failed =    "           FALLO AL CONECTAR A MQTT               ";
        mens_abrirwifimovil = "      CONECTE AL WIFI: MICROPIC-ENERGYMETER       ";
        mens_navegador =      "      Y NAVEGUE HASTA LA PAGINA 192.168.4.1       ";


    } else if (currentLanguage == LANGUAGE_ENGLISH) {
        menuLanguage = "LANGUAGE";
        menuPower = "CONTRACTED POWER";
        menuMaxProduction = "MAXIMUM PRODUCTION";
        menuLimiter = "LIMITER";
        menuConnection = "WIFI/MQTT CONNECTION";
        menuExit = "EXIT CONFIGURATION";


        produccion = "PRODUCTION";
        consumo =    "CONSUMPTION";
        limitador =  "  LIMITER ";

        titulo0 = "   LAST 24 HOURS ENERGY   "; // 26 caracteres
        titulo1 = "          TODAY           "; // 26 caracteres
        titulo2 = "        REAL TIME         "; // 26 caracteres

        mens_connecting =     "              CONNECTING TO WIFI...               ";
        mens_connected =      "               CONNECTED TO WIFI                  ";
        mens_failed =         "           FAILED TO CONNECT TO WIFI              ";
        mens_mqtt_connecting ="              CONNECTING TO MQTT...               ";
        mens_mqtt_connected = "               CONNECTED TO MQTT                  ";
        mens_mqtt_failed =    "           FAILED TO CONNECT TO MQTT              ";
        mens_abrirwifimovil = "      CONNECT TO WIFI: MICROPIC-ENERGYMETER       ";
        mens_navegador =      "      AND NAVIGATE TO PAGE 192.168.4.1            ";

    }
}