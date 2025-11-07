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
  {"192.168.1.129", "TV-SALÓN", false},
  {"192.168.1.129", "PC-MILITAR-WIFI", false},
  {"192.168.1.130", "PORTATIL-AIR", false},
  {"192.168.1.134", "ANDROID", false},
  {"192.168.1.139", "TV-HABITACIÓN", false},
  {"192.168.1.131", "IPHONE", false}
};
const int numHosts = sizeof(hosts) / sizeof(hosts[0]);

// ===== FUNCIONES =====
void conectarWiFi() {
  Serial.println("Conectando al WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Intentamos conectar por un máximo de 10 segundos
  long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado al WiFi");
    Serial.print("IP local: ");
    Serial.println(WiFi.localIP());
  } else {
    // Si falla la conexión, esperamos 30 segundos y reiniciamos
    Serial.println("\n❌ ¡ERROR! Fallo al conectar al WiFi. Reiniciando en 30 segundos.");
    delay(30000);
    ESP.restart();
  }
}

void verificarHosts() {
  Serial.println("Verificando estado de los hosts...");
  for (int i = 0; i < numHosts; i++) {
    // Usamos un timeout más corto para acelerar el proceso de ping
    bool online = Ping.ping(hosts[i].ip, 1); 
    hosts[i].isUp = online;
    Serial.printf("%s (%s): %s\n",
      hosts[i].name,
      hosts[i].ip,
      online ? "ONLINE" : "OFFLINE");
  }
}

void obtenerInfoESP(String &message) {
  message += "\n⚠️ [INFORME DE SISTEMA ESP8266]\n\n";
  message += "Chip ID: " + String(ESP.getChipId()) + "\n";
  message += "Flash total: " + String(ESP.getFlashChipRealSize() / 1024) + " KB\n";
  message += "Flash usado: " + String(ESP.getSketchSize() / 1024) + " KB\n";
  message += "Flash libre: " + String((ESP.getFlashChipRealSize() - ESP.getSketchSize()) / 1024) + " KB\n";
  message += "RAM libre: " + String(ESP.getFreeHeap() / 1024) + " KB\n";
  message += "SDK: " + String(ESP.getSdkVersion()) + "\n";
  message += "Tiempo activo: " + String(millis() / 60000.0, 1) + " min\n";
  message += "RSSI WiFi: " + String(WiFi.RSSI()) + " dBm\n";
}

void enviarCorreo() {
  SMTPSession smtp;

  String subject = "📬 [ESP8266] Reporte de pING2 en estado de red local";
  String message = "\n🧐 [ESTADO DE LOS HOST LOCAL]:\n\n";

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
  session.login.password = AUTHOR_PASSWORD;
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

  // La sesión se cerrará automáticamente, pero la cerramos explícitamente para limpiar.
  smtp.closeSession(); 
}

void esperarYReiniciar() {
  // Configuración de la espera: 2 minutos (120 segundos)
 const unsigned long TIEMPO_ESPERA_MS = 60 * 60 * 1000UL; // 01:00 hora en milisegundos

 Serial.println("📴 APAGANDO WIFI: Desactivando radio para 0% de emisiones...");
 
  // 1. APAGAR WIFI (0% EMISIONES)
 WiFi.disconnect(true); // Desconecta y olvida la red
 WiFi.mode(WIFI_OFF);   // Desactiva completamente la radio WiFi (el paso CRÍTICO)
 
 Serial.printf("🔄 MODO SOFTWARE: Empezando conteo de %lu segundos (Chip encendido, WiFi apagado).\n", 
  TIEMPO_ESPERA_MS / 1000);

  // 2. CONTADOR / ESPERA ACTIVA
 delay(TIEMPO_ESPERA_MS);
 
 Serial.println("🚀 Fin del conteo. Forzando reinicio para un nuevo ciclo...");
 
  // 3. REINICIO
 ESP.restart(); 
 
  // Después de ESP.restart(), el chip arranca desde setup(),
  // donde se llamará a conectarWiFi() y se encenderá el WiFi de nuevo.
}

void setup() {
  // Se ha ajustado el baud rate a 115200 (estándar para depuración)
 Serial.begin(11520); 
 delay(500);
 Serial.println("\n==============================================");
 Serial.println("🚀 Iniciando ESP8266 pING2 Watcher...");

  // Identificamos el motivo del reinicio para depuración
 if (ESP.getResetReason().startsWith("Software/Power")) {
  Serial.println("⚡ Reinicio detectado: Primer arranque o Reinicio por Software.");
 } else {
  Serial.println("❓ Reinicio detectado: Otro tipo de reinicio.");
 }
 Serial.println("==============================================");


 conectarWiFi(); // Enciende WiFi y conecta
 verificarHosts();
 enviarCorreo();
 
  // Llama a la función que apaga WiFi, espera 2 minutos y reinicia
  esperarYReiniciar();
}

void loop() {
  // No se usa. Todo ocurre en setup() tras cada reinicio por software.
  }
