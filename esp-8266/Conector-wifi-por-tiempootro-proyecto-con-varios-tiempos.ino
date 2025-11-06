#include <ESP8266WiFi.h>
#include <TimeLib.h>

const char* ssid = "TU_SSID";
const char* password = "TU_CONTRASEÑA";

void setup() {
  Serial.begin(115200);
  setTime(0, 0, 0, 1, 1, 2025); // Establecer hora inicial si usas un RTC

  // Ejecutar el ciclo de trabajo
  while (true) {
    int hora = hour();
    if (debeConectarse(hora)) {
      conectarWiFi();
      realizarTrabajo();
      desconectarWiFi();
    }
    delay(5000); // Verificar cada 5 segundos
  }
}

void loop() {
  // El loop principal se queda vacío
}

bool debeConectarse(int hora) {
  return (hora == 10 || hora == 12 || hora == 16 || hora == 18 || hora == 20 || hora == 22);
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
