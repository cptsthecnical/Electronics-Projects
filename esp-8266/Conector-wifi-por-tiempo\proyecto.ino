#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// En este proyecto solo se va a conectar al Wifi a las 10:00 y cuando termine la tarea programada se saldrá

const char* ssid = "TuSSID";
const char* password = "TuPassword";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600); // UTC+1 España
bool taskDone = false;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.println("OK");
    timeClient.begin();
    timeClient.update();
    WiFi.mode(WIFI_OFF); // 🔹 Apaga WiFi completamente tras sincronizar hora
  } else {
    Serial.println("Fallo conexión inicial");
    WiFi.mode(WIFI_OFF);
  }
}

void loop() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 60000) {  // chequeo cada minuto
    lastCheck = millis();

    timeClient.update(); // mantiene la hora interna del cliente NTP (sin reconectar)

    int hour = timeClient.getHours();
    int minute = timeClient.getMinutes();

    // Solo ejecutar a las 10:00 exactas
    if (hour == 10 && minute == 0 && !taskDone) {
      Serial.println("→ Activando WiFi para ejecutar tarea");

      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        Serial.println("WiFi conectado. Ejecutando tarea...");

        // --- Tarea programada ---
        // Aquí pones lo que necesites (HTTP, MQTT, etc)
        delay(5000); // Simulación de tarea

        Serial.println("Tarea finalizada. Apagando WiFi...");
      }
      WiFi.mode(WIFI_OFF);  // 🔹 Apagar WiFi 100%
      taskDone = true;      // Evita repetir durante esa hora
    }

    // Reset del flag cuando cambia la hora
    if (hour != 10) taskDone = false;
  }
}
