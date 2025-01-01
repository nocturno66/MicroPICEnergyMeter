
// https://elecrow.com/wiki/CrowPanel_ESP32_E-Paper_5.79inch_Arduino_Tutorial.html
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
#include "secrets.h"
#include <PubSubClient.h>

// Define the black and white image array as the buffer for the e-paper display
uint8_t ImageBW[27200];  // Define the buffer size according to the resolution of the e-paper display

WiFiClient espClient;
PubSubClient client(espClient);

// Default timer set to 10 seconds for testing
// For the final application, set an appropriate time interval based on the hourly/minute API call limits
unsigned long lastTime = 0;                // Last update time
unsigned long timerDelay = 10000;          // Timer set to 10 seconds (10000)

// Define variables related to JSON data
String jsonBuffer;
int httpResponseCode;
JSONVar myObject;

// Define variables related to weather information
unsigned int v_produccion;
unsigned int v_consumo;
unsigned int v_exporimpor;
unsigned int v_sentido; // 0: exportación, 1: importación
unsigned int v_tempext;
unsigned int v_tempint;
unsigned int v_humedad;
unsigned int v_particulas;
unsigned int v_litros;
float v_tempagua;
unsigned int v_modo; // 0: eléctrico, 1: gas
unsigned int v_manual_automatico; // 0: manual, 1: automático

// Variables adicionales para la fecha y la hora
String fecha;
String hora;

void actualiza_display()
{
  char buffer[40];  // Create a character array to store information

  // Clear the image and initialize the e-ink screen
  Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE); // Create a new image
  Paint_Clear(WHITE); // Clear the image content

  // Display the image
  EPD_ShowPicture(0, 0, 792, 272, gImage_fondo, WHITE); // Display the background image

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

  // Mostrar fecha y hora
  snprintf(buffer, sizeof(buffer), "%s", fecha);
  EPD_ShowString(10, 1, buffer, 24, BLACK);

  snprintf(buffer, sizeof(buffer), "%s", hora);
  EPD_ShowString(150, 1, buffer, 24, BLACK);

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%lu KWh", v_produccion); 
  EPD_ShowString(120, 50, buffer, 24, BLACK); 

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%lu KWh", v_consumo); 
  EPD_ShowString(40, 130, buffer, 24, BLACK); 

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%lu KWh", v_exporimpor); 
  EPD_ShowString(120, 220, buffer, 24, BLACK); 

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%u`C", v_tempext); 
  EPD_ShowString(385, 25, buffer, 24, BLACK);

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%u`C", v_tempint);
  EPD_ShowString(385, 130, buffer, 24, BLACK);

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%u %%", v_humedad); 
  EPD_ShowString(385, 182, buffer, 24, BLACK); 

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%u %%", v_particulas); 
  EPD_ShowString(385, 220, buffer, 24, BLACK); 

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%lu litros", v_litros);
  EPD_ShowString(650, 20, buffer, 24, BLACK);

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%3.1f`C", v_tempagua); 
  EPD_ShowString(650, 50, buffer, 24, BLACK); 

  if (v_manual_automatico == 0) {
    EPD_ShowString(650, 80, "Manual", 24, BLACK);
  } else {
    EPD_ShowString(650, 80, "Automatico", 24, BLACK);
  }

  EPD_Display(ImageBW); // Display the image
  EPD_PartUpdate(); // Partially update the screen
}


 // MQTT callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  if (strcmp(topic, mqtt_topic) == 0) {
    // Crear un buffer para el payload y añadir el terminador nulo
    char jsonPayload[length + 1];
    memcpy(jsonPayload, payload, length);
    jsonPayload[length] = '\0';

    // Depuración: Imprimir el payload recibido
    Serial.print("JSON payload: ");
    Serial.println(jsonPayload);

    // Parsear el JSON
    JSONVar myObject = JSON.parse(jsonPayload);

    // Verificar si el JSON fue parseado correctamente
    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Error: JSON parsing failed!");
      return;
    }

    // Extraer los valores del JSON y asignarlos a las variables
    if (myObject.hasOwnProperty("pro")) {
        v_produccion = int(myObject["pro"]);; // producción (entero)
    }
    if (myObject.hasOwnProperty("con")) {
        v_consumo = int(myObject["con"]); // consumo (entero)
    }
    if (myObject.hasOwnProperty("exi")) {
        v_exporimpor =int(myObject["exi"]); // exportación/importación (float)
    }
    if (myObject.hasOwnProperty("sen")) {
        v_sentido = int(myObject["sen"]); // sentido (entero)
    }
    if (myObject.hasOwnProperty("tex")) {
        v_tempext = int(myObject["tex"]); // temperatura exterior (entero)
    }
    if (myObject.hasOwnProperty("tin")) {
        v_tempint = int(myObject["tin"]); // temperatura interior (entero)
    }
    if (myObject.hasOwnProperty("hum")) {
        v_humedad = int(myObject["hum"]); // humedad (entero)
    }
    if (myObject.hasOwnProperty("par")) {
        v_particulas = int(myObject["par"]); // partículas (entero)
    }
    if (myObject.hasOwnProperty("lit")) {
        v_litros = int(myObject["lit"]); // litros (entero)
    }
    if (myObject.hasOwnProperty("tag")) {
        v_tempagua = (double)myObject["tag"]; // Asignar directamente si es un número
    }
    if (myObject.hasOwnProperty("mod")) {
        v_modo = int(myObject["mod"]); // modo (entero)
    }
    // la propiedad "mau" vendrá con true o false
    if (myObject.hasOwnProperty("mau")) {
      v_manual_automatico = (bool)myObject["mau"] ? 1 : 0; // modo (entero); 1 si es true, 0 si es false
    }
        
    if (myObject.hasOwnProperty("fec")) {
        // Extraer la cadena de fecha completa
        String fechaCompleta = myObject["fec"];

        // Extraer la parte de la fecha (YYYY-MM-DD) y convertirla a DD/MM/YYYY
        fecha = fechaCompleta.substring(8, 10) + "/" + fechaCompleta.substring(5, 7) + "/" + fechaCompleta.substring(0, 4);

        // Extraer la parte de la hora (HH:MM:SS)
        int horaUTC = fechaCompleta.substring(11, 13).toInt(); // Convertir a entero
        int minutos = fechaCompleta.substring(14, 16).toInt();
        int segundos = fechaCompleta.substring(17, 19).toInt();

        int dia = fechaCompleta.substring(8, 10).toInt();
        int mes = fechaCompleta.substring(5, 7).toInt();
        int ano = fechaCompleta.substring(0, 4).toInt();

        // Determinar si es horario de verano
        bool esHorarioDeVerano = false;
        if (mes > 3 && mes < 10) {
            esHorarioDeVerano = true; // Entre abril y septiembre siempre es horario de verano
        } else if (mes == 3) {
            // Último domingo de marzo
            int ultimoDomingoMarzo = 31 - (5 * ano / 4 + 4) % 7;
            if (dia > ultimoDomingoMarzo || (dia == ultimoDomingoMarzo && horaUTC >= 2)) {
                esHorarioDeVerano = true;
            }
        } else if (mes == 10) {
            // Último domingo de octubre
            int ultimoDomingoOctubre = 31 - (5 * ano / 4 + 1) % 7;
            if (dia < ultimoDomingoOctubre || (dia == ultimoDomingoOctubre && horaUTC < 2)) {
                esHorarioDeVerano = true;
            }
        }

        // Ajustar la hora según horario de verano/invierno
        int offset = esHorarioDeVerano ? 2 : 1; // UTC+2 en verano, UTC+1 en invierno
        horaUTC += offset;

        // Ajustar el cambio de día si la hora se pasa de 23
        if (horaUTC >= 24) {
            horaUTC -= 24;
            dia++;
            // Ajustar el cambio de mes si el día supera el último día del mes
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
            // Ajustar el cambio de año si el mes se pasa de 12
            if (mes > 12) {
                mes = 1;
                ano++;
            }
        }

        // Construir la fecha ajustada
        fecha = (dia < 10 ? "0" : "") + String(dia) + "/" +
                (mes < 10 ? "0" : "") + String(mes) + "/" +
                String(ano);

        // Construir la hora ajustada
        hora = (horaUTC < 10 ? "0" : "") + String(horaUTC) + ":" +
              (minutos < 10 ? "0" : "") + String(minutos) + ":" +
              (segundos < 10 ? "0" : "") + String(segundos);

        // Imprimir los resultados para verificar
        Serial.println("Fecha ajustada: " + fecha);
        Serial.println("Hora ajustada: " + hora);
    }



    actualiza_display();

    // Depuración
    Serial.println("Fecha: " + fecha);
    Serial.println("Hora: " + hora);



    // Depuración: Imprimir los valores
    Serial.println("Valores actualizados:");
    Serial.printf("Producción: %u\nConsumo: %u\nExportación/Importación: %u\nSentido: %u", v_produccion, v_consumo, v_exporimpor, v_sentido);
    Serial.printf("Temp Ext: %u\nTemp Int: %u\nHumedad: %u\nPartículas: %u\n", v_tempext, v_tempint, v_humedad, v_particulas);
    Serial.printf("Litros: %u\nTemp Agua: %.1f\n", v_litros, v_tempagua);
    Serial.println("Fecha: " + fecha);
    Serial.println("Hora: " + hora);
  }
}

// Reconnect to MQTT broker if the connection is lost
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client",mqtt_user,mqtt_password)) { // Provide a unique client ID
      Serial.println("connected");
      client.subscribe(mqtt_topic); // Subscribe to the topic
      Serial.print("Subscribed to topic: ");
      Serial.println(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); // Initialize the serial port

  // Connect to the WiFi network
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP()); // Print the IP address after successful connection

  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");

  // Initialize MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  // Set the screen power pin to output mode and set it to high level to turn on the power
  pinMode(7, OUTPUT);  // Set GPIO 7 to output mode
  digitalWrite(7, HIGH);  // Set GPIO 7 to high level to turn on the power

  // Initialize the e-paper display
  EPD_GPIOInit();  // Initialize the GPIO pins of the e-paper
  EPD_FastMode1Init(); // Initialize the e-ink screen
  EPD_Display_Clear(); // Clear the screen display
  EPD_Update(); // Update the screen
  EPD_Clear_R26A6H();

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
  actualiza_display();
}


void loop() {
   // Reconnect to MQTT if disconnected
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop(); // Process incoming MQTT messages
  delay(10);   
}
