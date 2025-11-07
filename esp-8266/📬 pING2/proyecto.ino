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
//     - 🌙 Entra en modo de sueño profundo durante 2 horas
//       con `ESP.deepSleep(2 * 60 * 60 * 1000000UL);`
// 5️⃣ 🔁 Pasadas esas 2 horas, el chip se reinicia automáticamente
//     y repite todo el proceso desde el punto 1.

// 🧠 He cambiado las dos horas progeramadas por 01:30 para que sea más fluido

// ===== ARRAY DE HOSTS =====
struct Host {
  const char* ip;
  const char* name;
  bool isUp;
};

Host hosts[] = {
  {"192.168.1.1", "ROUTER-DIGI", false},
  {"192.168.1.75", "PROXMOX", false},
  {"192.168.1.128", "TV-SALÓN", false},
  {"192.168.1.129", "PC-MILITAR-WIFI", false},
  {"192.168.1.130", "PORTATIL-AIR", false},
  {"192.168.1.134", "ANDROID", false},
  {"192.168.1.139", "TV-HABITACIÓN", false},
  {"192.168.1.131", "IPHONE", false}
};
const int numHosts = sizeof(hosts) / sizeof(hosts[0]);

// ===== OBJETO SMTP =====
SMTPSession smtp;

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

/*
void obtenerHoraNTP() {
  configTime(3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  int intentos = 0;
  while (now < 1000000000 && intentos < 10) { // Esperar a sincronizar
    delay(500);
    now = time(nullptr);
    intentos++;
  }
  if (now > 1000000000) {
    Serial.println("🕒 Hora NTP sincronizada correctamente.");
    Serial.println(ctime(&now));
  } else {
    Serial.println("⚠️ No se pudo obtener la hora NTP.");
  }
}
*/

void verificarHosts() {
  Serial.println("🔍 Verificando estado de los hosts...");
  for (int i = 0; i < numHosts; i++) {
    bool online = Ping.ping(hosts[i].ip, 3);
    hosts[i].isUp = online;
    Serial.printf("%s (%s): %s\n",
      hosts[i].name,
      hosts[i].ip,
      online ? "🟢 ONLINE" : "🔴 OFFLINE");
  }
}

void enviarCorreo() {
  String subject = "📬 [ESP8266] Reporte de pING2 en estado de red local";
  String message = "\n==============================================\n";
  message += "\n🕒 [ESTADO DE LOS HOST LOCAL]\n";

  for (int i = 0; i < numHosts; i++) {
    message += String(hosts[i].name) + " (" + hosts[i].ip + "): " +
               (hosts[i].isUp ? "🟢 ONLINE" : "🔴 OFFLINE") + "\n";
  }

  obtenerInfoESP(message);
  
  SMTP_Message mail;
  mail.sender.name = "ESP8266 Watcher";
  mail.sender.email = AUTHOR_EMAIL;
  mail.subject = subject;
  mail.addRecipient("Admin", RECIPIENT_EMAIL);
  mail.text.content = message;

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.secure.startTLS = true;

  Serial.println("📧 Enviando correo...");
  if (!smtp.connect(&session)) {
    Serial.println("❌ Error conectando al servidor SMTP.");
    return;
  }

  if (!MailClient.sendMail(&smtp, &mail)) {
    Serial.println("❌ Error enviando correo: " + smtp.errorReason());
  } else {
    Serial.println("✅ Correo enviado correctamente.");
  }

  smtp.closeSession();
}

void obtenerInfoESP(String &message) {
  message += "\n==============================================\n";
  message += "\n⚠️ [INFORME DE SISTEMA ESP8266]\n";
  message += "Chip ID: " + String(ESP.getChipId()) + "\n";
  message += "Flash total: " + String(ESP.getFlashChipRealSize() / 1024) + " KB\n";
  message += "Flash usado: " + String(ESP.getSketchSize() / 1024) + " KB\n";
  message += "Flash libre: " + String((ESP.getFlashChipRealSize() - ESP.getSketchSize()) / 1024) + " KB\n";
  message += "RAM libre: " + String(ESP.getFreeHeap() / 1024) + " KB\n";
  message += "SDK: " + String(ESP.getSdkVersion()) + "\n";
  message += "Tiempo activo para ping: " + String(millis() / 60000.0, 1) + " min\n";
  message += "RSSI WiFi: " + String(WiFi.RSSI()) + " dBm\n";
}

void dormir2Horas() {
  Serial.println("😴 Preparando para dormir 01:30 horas...");
  WiFi.disconnect(true); // Apaga WiFi completamente
  delay(1000);
  // ESP.deepSleep(2 * 60 * 60 * 1000000UL);  // 02:00 horas en microsegundos
  // ESP.deepSleep(1 * 60 * 60 * 1000000UL);  // 01:00 horas en microsegundos
  ESP.deepSleep(1.5 * 60 * 60 * 1000000UL);   // 01:30 horas en microsegundos
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n🚀 Iniciando ESP8266 Watcher...");

  conectarWiFi();
  //obtenerHoraNTP();
  verificarHosts();
  enviarCorreo();
  dormir2Horas();
}

void loop() {
  // No se usa. Todo ocurre en setup() tras cada reinicio del deep sleep.
}
