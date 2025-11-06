#include <Wire.h>
#include <RTClib.h>
#include <ESP8266WiFi.h> // Para ESP8266

const char* ssid = "TU_SSID";
const char* password = "TU_CONTRASEÑA";

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  rtc.begin();

  // Iniciar el RTC si es la primera vez (ajusta la fecha y la hora aquí)
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(2025, 1, 1, 0, 0, 0));  // Ajustar la fecha y hora inicial
  }

  while (true) {
    DateTime now = rtc.now();
    if (true) {
      conectarWiFi();
      realizarTrabajo();
      desconectarWiFi();
    }
    Serial.println("Entrando en modo de sueño profundo...");
    ESP.deepSleep(3600000 * 1000 * 2); // Esperar 2 horas (en microsegundos)
  }
}

void loop() {
  // El loop principal se queda vacío
}

void conectarWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando al WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Conectado");
}

void realizarTrabajo() {
  Serial.println("Realizando trabajo...");
}

void desconectarWiFi() {
  WiFi.disconnect();
  Serial.println("Desconectado del WiFi");
}
