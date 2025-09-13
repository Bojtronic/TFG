#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ===== CONFIGURACIÓN =====
const char* ssid = "DIGICONTROL";
const char* password = "7012digi19";
const char* serverURL = "https://horno-tecnelectro.onrender.com/api/message";
const char* commandsURL = "https://horno-tecnelectro.onrender.com/api/esp32-commands";

// ===== VARIABLES =====
unsigned long lastSendTime = 0;
unsigned long lastCommandCheck = 0;
int messageCount = 0;

// Variables simuladas para el horno de biomasa
float temperaturaTanque = 65.0;
float temperaturaHorno = 180.0;
float temperaturaCamara = 120.0;
float temperaturaSalida = 62.0;
int nivelVacio = 15;
int nivelMitad = 45;
int nivelLleno = 80;
float presion = 2.8;
bool valvula1 = false;
bool valvula2 = false;
bool bomba1 = false;
bool bomba2 = false;
String estadoSistema = "SISTEMA_APAGADO";
bool emergencia = false;
String bombaActiva = "PRINCIPAL";

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

void simularDatosSensores() {
  // Simular variaciones en los datos
  temperaturaTanque = 60.0 + random(0, 100) / 10.0;
  temperaturaHorno = 175.0 + random(0, 150) / 10.0;
  temperaturaCamara = 115.0 + random(0, 100) / 10.0;
  temperaturaSalida = 60.0 + random(0, 50) / 10.0;
  
  nivelVacio = random(5, 20);
  nivelMitad = random(30, 60);
  nivelLleno = random(70, 95);
  
  presion = 2.5 + random(0, 30) / 10.0;
}

void procesarComando(String comando) {
  Serial.print("🎯 Procesando comando: ");
  Serial.println(comando);
  
  if (comando == "start") {
    estadoSistema = "LLENADO_TANQUE";
    Serial.println("✅ Sistema iniciado - Modo LLENADO");
  } 
  else if (comando == "stop") {
    estadoSistema = "SISTEMA_APAGADO";
    valvula1 = false;
    valvula2 = false;
    bomba1 = false;
    bomba2 = false;
    Serial.println("✅ Sistema detenido");
  }
  else if (comando == "reset") {
    estadoSistema = "SISTEMA_APAGADO";
    emergencia = false;
    Serial.println("✅ Sistema reseteado");
  }
  else if (comando == "emergency") {
    estadoSistema = "EMERGENCIA";
    emergencia = true;
    valvula1 = false;
    valvula2 = false;
    bomba1 = false;
    bomba2 = false;
    Serial.println("🚨 Modo emergencia activado");
  }
  else if (comando == "valv1_on") {
    valvula1 = true;
    Serial.println("✅ Válvula 1 abierta");
  }
  else if (comando == "valv1_off") {
    valvula1 = false;
    Serial.println("✅ Válvula 1 cerrada");
  }
  else if (comando == "valv2_on") {
    valvula2 = true;
    Serial.println("✅ Válvula 2 abierta");
  }
  else if (comando == "valv2_off") {
    valvula2 = false;
    Serial.println("✅ Válvula 2 cerrada");
  }
  else if (comando == "bomba1_on") {
    bomba1 = true;
    bombaActiva = "PRINCIPAL";
    Serial.println("✅ Bomba 1 activada");
  }
  else if (comando == "bomba1_off") {
    bomba1 = false;
    Serial.println("✅ Bomba 1 desactivada");
  }
  else if (comando == "bomba2_on") {
    bomba2 = true;
    bombaActiva = "REDUNDANTE";
    Serial.println("✅ Bomba 2 activada");
  }
  else if (comando == "bomba2_off") {
    bomba2 = false;
    Serial.println("✅ Bomba 2 desactivada");
  }
  else {
    Serial.print("❌ Comando no reconocido: ");
    Serial.println(comando);
  }
}

void checkForCommands() {
  WiFiClientSecure client;
  HTTPClient http;
  
  Serial.println("🔍 Consultando comandos...");
  
  // CONFIGURACIÓN CRÍTICA PARA HTTPS
  client.setInsecure(); // Permite conexiones HTTPS sin verificar certificado
  client.setTimeout(15000);
  
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print("🔄 Intento ");
    Serial.print(attempt);
    Serial.println("/3");
    
    if (http.begin(client, commandsURL)) {
      http.setTimeout(15000);
      
      int httpCode = http.GET();
      Serial.print("📡 Código HTTP: ");
      Serial.println(httpCode);
      
      if (httpCode == 200) {
        String response = http.getString();
        Serial.print("📨 Respuesta: ");
        Serial.println(response);
        
        // Parsear respuesta JSON
        if (response.indexOf('{') != -1) {
          // Es una respuesta JSON
          if (response.indexOf("\"commands\":") != -1) {
            int start = response.indexOf("\"commands\":\"") + 11;
            int end = response.indexOf("\"", start);
            String commands = response.substring(start, end);
            
            if (commands != "no_commands") {
              // Procesar cada comando separado por comas
              int commaPos = 0;
              while (commaPos != -1) {
                int nextComma = commands.indexOf(',', commaPos);
                String command;
                if (nextComma == -1) {
                  command = commands.substring(commaPos);
                  commaPos = -1;
                } else {
                  command = commands.substring(commaPos, nextComma);
                  commaPos = nextComma + 1;
                }
                command.trim();
                if (command.length() > 0) {
                  procesarComando(command);
                }
              }
            } else {
              Serial.println("📭 No hay comandos pendientes");
            }
          }
        } else {
          // Formato antiguo (solo texto)
          if (response != "no_commands") {
            procesarComando(response);
          } else {
            Serial.println("📭 No hay comandos pendientes");
          }
        }
        
        http.end();
        return;
      } else {
        Serial.print("❌ Error obteniendo comandos: ");
        Serial.println(http.errorToString(httpCode));
      }
      
      http.end();
    } else {
      Serial.println("❌ No se pudo conectar al servidor");
    }
    
    if (attempt < 3) {
      Serial.println("⏳ Esperando 2 segundos antes de reintentar...");
      delay(2000);
    }
  }
  
  Serial.println("❌ Todos los intentos fallaron");
}

void sendSensorData() {
  // Simular lectura de sensores
  simularDatosSensores();
  
  Serial.println("\n📤 Enviando datos...");
  
  WiFiClientSecure client;
  HTTPClient http;
  
  client.setInsecure();
  client.setTimeout(15000);
  
  if (http.begin(client, serverURL)) {
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(15000);
    
    messageCount++;
    
    // Crear JSON con datos del sistema de horno
    String jsonPayload = "{\"topic\":\"horno/data\",\"message\":\"";
    jsonPayload += "temperaturas=[" + String(temperaturaTanque, 1) + ",";
    jsonPayload += String(temperaturaHorno, 1) + ",";
    jsonPayload += String(temperaturaCamara, 1) + ",";
    jsonPayload += String(temperaturaSalida, 1) + "]";
    jsonPayload += "&niveles=[" + String(nivelVacio) + ",";
    jsonPayload += String(nivelMitad) + ",";
    jsonPayload += String(nivelLleno) + "]";
    jsonPayload += "&presion=" + String(presion, 1);
    jsonPayload += "&valvula1=" + String(valvula1 ? "true" : "false");
    jsonPayload += "&valvula2=" + String(valvula2 ? "true" : "false");
    jsonPayload += "&bomba1=" + String(bomba1 ? "true" : "false");
    jsonPayload += "&bomba2=" + String(bomba2 ? "true" : "false");
    jsonPayload += "&estado=" + estadoSistema;
    jsonPayload += "&emergencia=" + String(emergencia ? "true" : "false");
    jsonPayload += "&bombaActiva=" + bombaActiva;
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
  } else {
    Serial.println("❌ No se pudo conectar para enviar datos");
  }
  
  Serial.println("--------------------------------");
}

void testServerConnection() {
  WiFiClientSecure client;
  HTTPClient http;
  
  client.setInsecure();
  client.setTimeout(15000);
  
  Serial.println("🧪 Probando conexión con el servidor...");
  
  if (http.begin(client, commandsURL)) {
    http.setTimeout(15000);
    
    int httpCode = http.GET();
    Serial.print("📡 Código HTTP de prueba: ");
    Serial.println(httpCode);
    
    if (httpCode == 200) {
      String response = http.getString();
      Serial.print("✅ Conexión exitosa. Respuesta: ");
      Serial.println(response);
    } else {
      Serial.print("❌ Error en la prueba: ");
      Serial.println(http.errorToString(httpCode));
    }
    
    http.end();
  } else {
    Serial.println("❌ No se pudo conectar para prueba");
  }
}

void mostrarEstadoSistema() {
  Serial.println("\n📊 ESTADO DEL SISTEMA:");
  Serial.println("====================");
  Serial.print("🌡️  Temperaturas: Tanque=");
  Serial.print(temperaturaTanque);
  Serial.print("°C, Horno=");
  Serial.print(temperaturaHorno);
  Serial.print("°C, Cámara=");
  Serial.print(temperaturaCamara);
  Serial.print("°C, Salida=");
  Serial.print(temperaturaSalida);
  Serial.println("°C");
  
  Serial.print("💧 Niveles: Vacío=");
  Serial.print(nivelVacio);
  Serial.print("%, Mitad=");
  Serial.print(nivelMitad);
  Serial.print("%, Lleno=");
  Serial.print(nivelLleno);
  Serial.println("%");
  
  Serial.print("📊 Presión: ");
  Serial.print(presion);
  Serial.println(" bar");
  
  Serial.print("🔧 Válvulas: 1=");
  Serial.print(valvula1 ? "Abierta" : "Cerrada");
  Serial.print(", 2=");
  Serial.println(valvula2 ? "Abierta" : "Cerrada");
  
  Serial.print("⚡ Bombas: 1=");
  Serial.print(bomba1 ? "Encendida" : "Apagada");
  Serial.print(", 2=");
  Serial.print(bomba2 ? "Encendida" : "Apagada");
  Serial.print(", Activa=");
  Serial.println(bombaActiva);
  
  Serial.print("📋 Estado: ");
  Serial.print(estadoSistema);
  Serial.print(", Emergencia=");
  Serial.println(emergencia ? "ACTIVA" : "Inactiva");
  
  Serial.print("📶 RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.print(" | 📱 Mensajes: ");
  Serial.println(messageCount);
  Serial.println("====================");
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n🚀 ESP32 - Horno de biomasa ECOVIEW (HTTP)");
  Serial.print("🔗 Servidor: ");
  Serial.println(serverURL);
  Serial.print("🔗 Comandos: ");
  Serial.println(commandsURL);
  
  connectToWiFi();
  
  // Test inicial de conexión
  if (WiFi.status() == WL_CONNECTED) {
    testServerConnection();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi desconectado, reconectando...");
    connectToWiFi();
    delay(5000);
    return;
  }
  
  // Enviar datos cada 15 segundos
  if (millis() - lastSendTime > 15000) {
    lastSendTime = millis();
    sendSensorData();
  }
  
  // Consultar comandos cada 5 segundos
  if (millis() - lastCommandCheck > 5000) {
    lastCommandCheck = millis();
    checkForCommands();
  }
  
  // Mostrar estado completo cada 20 segundos
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 20000) {
    lastStatusTime = millis();
    mostrarEstadoSistema();
  }
  
  delay(1000);
}