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
  {"192.168.1.75", "PROXMOX", false},
  {"192.168.1.128", "TV-SALÓN", false},
  {"192.168.1.129", "PC-MILITAR-WIFI", false},
  {"192.168.1.130", "PORTATIL-AIR", false},
  {"192.168.1.134", "ANDROID", false},
  {"192.168.1.139", "TV-HABITACIÓN", false},
  {"192.168.1.131", "IPHONE", false}
};
const int numHosts = sizeof(hosts) / sizeof(hosts[0]);

// === OBJETO SMTP (¡ELIMINADO GLOBALMENTE!) ===
// Ya NO declaramos SMTPSession globalmente. Ahora se hace localmente
// dentro de la función enviarCorreo() para garantizar un estado limpio.

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

void verificarHosts() {
  Serial.println("🧐 Verificando estado de los hosts...");
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

void enviarCorreo() {
  // === CAMBIO CRUCIAL: Declaramos SMTPSession localmente ===
  // Esto garantiza que la sesión se inicialice correctamente en cada reinicio.
  SMTPSession smtp;

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

  Serial.println("📧 Intentando conectar al servidor SMTP y enviar correo...");
  if (!smtp.connect(&session)) {
    Serial.println("❌ Error conectando al servidor SMTP.");
    // Añadido para mejor depuración:
    Serial.println("Razón del error: " + smtp.errorReason());
    return;
  }

  // Usamos el objeto smtp local para enviar el correo
  if (!MailClient.sendMail(&smtp, &mail)) {
    Serial.println("❌ Error enviando correo: " + smtp.errorReason());
  } else {
    Serial.println("✅ Correo enviado correctamente.");
  }

  // La sesión se cerrará automáticamente, pero la cerramos explícitamente para limpiar.
  smtp.closeSession(); 
}

void dormir2Horas() {
  const unsigned long tiempo_segundos = 90 * 60; // 90 minutos
  const unsigned long tiempo_microsegundos = tiempo_segundos * 1000000UL;

  Serial.printf("😴 Preparando para dormir %.2f horas (%lu segundos)...\n", 
    tiempo_segundos / 3600.0, tiempo_segundos);

  WiFi.disconnect(true); // Apaga WiFi completamente
  delay(1000);
  
  // Usamos el valor calculado con aritmética de enteros para mayor robustez
  ESP.deepSleep(tiempo_microsegundos);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n🚀 Iniciando ESP8266 Watcher...");

  conectarWiFi();
  verificarHosts();
  enviarCorreo();
  dormir2Horas();
}

void loop() {
  // No se usa. Todo ocurre en setup() tras cada reinicio del deep sleep.
}
