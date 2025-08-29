#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ===== CONFIGURACIÓN =====
const char* ssid = "Familia G.V";
const char* password = "VerseK3r98";
const char* serverURL = "https://horno-tecnelectro.onrender.com/api/message";

// ===== PINES =====
const int ledPin = 26;
const int fotoPin = 33;

// ===== VARIABLES =====
unsigned long lastSendTime = 0;
unsigned long lastCommandCheck = 0;
int messageCount = 0;
int ledState = 0;

// ===== FUNCIONES =====
void connectToWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi conectado");
    Serial.print("📶 RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println("\n❌ Error: No se pudo conectar a WiFi");
  }
}

void controlLED(int state) {
  ledState = state;
  digitalWrite(ledPin, state);
  Serial.print("💡 LED ");
  Serial.println(state ? "ENCENDIDO" : "APAGADO");
}

void checkForCommands() {
  WiFiClientSecure client;
  HTTPClient http;
  
  String commandsURL = "https://horno-tecnelectro.onrender.com/api/esp32-commands";
  
  Serial.println("🔍 Consultando comandos...");
  
  if (http.begin(client, commandsURL)) {
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    Serial.print("📡 Código HTTP comandos: ");
    Serial.println(httpCode);
    
    if (httpCode == 200) {
      String commands = http.getString();
      Serial.print("📨 Comandos recibidos: ");
      Serial.println(commands);
      
      if (commands != "no_commands") {
        if (commands.indexOf("led_on") != -1) {
          controlLED(1);
          Serial.println("✅ Comando LED ON ejecutado");
        } else if (commands.indexOf("led_off") != -1) {
          controlLED(0);
          Serial.println("✅ Comando LED OFF ejecutado");
        }
      }
    } else {
      Serial.print("❌ Error obteniendo comandos: ");
      Serial.println(http.errorToString(httpCode));
    }
    
    http.end();
  } else {
    Serial.println("❌ No se pudo conectar para obtener comandos");
  }
}

void sendSensorData() {
  int fotoValue = analogRead(fotoPin);
  
  Serial.println("\n📤 Enviando datos...");
  
  WiFiClientSecure client;
  HTTPClient http;
  
  client.setInsecure();
  client.setTimeout(10000);
  
  if (http.begin(client, serverURL)) {
    http.addHeader("Content-Type", "application/json");
    
    messageCount++;
    String jsonPayload = "{\"topic\":\"esp32/sensors\",\"message\":\"";
    jsonPayload += "count=" + String(messageCount);
    jsonPayload += "&led=" + String(ledState);
    jsonPayload += "&foto=" + String(fotoValue);
    jsonPayload += "&rssi=" + String(WiFi.RSSI());
    jsonPayload += "\"}";
    
    Serial.print("📦 JSON: ");
    Serial.println(jsonPayload);
    
    int httpCode = http.POST(jsonPayload);
    Serial.print("📡 Respuesta HTTP: ");
    Serial.println(httpCode);
    
    if (httpCode == 200) {
      String response = http.getString();
      Serial.print("✅ Respuesta: ");
      Serial.println(response);
    } else {
      Serial.print("❌ Error: ");
      Serial.println(http.errorToString(httpCode));
    }
    
    http.end();
  }
  
  Serial.println("--------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  pinMode(ledPin, OUTPUT);
  pinMode(fotoPin, INPUT);
  controlLED(0);
  
  Serial.println("\n🚀 ESP32 - Horno Tecnelectro (HTTP)");
  Serial.print("🔗 Servidor: ");
  Serial.println(serverURL);
  
  connectToWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi desconectado, reconectando...");
    connectToWiFi();
    delay(5000);
    return;
  }
  
  // Enviar datos cada 10 segundos
  if (millis() - lastSendTime > 10000) {
    lastSendTime = millis();
    sendSensorData();
  }
  
  // Consultar comandos cada 3 segundos
  if (millis() - lastCommandCheck > 3000) {
    lastCommandCheck = millis();
    checkForCommands();
  }
  
  // Mostrar estado cada 5 segundos
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 5000) {
    lastStatusTime = millis();
    Serial.print("💡 LED: ");
    Serial.print(ledState);
    Serial.print(" | 📷 Foto: ");
    Serial.print(analogRead(fotoPin));
    Serial.print(" | 📶 RSSI: ");
    Serial.println(WiFi.RSSI());
  }
  
  delay(1000);
}
