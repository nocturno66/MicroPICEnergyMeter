#include "display_utils.h"
#include "secrets.h"
#include "pic.h"


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

// Define variables related to weather information
String weather;
String temperature;
String humidity;
String sea_level;
String wind_speed;
String city_js;
int weather_flag = 0;  
// Define variables related to JSON data
String jsonBuffer;
int httpResponseCode;
JSONVar myObject;

unsigned int display = 0;   // '0' for Wall Panel Home Assistant, '1' for openweather

WiFiClient espClient;
PubSubClient client(espClient);

void inicializa_display() {
    // Set the screen power pin to output mode and set it to high level to turn on the power
    pinMode(7, OUTPUT);  // Set GPIO 7 to output mode
    digitalWrite(7, HIGH);  // Set GPIO 7 to high level to turn on the power

    pinMode(1, INPUT);  // Set GPIO 1 to input mode EXIT
    pinMode(2, INPUT);  // Set GPIO 2 to input mode MENU
    pinMode(4, INPUT);  // Set GPIO 4 to input mode DOWN
    pinMode(5, INPUT);  // Set GPIO 5 to input mode CONF
    pinMode(6, INPUT);  // Set GPIO 6 to input mode UP

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
}

void actualiza_display() {
    char buffer[40];

    if (display==0) { // Wall Panel Home Assistant
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
    } else if (display==1) {
        // openweather

        // Clear the image and initialize the e-ink screen
        Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE); // Create a new image
        Paint_Clear(WHITE); // Clear the image content

        // Display the image
        EPD_ShowPicture(0, 0, 792, 272, pic_original, WHITE); // Display the background image

        // Display the corresponding weather icon based on the weather icon flag
        EPD_ShowPicture(4, 3, 432, 184, Weather_Num[weather_flag], WHITE);
        
        // Draw partition lines
        EPD_DrawLine(0, 190, 792, 190, BLACK); // Draw a horizontal line
        EPD_DrawLine(530, 0, 530, 270, BLACK); // Draw a vertical line

        // Display the update time
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s ", city_js); // Format the update time as a string
        EPD_ShowString(620, 60, buffer, 24, BLACK); // Display the update time

        // Display the temperature
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s C", temperature); // Format the temperature as a string
        EPD_ShowString(340, 240, buffer, 24, BLACK); // Display the temperature

        // Display the humidity
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s ", humidity); // Format the humidity as a string
        EPD_ShowString(620, 150, buffer, 24, BLACK); // Display the humidity

        // Display the wind speed
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s m/s", wind_speed); // Format the wind speed as a string
        EPD_ShowString(135, 240, buffer, 24, BLACK); // Display the wind speed

        // Display the sea level pressure
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s ", sea_level); // Format the sea level pressure as a string
        EPD_ShowString(620, 240, buffer, 24, BLACK); // Display the sea level pressure*/

        // Update the e-ink screen display content
        EPD_Display(ImageBW); // Display the image
        EPD_PartUpdate(); // Partially update the screen
    } else if (display==2) {
        // Clear the image and initialize the e-ink screen
        Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE); // Create a new image
        Paint_Clear(WHITE); // Clear the image content
        
        // Display the image
        EPD_ShowPicture(0, 0, 792, 272, pic, WHITE); // Display the background image
        // Update the e-ink screen display content
        EPD_Display(ImageBW); // Display the image
        EPD_PartUpdate(); // Partially update the screen

    }

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



// Define the HTTP GET request function
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Initialize the HTTP client and specify the requested server URL
  http.begin(client, serverName);

  // Send an HTTP GET request
  httpResponseCode = http.GET();

  // Initialize the returned response content
  String payload = "{}";

  // Check the response code and process the response content
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode); // Print the response code
    payload = http.getString(); // Get the response content
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode); // Print the error code
  }
  // Release the HTTP client resources
  http.end();

  return payload; // Return the response content
}

void js_analysis()
{
  // Check if successfully connected to the WiFi network
  if (WiFi.status() == WL_CONNECTED) {
    // Build the OpenWeatherMap API request URL
    // OpenWeatherMap API key
    String  openWeatherMapApiKey = "5307f05ae8c33c1ffe4192c80d79f95c";
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=Coria,2519233&APPID=" + openWeatherMapApiKey + "&units=metric";

    // Loop until a valid HTTP response code of 200 is obtained
    while (httpResponseCode != 200) {
      // Send an HTTP GET request and get the response content
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer); // Print the obtained JSON data
      myObject = JSON.parse(jsonBuffer); // Parse the JSON data

      // Check if JSON parsing was successful
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!"); // Error message when parsing fails
        return; // Exit the function if parsing fails
      }
      delay(2000); // Wait for 2 seconds before retrying
    }

    // Extract weather information from the parsed JSON data
    weather = JSON.stringify(myObject["weather"][0]["main"]); // Weather main information
    temperature = JSON.stringify(myObject["main"]["temp"]); // Temperature
    humidity = JSON.stringify(myObject["main"]["humidity"]); // Humidity
    sea_level = JSON.stringify(myObject["main"]["sea_level"]); // Sea level pressure
    wind_speed = JSON.stringify(myObject["wind"]["speed"]); // Wind speed
    city_js = JSON.stringify(myObject["name"]); // City name

    // Print the extracted weather information
    Serial.print("String weather: ");
    Serial.println(weather);
    Serial.print("String Temperature: ");
    Serial.println(temperature);
    Serial.print("String humidity: ");
    Serial.println(humidity);
    Serial.print("String sea_level: ");
    Serial.println(sea_level);
    Serial.print("String wind_speed: ");
    Serial.println(wind_speed);
    Serial.print("String city_js: ");
    Serial.println(city_js);

    // Set the weather icon flag based on the weather description
    if (weather.indexOf("clouds") != -1 || weather.indexOf("Clouds") != -1) {
      weather_flag = 1; // Cloudy
    } else if (weather.indexOf("clear sky") != -1 || weather.indexOf("Clear sky") != -1) {
      weather_flag = 3; // Clear sky
    } else if (weather.indexOf("rain") != -1 || weather.indexOf("Rain") != -1) {
      weather_flag = 5; // Rainy
    } else if (weather.indexOf("thunderstorm") != -1 || weather.indexOf("Thunderstorm") != -1) {
      weather_flag = 2; // Thunderstorm
    } else if (weather.indexOf("snow") != -1 || weather.indexOf("Snow") != -1) {
      weather_flag = 4; // Snowy
    } else if (weather.indexOf("mist") != -1 || weather.indexOf("Mist") != -1) {
      weather_flag = 0; // Foggy
    }
  }
  else {
    // Print a message if the WiFi connection is lost
    Serial.println("WiFi Disconnected");
  }
}