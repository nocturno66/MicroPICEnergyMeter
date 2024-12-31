
// https://elecrow.com/wiki/CrowPanel_ESP32_E-Paper_5.79inch_Arduino_Tutorial.html
#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
//#include <Fonts/FreeMonoBold9pt7b.h>
#include "fondo.h"
#include "enchufe.h"
#include "fuego.h"
#include "exportar.h"
#include "importar.h"

#include <PubSubClient.h>

// Define the black and white image array as the buffer for the e-paper display
uint8_t ImageBW[27200];  // Define the buffer size according to the resolution of the e-paper display

const char* ssid = "NocturnoDeco";       // WiFi network name
const char* password = "d4fea61aa3";     // WiFi password
// Define MQTT parameters
const char* mqtt_server = "192.168.0.224";  // MQTT broker address
const char* mqtt_topic = "wp_produccion";   // Topic to subscribe
const char* mqtt_user = "nocturno";         // MQTT username
const char* mqtt_password = "Esdru_Jula66"; // MQTT password

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
unsigned int v_tempext;
unsigned int v_tempint;
unsigned int v_humedad;
unsigned int v_particulas;
unsigned int v_litros;
float v_tempagua;
// Variables adicionales para la fecha y la hora
String fecha;
String hora;

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
    v_produccion = int(myObject["v_produccion"]);
    v_consumo = int(myObject["v_consumo"]);
    v_exporimpor = int(myObject["v_exporimpor"]);
    v_tempext = int(myObject["v_tempext"]);
    v_tempint = int(myObject["v_tempint"]);
    v_humedad = int(myObject["v_humedad"]);
    v_particulas = int(myObject["v_particulas"]);
    v_litros = int(myObject["v_litros"]);
    v_tempagua = static_cast<float>(double(myObject["v_tempagua"]));

    // Extraer la fecha y la hora sin comillas
    fecha = JSON.stringify(myObject["fecha"]);
    fecha.replace("\"", ""); // Elimina las comillas

    hora = JSON.stringify(myObject["hora"]);
    hora.replace("\"", ""); // Elimina las comillas

    // Depuración
    Serial.println("Fecha: " + fecha);
    Serial.println("Hora: " + hora);



    // Depuración: Imprimir los valores
    Serial.println("Valores actualizados:");
    Serial.printf("Producción: %u\nConsumo: %u\nExportación/Importación: %u\n", v_produccion, v_consumo, v_exporimpor);
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

// Function to display weather forecast
void actualiza_info()
{
  char buffer[40];  // Create a character array to store information

  // Clear the image and initialize the e-ink screen
  Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE); // Create a new image
  Paint_Clear(WHITE); // Clear the image content
  EPD_FastMode1Init(); // Initialize the e-ink screen
  EPD_Display_Clear(); // Clear the screen display
  EPD_Update(); // Update the screen
  EPD_Clear_R26A6H(); // Clear the e-ink screen cache

  // Display the image
  EPD_ShowPicture(0, 0, 792, 272, gImage_fondo, WHITE); // Display the background image

  EPD_ShowPicture(625, 120, 64, 52, gImage_enchufe, WHITE);
  EPD_ShowPicture(100, 170, 64, 40, gImage_exportar, WHITE);

  // Mostrar fecha y hora
  snprintf(buffer, sizeof(buffer), "%s", fecha.c_str());
  EPD_ShowString(10, 1, buffer, 24, BLACK);

  snprintf(buffer, sizeof(buffer), "%s", hora.c_str());
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

  // Update the e-ink screen display content
  EPD_Display(ImageBW); // Display the image
  EPD_PartUpdate(); // Partially update the screen
  //EPD_DeepSleep(); // Enter deep sleep mode
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

  v_produccion = 1500;
  v_consumo = 1000;
  v_exporimpor = 500;
  v_tempext = 25;
  v_tempint = 20;
  v_humedad = 50;
  v_particulas = 30;
  v_litros = 100;
  v_tempagua = 68.2;  

}




void loop() {
   // Reconnect to MQTT if disconnected
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop(); // Process incoming MQTT messages

  actualiza_info();
  delay(3000); 
  v_produccion++;
}



/*#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <SPI.h>

// Configuración personalizada para tu pantalla
// Sustituye GxEPD2_154_D67 por el controlador real de tu pantalla
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(45,  46,  47,  48));


const char HelloWorld[] = "Manolo Jimenez";

void helloWorld()
{
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; 
  uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // Centrar el texto en la pantalla
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  Serial.println(tbx);
  Serial.println(tby);
  Serial.println(tbw);
  Serial.println(tbh);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloWorld);
  }
  while (display.nextPage());
}

void setup()
{
  // Inicializa SPI con los pines personalizados
  SPI.begin(12,  -1,  11);

  // Inicializa la pantalla
  display.init(115200); // Velocidad del SPI

  display.setRotation(0); // Configuración de rotación si es necesario

  // Ejemplo de dibujo básico
  display.setFullWindow(); // Usa toda la pantalla
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE); // Fondo blanco
    display.setCursor(10, 20);
    display.print("Pantalla configurada!");
  } while (display.nextPage());

  helloWorld(); // Mostrar el mensaje de ejemplo
  display.hibernate(); // Poner la pantalla en modo de bajo consumo
}

void loop() {
  // Código principal
}
*/