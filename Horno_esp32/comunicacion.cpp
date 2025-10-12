#include "config.h"
#include "comunicacion.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// #include "hmi.h"

void connectToWiFi()
{
  static unsigned long lastAttemptTime = 0;
  const unsigned long ATTEMPT_INTERVAL = 2000; // ms entre intentos
  const int MAX_ATTEMPTS = 20;
  static int attempts = 0;

  // Iniciar conexión solo si no está conectado
  if (WiFi.status() != WL_CONNECTED) {
    if (attempts == 0) {
      Serial.print("Conectando a WiFi");
      WiFi.begin(ssid, password);
    }

    unsigned long now = millis();
    if (now - lastAttemptTime >= ATTEMPT_INTERVAL) {
      lastAttemptTime = now;
      Serial.print(".");
      attempts++;

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi conectado");
        Serial.print("📶 RSSI: ");
        Serial.println(WiFi.RSSI());
        attempts = 0; // resetear para la próxima desconexión
      }
      else if (attempts >= MAX_ATTEMPTS) {
        Serial.println("\n❌ No se pudo conectar a WiFi");
        attempts = 0; // resetear para intentar después
      }
    }
  }
}


void checkForCommands()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  static WiFiClientSecure client;
  static HTTPClient http;

  Serial.println("🔍 Consultando comandos...");

  client.setInsecure();
  client.setTimeout(7000); // B. Timeout reducido

  if (http.begin(client, commandsURL)) // A. Reutilización
  {
    http.setTimeout(7000); // B. Timeout reducido

    int httpCode = http.GET(); // C. Un solo intento
    Serial.print("📡 Código HTTP: ");
    Serial.println(httpCode);

    if (httpCode == 200)
    {
      String commands;
      commands.reserve(256); // D. Evitar múltiples reallocs
      commands = http.getString();

      Serial.print("📨 Comandos recibidos: ");
      Serial.println(commands);

      if (commands != "no_commands")
      {
        if (commands.indexOf("start") != -1 && (estadoActual == APAGADO || estadoActual == MANUAL || estadoActual == DETENER))
        {
          estadoActual = PROCESANDO;
          Serial.println("✅ Comando START ejecutado");
        }
        else if (commands.indexOf("stop") != -1 && (estadoActual == PROCESANDO || estadoActual == MANUAL))
        {
          estadoActual = DETENER;
          Serial.println("✅ Comando STOP ejecutado");
        }
        else if (commands.indexOf("manual") != -1 && (estadoActual == PROCESANDO || estadoActual == APAGADO || estadoActual == DETENER))
        {
          estadoActual = MANUAL;
          Serial.println("✅ Comando MANUAL ejecutado");
        }
        else if (commands.indexOf("valv1_on") != -1)
        {
          if (estadoActual == MANUAL)
          {
            valvula_1_auto = true;
            Serial.println("✅ Comando VALVE1 ON ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando VALV1 ON ejecutado pero no esta en estado MANUAL");
          }
        }
        else if (commands.indexOf("valv1_off") != -1)
        {
          if (estadoActual == MANUAL)
          {
            valvula_1_auto = false;
            Serial.println("✅ Comando VALVE1 OFF ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando VALV1 OFF ejecutado pero no esta en estado MANUAL");
          }
        }
        else if (commands.indexOf("valv2_on") != -1)
        {
          if (estadoActual == MANUAL)
          {
            valvula_2_auto = true;
            Serial.println("✅ Comando VALV2 ON ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando VALV2 ON ejecutado pero no esta en estado MANUAL");
          }
        }
        else if (commands.indexOf("valv2_off") != -1)
        {
          if (estadoActual == MANUAL)
          {
            valvula_2_auto = false;
            Serial.println("✅ Comando VALV2 OFF ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando VALV2 OFF ejecutado pero no esta en estado MANUAL");
          }
        }
        else if (commands.indexOf("bomba1_on") != -1)
        {
          if (estadoActual == MANUAL)
          {
            bomba_1_auto = true;
            Serial.println("✅ Comando BOMBA1 ON ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando BOMBA1 ON ejecutado pero no esta en estado MANUAL");
          }
        }
        else if (commands.indexOf("bomba1_off") != -1)
        {
          if (estadoActual == MANUAL)
          {
            bomba_1_auto = false;
            Serial.println("✅ Comando BOMBA1 OFF ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando BOMBA1 OFF ejecutado pero no esta en estado MANUAL");
          }
        }
        else if (commands.indexOf("bomba2_on") != -1)
        {
          if (estadoActual == MANUAL)
          {
            bomba_2_auto = true;
            Serial.println("✅ Comando BOMBA2 ON ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando BOMBA2 ON ejecutado pero no esta en estado MANUAL");
          }
        }
        else if (commands.indexOf("bomba2_off") != -1)
        {
          if (estadoActual == MANUAL)
          {
            bomba_2_auto = false;
            Serial.println("✅ Comando BOMBA2 OFF ejecutado en estado MANUAL");
          }
          else
          {
            Serial.println("✅❌ Comando BOMBA2 OFF ejecutado pero no esta en estado MANUAL");
          }
        }

        http.end();
        return;
      }
      else
      {
        Serial.println("📭 No hay comandos pendientes");
      }
    }
    else
    {
      Serial.print("❌ Error obteniendo comandos: ");
      Serial.println(http.errorToString(httpCode));
    }

    http.end();
  }
  else
  {
    Serial.println("❌ No se pudo conectar al servidor");
  }
}


void sendSystemData()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  Serial.println("\n📤 Enviando datos del sistema...");

  WiFiClientSecure client;
  HTTPClient http;

  client.setInsecure();
  client.setTimeout(7000); // si la conectividad es buena (internet rapido y servidor estable) es mejor disminuir este tiempo, sino se debe aumentar pero realentiza el sistema

  if (http.begin(client, serverURL))
  {
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(7000); // si la conectividad es buena (internet rapido y servidor estable) es mejor disminuir este tiempo, sino se debe aumentar pero realentiza el sistema

    messageCount++;

    // 🔧 Armar JSON con el mismo formato que el primer código
    String jsonPayload = "{\"topic\":\"horno/data\",\"message\":\"";
    jsonPayload += "temperaturas=[" + String(temperaturas[0], 1) + ",";
    jsonPayload += String(temperaturas[1], 1) + ",";
    jsonPayload += String(temperaturas[2], 1) + ",";
    jsonPayload += String(temperaturas[3], 1) + "]";
    jsonPayload += "&nivelTanque=" + String((int)nivelTanque);
    jsonPayload += "&presion=" + String(presionActual, 1);
    jsonPayload += "&valvula1=" + String(digitalRead(VALVULA_1) ? "true" : "false");
    jsonPayload += "&valvula2=" + String(digitalRead(VALVULA_2) ? "true" : "false");
    jsonPayload += "&bomba1=" + String(digitalRead(BOMBA_1) ? "true" : "false");
    jsonPayload += "&bomba2=" + String(digitalRead(BOMBA_2) ? "true" : "false");
    jsonPayload += "&estado=" + String(estadoActual);
    jsonPayload += "&mensaje=" + String(mensajeActual);
    jsonPayload += "\"}";

    Serial.println("\n \n 📦 Datos enviados: ");
    Serial.println(jsonPayload);
    Serial.println("\n \n");

    int httpCode = http.POST(jsonPayload);
    Serial.print("📡 Respuesta HTTP: ");
    Serial.println(httpCode);

    if (httpCode == 200)
    {
      String response = http.getString();
      Serial.println("✅ El servidor recibió los datos \n");
      Serial.print("✅ Respuesta: ");
      Serial.println(response);
    }
    else
    {
      Serial.print("❌ Error: ");
      Serial.println(http.errorToString(httpCode));
    }

    http.end();
  }
  else
  {
    Serial.println("❌ No se pudo conectar para enviar datos");
  }

  Serial.println("--------------------------------");
}

void handleServerCommunication()
{
  static unsigned long lastCommTime = 0;
  static bool sendDataNext = true;          // Alternar entre envío y recepción
  const unsigned long COMM_INTERVAL = 3000; 

  // Reconexión WiFi si es necesario (se mantiene igual)
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("⚠️  WiFi desconectado, reconectando...");
    connectToWiFi();
  }

  // Verificar si es tiempo de comunicación
  if (millis() - lastCommTime >= COMM_INTERVAL)
  {
    lastCommTime = millis();

    if (sendDataNext)
    {
      // Ciclo 1: Enviar datos al servidor
      sendSystemData();
      Serial.println("📤 Datos enviados al servidor");
    }
    else
    {

      if ((estadoActual != EMERGENCIA) || (estadoActual != APAGADO))
      {
        // Ciclo 2: Recibir comandos del servidor
        checkForCommands();
        Serial.println("📥 Comandos consultados del servidor");
      }
    }

    // Alternar para el próximo ciclo
    sendDataNext = !sendDataNext;
  }

}
