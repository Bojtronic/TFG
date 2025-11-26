#include "config.h"
#include "test.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ================= PRUEBA DE CONEXION =================

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
    } else {
      Serial.print("❌ Error en la prueba: ");
      Serial.println(http.errorToString(httpCode));
    }
    
    http.end();
  } else {
    Serial.println("❌ No se pudo conectar para prueba");
  }
}

// ================= PRUEBAS POR ESTADO =================

// Estado: APAGADO → todo frío y sin actividad
void testApagado() {
  Serial.println("🔹 TEST: Estado APAGADO");
  temperaturas[0] = 25;   // Tanque frio (MAX 70°C)
  temperaturas[1] = 25;   // Horno frio (MIN 35°C, MAX 200°C)
  temperaturas[2] = 25;   // Camara fria (MAX 150°C)
  temperaturas[3] = 25;   // Salida de agua caliente para el usuario
  nivelTanque     = 0;    // Vacío (MIN 10%)
  presionActual   = 2;    // (MIN 1 bar)
}

// Estado: PROCESANDO → múltiples escenarios
void testProcesando(int caso) {
  switch (caso) {
    case 0:
      Serial.println("🔹 TEST: PROCESANDO - Nivel medio, horno caliente");
      temperaturas[0] = 50;   // Tanque (MAX 70°C)
      temperaturas[1] = 100;  // Horno caliente (MIN 35°C, MAX 200°C)
      temperaturas[2] = 90;   // Camara (MAX 150°C)
      temperaturas[3] = 50;   // Salida de agua caliente para el usuario
      nivelTanque     = 60;   // Medio (40–80%)
      //presionActual   = 5;    // Correcta (MIN 1 bar)
      break;
      
    case 1:
      Serial.println("🔹 TEST: PROCESANDO - Tanque lleno, máxima eficiencia");
      temperaturas[0] = 65;   // Tanque cerca del máximo
      temperaturas[1] = 150;  // Horno muy caliente
      temperaturas[2] = 120;  // Camara caliente
      temperaturas[3] = 62;   // Agua casi en temperatura objetivo
      nivelTanque     = 95;   // Casi lleno (>80%)
      //presionActual   = 6;    // Buena presión
      break;
      
    case 2:
      Serial.println("🔹 TEST: PROCESANDO - Nivel bajo, necesita llenado");
      temperaturas[0] = 45;   // Tanque 
      temperaturas[1] = 80;   // Horno caliente
      temperaturas[2] = 70;   // Camara caliente
      temperaturas[3] = 40;   // Agua fría
      nivelTanque     = 20;   // Bajo (<40%)
      //presionActual   = 4;    // Presión adecuada
      break;
      
    case 3:
      Serial.println("🔹 TEST: PROCESANDO - Temperatura límite tanque");
      temperaturas[0] = 68;   // Tanque cerca del máximo (70°C)
      temperaturas[1] = 120;  // Horno caliente
      temperaturas[2] = 100;  // Camara caliente
      temperaturas[3] = 65;   // Agua caliente
      nivelTanque     = 70;   // Medio-alto
      //presionActual   = 5;    // Presión normal
      break;
  }
}

// Estado: DETENER → varias razones
void testDetener(int caso) {
  switch (caso) {
    case 0:
      Serial.println("🔹 TEST: DETENER por nivel bajo");
      temperaturas[0] = 65;   // Tanque caliente
      temperaturas[1] = 180;  // Horno caliente
      temperaturas[2] = 160;  // Camara caliente
      temperaturas[3] = 60;   // Agua caliente
      nivelTanque     = 30;   // Bajo (<40%)
      presionActual   = 4;    // Correcta
      break;

    case 1:
      Serial.println("🔹 TEST: DETENER por presión baja");
      temperaturas[0] = 60;   // Tanque caliente
      temperaturas[1] = 150;  // Horno caliente
      temperaturas[2] = 130;  // Camara caliente
      temperaturas[3] = 55;   // Agua caliente
      nivelTanque     = 60;   // Medio
      presionActual   = 0.5;  // Baja (<1 bar)
      break;
      
    case 2:
      Serial.println("🔹 TEST: DETENER - Enfriamiento progresivo");
      temperaturas[0] = 40;   // Tanque enfriándose
      temperaturas[1] = 45;   // Horno casi frío
      temperaturas[2] = 42;   // Camara casi fría
      temperaturas[3] = 38;   // Agua enfriándose
      nivelTanque     = 50;   // Medio
      presionActual   = 3;    // Normal
      break;
  }
}

// Estado: MANUAL → forzado
void testManual() {
  Serial.println("🔹 TEST: Estado MANUAL");
  temperaturas[0] = 50;   // Tanque (MAX 70°C)
  temperaturas[1] = 60;   // Horno (MIN 35°C, MAX 200°C)
  temperaturas[2] = 55;   // Camara (MAX 150°C)
  temperaturas[3] = 50;   // Salida de agua caliente para el usuario
  nivelTanque     = 100;  // Lleno (>80%)
  presionActual   = 5;    // Correcta (MIN 1 bar)
}

// Estado: EMERGENCIA → múltiples tipos
void testEmergencia(int caso) {
  switch (caso) {
    case 0:
      Serial.println("🔹 EMERGENCIA: Tanque vacío + horno caliente");
      temperaturas[0] = 75;   // Tanque sobrecalentado
      temperaturas[1] = 400;  // Horno crítico
      temperaturas[2] = 410;  // Camara crítica
      temperaturas[3] = 90;   // Agua sobrecalentada
      nivelTanque     = 5;    // Vacío crítico (<10%)
      presionActual   = 0.5;  // Baja
      break;

    case 1:
      Serial.println("🔹 EMERGENCIA: Sobretemperatura en tanque");
      temperaturas[0] = 80;   // Tanque (MAX 70°C → sobrepasado)
      temperaturas[1] = 100;  // Horno caliente
      temperaturas[2] = 90;   // Camara caliente
      temperaturas[3] = 75;   // Agua sobrecalentada
      nivelTanque     = 70;   // Medio
      presionActual   = 5;    // Correcta
      break;

    case 2:
      Serial.println("🔹 EMERGENCIA: Sobretemperatura en horno");
      temperaturas[0] = 60;   // Tanque normal
      temperaturas[1] = 250;  // Horno (MAX 200°C → sobrepasado)
      temperaturas[2] = 200;  // Camara (MAX 150°C → sobrepasado)
      temperaturas[3] = 90;   // Agua caliente
      nivelTanque     = 50;   // Medio
      presionActual   = 5;    // Correcta
      break;

    case 3:
      Serial.println("🔹 EMERGENCIA: Sobretemperatura en cámara");
      temperaturas[0] = 50;   // Tanque normal
      temperaturas[1] = 100;  // Horno caliente
      temperaturas[2] = 180;  // Camara (MAX 150°C → sobrepasado)
      temperaturas[3] = 60;   // Agua caliente
      nivelTanque     = 40;   // Mitad
      presionActual   = 5;    // Correcta
      break;
      
    case 4:
      Serial.println("🔹 EMERGENCIA: Múltiples fallos simultáneos");
      temperaturas[0] = 85;   // Tanque crítico
      temperaturas[1] = 300;  // Horno crítico
      temperaturas[2] = 250;  // Camara crítica
      temperaturas[3] = 95;   // Agua crítica
      nivelTanque     = 8;    // Casi vacío
      presionActual   = 0.2;  // Sin presión
      break;
  }
}

// ================= CONTROLADOR DE PRUEBAS MEJORADO =================
void ejecutarPruebas() {
  static int estadoPrincipal = 0;  // 0:APAGADO, 1:PROCESANDO, 2:DETENER, 3:EMERGENCIA, 4:MANUAL
  static int subprueba = 0;        // Subprueba dentro de cada estado
  static unsigned long inicioEstado = millis();
  static unsigned long inicioSubprueba = millis();

  // Tiempo por subprueba individual
  if (millis() - inicioSubprueba > 20000) {  // 15 segundos por subprueba
    subprueba++;
    inicioSubprueba = millis();
    
    // Verificar si hemos completado todas las subpruebas del estado actual
    bool cambiarEstado = false;
    switch (estadoPrincipal) {
      case 0: // APAGADO - 1 subprueba
        if (subprueba >= 1) cambiarEstado = true;
        break;
      case 1: // PROCESANDO - 4 subpruebas
        if (subprueba >= 4) cambiarEstado = true;
        break;
      case 2: // DETENER - 3 subpruebas
        if (subprueba >= 3) cambiarEstado = true;
        break;
      case 3: // EMERGENCIA - 5 subpruebas
        if (subprueba >= 5) cambiarEstado = true;
        break;
      case 4: // MANUAL - 1 subprueba
        if (subprueba >= 1) cambiarEstado = true;
        break;
    }
    
    if (cambiarEstado) {
      estadoPrincipal++;
      subprueba = 0;
      inicioEstado = millis();
      
      if (estadoPrincipal > 4) {
        estadoPrincipal = 0;
        Serial.println("\n \n \n🎯 ========= CICLO DE PRUEBAS COMPLETADO =========\n \n \n");
      }
      
      // Log del cambio de estado principal
      Serial.println("=====================================================================================================================");
      Serial.print("🏁 CAMBIO A ESTADO PRINCIPAL: ");
      switch (estadoPrincipal) {
        case 0: Serial.println("APAGADO"); break;
        case 1: Serial.println("PROCESANDO"); break;
        case 2: Serial.println("DETENER"); break;
        case 3: Serial.println("EMERGENCIA"); break;
        case 4: Serial.println("MANUAL"); break;
      }
      Serial.println("=====================================================================================================================");
    }
  }

  // Ejecutar la subprueba actual según el estado principal
  switch (estadoPrincipal) {
    
    // ================= ESTADO: APAGADO =================
    case 0: 
      testApagado(); // Solo 1 prueba para APAGADO
      break;
      
    // ================= ESTADO: PROCESANDO =================
    case 1: 
      switch (subprueba) {
        case 0: testProcesando(0); break; // Nivel medio, horno caliente
        case 1: testProcesando(1); break; // Tanque lleno, máxima eficiencia
        case 2: testProcesando(2); break; // Nivel bajo, necesita llenado
        case 3: testProcesando(3); break; // Temperatura límite tanque
      }
      break;
      
    // ================= ESTADO: DETENER =================
    case 2: 
      switch (subprueba) {
        case 0: testDetener(0); break; // Por nivel bajo
        case 1: testDetener(1); break; // Por presión baja
        case 2: testDetener(2); break; // Enfriamiento progresivo
      }
      break;
      
    // ================= ESTADO: EMERGENCIA =================
    case 3: 
      switch (subprueba) {
        case 0: testEmergencia(0); break; // Tanque vacío + horno caliente
        case 1: testEmergencia(1); break; // Sobretemperatura en tanque
        case 2: testEmergencia(2); break; // Sobretemperatura en horno
        case 3: testEmergencia(3); break; // Sobretemperatura en cámara
        case 4: testEmergencia(4); break; // Múltiples fallos simultáneos
      }
      break;
      
    // ================= ESTADO: MANUAL =================
    case 4: 
      testManual(); 
      break;
  }
  
}