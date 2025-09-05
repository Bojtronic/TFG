#include "comunicacion.h"
#include "config.h"

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
    mensajesHMI("WiFi conectado");
  } else {
    Serial.println("\n❌ Error: No se pudo conectar a WiFi");
    mensajesHMI("Error WiFi");
  }
}

void checkForCommands() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClientSecure client;
  HTTPClient http;
  
  Serial.println("🔍 Consultando comandos...");
  
  client.setInsecure();
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
        String commands = http.getString();
        Serial.print("📨 Comandos recibidos: ");
        Serial.println(commands);
        
        if (commands != "no_commands") {
          // Comandos para el sistema de agua caliente
          if (commands.indexOf("system_start") != -1 && estadoActual == SISTEMA_APAGADO) {
            iniciarSistema();
            Serial.println("✅ Comando SYSTEM START ejecutado");
          } 
          else if (commands.indexOf("system_stop") != -1 && estadoActual != SISTEMA_APAGADO) {
            detenerSistema();
            Serial.println("✅ Comando SYSTEM STOP ejecutado");
          }
          else if (commands.indexOf("system_emergency") != -1) {
            activarEmergencia("EMERGENCIA: Comando remoto!");
            Serial.println("✅ Comando EMERGENCY ejecutado");
          }
          else if (commands.indexOf("system_reset") != -1 && estadoActual == EMERGENCIA) {
            resetBtnCallback(NULL);
            Serial.println("✅ Comando RESET ejecutado");
          }
          else if (commands.indexOf("valve1_on") != -1) {
            digitalWrite(VALVULA_1, HIGH);
            Serial.println("✅ Comando VALVE1 ON ejecutado");
          }
          else if (commands.indexOf("valve1_off") != -1) {
            digitalWrite(VALVULA_1, LOW);
            Serial.println("✅ Comando VALVE1 OFF ejecutado");
          }
          else if (commands.indexOf("valve2_on") != -1) {
            digitalWrite(VALVULA_2, HIGH);
            Serial.println("✅ Comando VALVE2 ON ejecutado");
          }
          else if (commands.indexOf("valve2_off") != -1) {
            digitalWrite(VALVULA_2, LOW);
            Serial.println("✅ Comando VALVE2 OFF ejecutado");
          }
          
          http.end();
          return;
        } else {
          Serial.println("📭 No hay comandos pendientes");
        }
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

void sendSystemData() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.println("\n📤 Enviando datos del sistema...");
  
  WiFiClientSecure client;
  HTTPClient http;
  
  client.setInsecure();
  client.setTimeout(15000);
  
  if (http.begin(client, serverURL)) {
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(15000);
    
    messageCount++;
    
    // Crear JSON con todos los datos del sistema
    String jsonPayload = "{\"topic\":\"agua_caliente/sistema\",\"message\":\"";
    jsonPayload += "count=" + String(messageCount);
    jsonPayload += "&estado=" + String((int)estadoActual);
    jsonPayload += "&temp_tanque=" + String(temperaturas[0], 1);
    jsonPayload += "&temp_horno=" + String(temperaturas[1], 1);
    jsonPayload += "&temp_camara=" + String(temperaturas[2], 1);
    jsonPayload += "&temp_salida=" + String(temperaturas[3], 1);
    jsonPayload += "&nivel_vacio=" + String(niveles[0]);
    jsonPayload += "&nivel_mitad=" + String(niveles[1]);
    jsonPayload += "&nivel_lleno=" + String(niveles[2]);
    jsonPayload += "&presion=" + String(presionActual, 1);
    jsonPayload += "&valvula1=" + String(digitalRead(VALVULA_1));
    jsonPayload += "&valvula2=" + String(digitalRead(VALVULA_2));
    jsonPayload += "&bomba1=" + String(digitalRead(BOMBA_1));
    jsonPayload += "&bomba2=" + String(digitalRead(BOMBA_2));
    jsonPayload += "&emergencia=" + String(emergencia);
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
  if (WiFi.status() != WL_CONNECTED) return;
  
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
      mensajesHMI("Servidor conectado");
    } else {
      Serial.print("❌ Error en la prueba: ");
      Serial.println(http.errorToString(httpCode));
      mensajesHMI("Error servidor");
    }
    
    http.end();
  } else {
    Serial.println("❌ No se pudo conectar para prueba");
  }
}

void handleServerCommunication() {
  // Enviar datos cada 30 segundos
  if (millis() - lastSendTime > 30000) {
    lastSendTime = millis();
    sendSystemData();
  }
  
  // Consultar comandos cada 10 segundos
  if (millis() - lastCommandCheck > 10000) {
    lastCommandCheck = millis();
    checkForCommands();
  }
  
  // Reconexión WiFi si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi desconectado, reconectando...");
    connectToWiFi();
  }
}
