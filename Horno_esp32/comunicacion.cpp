#include "comunicacion.h"
#include "config.h"
#include "hmi.h"

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
          if (commands.indexOf("start") != -1 && estadoActual == APAGADO) {
            estadoActual = PROCESANDO;
            Serial.println("✅ Comando START ejecutado");
          } 
          else if (commands.indexOf("stop") != -1 && estadoActual != APAGADO) {
            //detenerSistema();
            estadoActual = DETENER;
            Serial.println("✅ Comando STOP ejecutado");
          }
          /*
          else if (commands.indexOf("system_emergency") != -1) {
            //activarEmergencia("EMERGENCIA: Comando remoto!");
            estadoActual = EMERGENCIA;
            Serial.println("✅ Comando EMERGENCY ejecutado");
          }

          else if (commands.indexOf("system_reset") != -1 && estadoActual == EMERGENCIA) {
            //resetBtnCallback(NULL);
            estadoActual = APAGADO;
            Serial.println("✅ Comando RESET ejecutado");
          }
          */
          else if (commands.indexOf("manual") != -1 && estadoActual == EMERGENCIA) {
            estadoActual = MANUAL
            Serial.println("✅ Comando MANUAL ejecutado");
          }

          else if (commands.indexOf("valv1_on") != -1) {
            if(estadoActual == MANUAL){
              valvula_1_auto = true; // Actualizar estado automático
              Serial.println("✅ Comando VALVE1 ON ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando VALVE1 ON ejecutado pero no esta en estado MANUAL");
            }
          }
          else if (commands.indexOf("valv1_off") != -1) {
            if(estadoActual == MANUAL){
              valvula_1_auto = false; // Actualizar estado automático
              Serial.println("✅ Comando VALVE1 OFF ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando VALV1 OFF ejecutado pero no esta en estado MANUAL");
            }
          }
          else if (commands.indexOf("valv2_on") != -1) {
            if(estadoActual == MANUAL){
              valvula_2_auto = true; // Actualizar estado automático
              Serial.println("✅ Comando VALVE2 ON ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando VALV2 ON ejecutado pero no esta en estado MANUAL");
            }
          }
          else if (commands.indexOf("valv2_off") != -1) {
            if(estadoActual == MANUAL){
              valvula_2_auto = false; // Actualizar estado automático
              Serial.println("✅ Comando VALVE2 OFF ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando VALVE2 OFF ejecutado pero no esta en estado MANUAL");
            }
          }
          else if (commands.indexOf("bomba1_on") != -1) {
            if(estadoActual == MANUAL){
              bomba_1_auto = true; // Actualizar estado automático
              Serial.println("✅ Comando BOMBA1 ON ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando BOMBA1 ON ejecutado pero no esta en estado MANUAL");
            }
          }
          else if (commands.indexOf("bomba1_off") != -1) {
            if(estadoActual == MANUAL){
              bomba_1_auto = false; // Actualizar estado automático
              Serial.println("✅ Comando BOMBA1 OFF ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando BOMBA1 OFF ejecutado pero no esta en estado MANUAL");
            }            
          }
          else if (commands.indexOf("bomba2_on") != -1) {
            if(estadoActual == MANUAL){
              bomba_2_auto = true; // Actualizar estado automático
              Serial.println("✅ Comando BOMBA2 ON ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando BOMBA2 ON ejecutado pero no esta en estado MANUAL");
            }
          }
          else if (commands.indexOf("bomba2_off") != -1) {
            if(estadoActual == MANUAL){
              bomba_2_auto = false; // Actualizar estado automático
              Serial.println("✅ Comando BOMBA2 OFF ejecutado en estado MANUAL");
            }
            else{
              Serial.println("✅❌ Comando BOMBA2 OFF ejecutado pero no esta en estado MANUAL");
            }
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

    // 🔧 Armar JSON con el mismo formato que el primer código
    String jsonPayload = "{\"topic\":\"horno/data\",\"message\":\"";
    jsonPayload += "temperaturas=[" + String(temperaturas[0], 1) + ",";
    jsonPayload += String(temperaturas[1], 1) + ",";
    jsonPayload += String(temperaturas[2], 1) + ",";
    jsonPayload += String(temperaturas[3], 1) + "]";
    jsonPayload += "&nivelTanque=" + String((int)nivelTanque);
    //jsonPayload += "&niveles=[" + String(niveles[0]) + ",";
    //jsonPayload += String(niveles[1]) + ",";
    //jsonPayload += String(niveles[2]) + "]";
    jsonPayload += "&presion=" + String(presionActual, 1);
    jsonPayload += "&valvula1=" + String(digitalRead(VALVULA_1) ? "true" : "false");
    jsonPayload += "&valvula2=" + String(digitalRead(VALVULA_2) ? "true" : "false");
    jsonPayload += "&bomba1=" + String(digitalRead(BOMBA_1) ? "true" : "false");
    jsonPayload += "&bomba2=" + String(digitalRead(BOMBA_2) ? "true" : "false");

    // CAMBIAR LOS ESTADOS EN EL SERVIDOR
    jsonPayload += "&estado=" + String((int)estadoActual);  


    jsonPayload += "&emergencia=" + String(emergencia ? "true" : "false");
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
  // Enviar datos cada 10 segundos
  if (millis() - lastSendTime > 10000) {
    lastSendTime = millis();
    sendSystemData();
  }
  
  // Consultar comandos cada 2 segundos
  if (millis() - lastCommandCheck > 6000) {
    lastCommandCheck = millis();
    checkForCommands();
  }
  
  // Reconexión WiFi si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi desconectado, reconectando...");
    connectToWiFi();
  }
}
