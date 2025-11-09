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
// 1️⃣ Al iniciar el ESP8266:
//     - 🔌 Arranca desde cero y se inicializa la comunicación serie.
//     - 🧠 Se configuran las variables y el array de hosts a monitorizar.
//     - 📶 Configura y conecta la red WiFi usando una IP estática definida
//       (por ejemplo 192.168.1.254), con máscara, gateway y DNS opcionales.
// 2️⃣ 📡 Verifica la disponibilidad de cada host:
//     - Hace un ping a todos los dispositivos definidos en el array “hosts”.
//     - Actualiza el estado `isUp` de cada host: ONLINE 🟢 si responde, OFFLINE 🔴 si no.
// 3️⃣ 💾 Obtiene telemetría del ESP8266:
//     - Chip ID, memoria flash, RAM libre, SDK, tiempo activo y nivel de señal WiFi.
// 4️⃣ 📧 Crea y envía un correo con el reporte:
//     - Incluye el estado de todos los hosts y la telemetría del dispositivo.
//     - Se conecta al servidor SMTP configurado (Gmail u otro).
//     - Envía el correo al destinatario definido.
// 5️⃣ 🔕 Tras enviar el correo:
//     - Se desconecta completamente del WiFi con `WiFi.disconnect(true)`.
//     - Apaga el chip de radio WiFi con `WiFi.mode(WIFI_OFF)` y `WiFi.forceSleepBegin()`.
//     - Se espera el tiempo definido (`TIEMPO_ESPERA`, por ejemplo 1 hora) antes del siguiente ciclo.
// 6️⃣ 🔁 Al finalizar la espera:
//     - Se repite el ciclo desde el punto 1, asegurando que la red, los hosts y la telemetría
//       se revisen periódicamente y se envíen los reportes automáticamente.
// 🧠 Nota: Se usa IP estática para garantizar que el ESP8266 tenga siempre la misma dirección
//     en la red, facilitando reglas de firewall o monitoreo fijo de dispositivos.

// ===== ARRAY DE HOSTS =====
struct Host {
  const char* ip;
  const char* name;
  bool isUp;
};
// 192.168.1.255
Host hosts[] = {
  {"192.168.1.1", "ROUTER-DIGI", false},
  {"192.168.1.2", "TV-SALÓN", false},
  {"192.168.1.3", "PC-MILITAR", false},
  {"192.168.1.4", "PORTATIL-AIR-WIFI", false},
  {"192.168.1.5", "PROXMOX", false},
  {"192.168.1.6", "ANDROID-WIFI", false},
  {"192.168.1.7", "IPHONE-WIFI", false},
  {"192.168.1.8", "PORTATIL-WINDOWS-WIFI", false},
  {"192.168.1.128", "TV-HABITACIÓN-WIFI", false}
};
const int numHosts = sizeof(hosts) / sizeof(hosts[0]);

// ===== FUNCIONES =====
void conectarWiFi() {
  Serial.println("Conectando al WiFi...");

  // --- CONFIGURAR IP ESTÁTICA ---
  IPAddress local_IP(192, 168, 1, 254);    // 🔹 IP fija que le asignas al ESP8266
  IPAddress gateway(192, 168, 1, 1);       // 🔹 Puerta de enlace (normalmente tu router)
  IPAddress subnet(255, 255, 255, 0);      // 🔹 Máscara de subred
  IPAddress primaryDNS(8, 8, 8, 8);        // (opcional) DNS primario
  IPAddress secondaryDNS(8, 8, 4, 4);      // (opcional) DNS secundario

  // Configurar red con IP fija
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("⚠️ Error al configurar IP estática");
  }

  // --- CONECTAR A LA RED ---
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Conectado al WiFi");
  Serial.print("IP local asignada: ");
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
  message += "\n\n🕰️ Esperando 01:00 horas antes del siguiente reporte...\n";
  message += "\n💾 Chip ID: " + String(ESP.getChipId()) + " · Flash: " + String(ESP.getSketchSize() / 1024) + "/" + String(ESP.getFlashChipRealSize() / 1024) + " KB · RAM libre: " + String(ESP.getFreeHeap() / 1024) + " KB · SDK: " + String(ESP.getSdkVersion()) + " · Tiempo activo: " + String(millis() / 60000.0, 1) + " min · RSSI WiFi: " + String(WiFi.RSSI()) + " dBm\n";
}

void obtenerInfoMilitar(String &message) {
  message += "\n🪖 [CONFLICTOS DE ESTADO]:\n";
  message += "\nTrabajando para obtener estos datos...\n";
}

void obtenerInfoMeteorologicos(String &message) {
  message += "\n🌩️ [ESTADÍSTICAS METEORLOGICAS]:\n";
  message += "\nTrabajando para obtener estos datos...\n";
}

void obtenerRiesgoApagon(String &message) {
message += "\n⚡ [ÍNDICE DE RIESGO DE APAGÓN]:\n";
  message += "\nTrabajando para obtener estos datos...\n";
}

void enviarCorreo() {
  SMTPSession smtp;

  String subject = "📬 [ESP8266] Reporte de pING2 en estado de red local";
  String message = "\n☎️ [ESTADO DE LOS HOST LOCAL]:\n\n";

  for (int i = 0; i < numHosts; i++) {
    message += String(hosts[i].name) + " (" + hosts[i].ip + "): " +
               (hosts[i].isUp ? "🟢 ONLINE" : "🔴 OFFLINE") + "\n";
  }

  obtenerInfoMeteorologicos(message);
  obtenerRiesgoApagon(message);
  obtenerInfoMilitar(message);
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

  Serial.println("⏱ Esperando 01:00 horas antes del siguiente reporte...");
  // const unsigned long TIEMPO_ESPERA = 1000 * 60; // 1 minuto
  const unsigned long TIEMPO_ESPERA = 1000UL * 60 * 60; // 1 hora
  delay(TIEMPO_ESPERA);
}
