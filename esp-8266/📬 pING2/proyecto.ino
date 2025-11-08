#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <ESP8266Ping.h>

// =====================
// CONFIGURACIÓN RED
// =====================
const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASSWORD = "TU_PASSWORD";

// =====================
// CONFIGURACIÓN CORREO
// =====================
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 587
#define AUTHOR_EMAIL "TU_CORREO@gmail.com"
#define AUTHOR_PASSWORD "TU_APP_PASSWORD"
#define RECIPIENT_EMAIL "DESTINO@gmail.com"

//===== LÓGICA =========
// 👍 Explicación general del comportamiento:
//
// 1️⃣ Al encenderse o despertarse del modo deep sleep:
//     - 🔌 El ESP8266 arranca desde cero (como si lo conectaras por primera vez).
//     - 🧠 Inicializa el sistema serie y las variables.
//     - 📶 Se conecta al WiFi para tener conectividad.
// 2️⃣ 📡 Realiza un ping a todos los dispositivos definidos en el array “hosts”
//     y guarda si cada uno está ONLINE (responde al ping) u OFFLINE.
// 3️⃣ 📧 Crea un correo con el reporte del estado de cada host
//     y lo envía usando el servidor SMTP configurado (Gmail, etc.).
// 4️⃣ 😴 Tras confirmar el envío:
//     - 🔕 Se desconecta completamente del WiFi con `WiFi.disconnect(true)`
//       (esto apaga el chip de radio WiFi del ESP8266, 0 emisiones).
//     - 🌙 Entra en modo de sueño profundo y se reinicia.
// 5️⃣ 🔁 Pasadas esas horas, el chip se reinicia automáticamente
//     y repite todo el proceso desde el punto 1.

// ===== ARRAY DE HOSTS =====
struct Host {
  const char* ip;
  const char* name;
  bool isUp;
};

Host hosts[] = {
  {"192.168.1.1", "ROUTER-DIGI", false},
  {"192.168.1.2", "TV-SALÓN", false},
  {"192.168.1.3", "PC-MILITAR", false},
  {"192.168.1.4", "PORTATIL-AIR-WIFI", false},
  {"192.168.1.5", "PROXMOX", false},
  {"192.168.1.6", "ANDROID-WIFI", false},
  {"192.168.1.7", "IPHONE-WIFI", false},
  {"192.168.1.133", "TV-HABITACIÓN-WIFI", false}
};
const int numHosts = sizeof(hosts) / sizeof(hosts[0]);

// ===== FUNCIONES =====
void conectarWiFi() {
  Serial.println("Conectando al WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado al WiFi");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
}

void apagarWiFi() {
  Serial.println("📴 Apagando WiFi...");
  WiFi.disconnect(true);    // Desconecta de la red
  WiFi.mode(WIFI_OFF);      // Apaga WiFi
  WiFi.forceSleepBegin();   // Desactiva radio
  delay(1);                 // Espera breve
  Serial.println("✅ WiFi apagado, no se emiten ondas.");
}

void verificarHosts() {
  Serial.println("Verificando estado de los hosts...");
  for (int i = 0; i < numHosts; i++) {
    bool online = Ping.ping(hosts[i].ip, 3);
    hosts[i].isUp = online;
    Serial.printf("%s (%s): %s\n",
      hosts[i].name,
      hosts[i].ip,
      online ? "ONLINE" : "OFFLINE");
  }
}

void obtenerInfoESP(String &message) {
  message += "\n🗂️ [INFORME DE SISTEMA ESP8266]:\n\n";
  message += "Chip ID: " + String(ESP.getChipId()) + " · Flash: " + String(ESP.getSketchSize() / 1024) + "/" + String(ESP.getFlashChipRealSize() / 1024) + " KB · RAM libre: " + String(ESP.getFreeHeap() / 1024) + " KB · SDK: " + String(ESP.getSdkVersion()) + " · Tiempo activo: " + String(millis() / 60000.0, 1) + " min · RSSI WiFi: " + String(WiFi.RSSI()) + " dBm\n";
}

void enviarCorreo() {
  SMTPSession smtp;

  String subject = "📬 [ESP8266] Reporte de pING2 en estado de red local";
  String message = "\n☎️ [ESTADO DE LOS HOST LOCAL]:\n\n";

  for (int i = 0; i < numHosts; i++) {
    message += String(hosts[i].name) + " (" + hosts[i].ip + "): " +
               (hosts[i].isUp ? "🟢 ONLINE" : "🔴 OFFLINE") + "\n";
  }

  obtenerInfoESP(message);
  
  SMTP_Message mail;
  mail.sender.name = "ESP8266 pING2";
  mail.sender.email = AUTHOR_EMAIL;
  mail.subject = subject;
  mail.addRecipient("Admin", RECIPIENT_EMAIL);
  mail.text.content = message;

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD; // <--- corregido
  session.secure.startTLS = true;

  Serial.println("📧 Intentando conectar al servidor SMTP y enviar correo...");
  if (!smtp.connect(&session)) {
    Serial.println("❌ Error conectando al servidor SMTP.");
    Serial.println("Razón del error: " + smtp.errorReason());
    return;
  }

  if (!MailClient.sendMail(&smtp, &mail)) {
    Serial.println("❌ Error enviando correo: " + smtp.errorReason());
  } else {
    Serial.println("✅ Correo enviado correctamente.");
  }

  smtp.closeSession();
}

void setup() {
  Serial.begin(11520);
  delay(500);
  Serial.println("\n🚀 Iniciando ESP8266 pING2 Watcher...");
  conectarWiFi();
}

void loop() {
  conectarWiFi();     // Asegura conexión al inicio de cada ciclo
  verificarHosts();
  enviarCorreo();
  apagarWiFi();       // Apagar si quieres, pero solo después de enviar

  Serial.println("⏱ Esperando 1 minuto antes del siguiente reporte...");
  // const unsigned long TIEMPO_ESPERA = 1000 * 60; // 1 minuto
  const unsigned long TIEMPO_ESPERA = 1000UL * 60 * 60; // 1 hora
  delay(TIEMPO_ESPERA);
}
