#include "test.h"

// ================= PRUEBAS POR ESTADO =================

// Estado: APAGADO → todo frío y sin actividad
void testApagado() {
  Serial.println("🔹 TEST: Estado APAGADO");
  temperaturas[0] = 25;   // Tanque frio
  temperaturas[1] = 25;   // Horno frio
  temperaturas[2] = 25;   // Camara
  temperaturas[3] = 25;   // Salida
  nivelTanque     = 0;    // Vacío
  presionActual   = 0;    // Sin presión
}

// Estado: PROCESANDO → condiciones listas
void testProcesando() {
  Serial.println("🔹 TEST: Estado PROCESANDO");
  temperaturas[0] = 50;
  temperaturas[1] = 180;
  temperaturas[2] = 200;
  temperaturas[3] = 210;
  nivelTanque     = 60;   // Medio
  presionActual   = 5;
}

// Estado: DETENER → varias razones
void testDetener(int caso) {
  switch (caso) {
    case 0:
      Serial.println("🔹 TEST: DETENER por nivel bajo");
      temperaturas[0] = 180;
      temperaturas[1] = 200;
      temperaturas[2] = 190;
      temperaturas[3] = 180;
      nivelTanque     = 30;  // Bajo
      presionActual   = 4;
      break;

    case 1:
      Serial.println("🔹 TEST: DETENER por presion baja");
      temperaturas[0] = 150;
      temperaturas[1] = 180;
      temperaturas[2] = 170;
      temperaturas[3] = 160;
      nivelTanque     = 60;
      presionActual   = 0.5; // Presión baja
      break;
  }
}

// Estado: MANUAL → forzado
void testManual() {
  Serial.println("🔹 TEST: Estado MANUAL");
  temperaturas[0] = 50;
  temperaturas[1] = 60;
  temperaturas[2] = 55;
  temperaturas[3] = 50;
  nivelTanque     = 100;
  presionActual   = 5;
}

// Estado: EMERGENCIA → múltiples tipos
void testEmergencia(int caso) {
  switch (caso) {
    case 0:
      Serial.println("🔹 EMERGENCIA: Tanque vacío + horno caliente");
      temperaturas[0] = 200;
      temperaturas[1] = 400;
      temperaturas[2] = 410;
      temperaturas[3] = 390;
      nivelTanque     = 0;
      presionActual   = 0.5;
      break;

    case 1:
      Serial.println("🔹 EMERGENCIA: Sobretemperatura en tanque");
      temperaturas[0] = 80;
      temperaturas[1] = 100;
      temperaturas[2] = 90;
      temperaturas[3] = 85;
      nivelTanque     = 70;
      presionActual   = 5;
      break;

    case 2:
      Serial.println("🔹 EMERGENCIA: Sobretemperatura en horno");
      temperaturas[0] = 60;
      temperaturas[1] = 250;
      temperaturas[2] = 200;
      temperaturas[3] = 190;
      nivelTanque     = 50;
      presionActual   = 5;
      break;

    case 3:
      Serial.println("🔹 EMERGENCIA: Sobretemperatura en cámara");
      temperaturas[0] = 50;
      temperaturas[1] = 100;
      temperaturas[2] = 180;
      temperaturas[3] = 160;
      nivelTanque     = 40;
      presionActual   = 5;
      break;
  }
}

// ================= CONTROLADOR DE PRUEBAS =================
void ejecutarPruebas() {
  static int paso = 0;
  static unsigned long inicioPaso = millis();

  if (millis() - inicioPaso < 15000) {
    // Durante 10s mantener el estado actual
    switch (paso) {
      case 0: testApagado(); break;
      case 1: testProcesando(); break;
      case 2: testDetener(0); break;
      case 3: testDetener(1); break;
      case 4: testManual(); break;
      case 5: testEmergencia(0); break;
      case 6: testEmergencia(1); break;
      case 7: testEmergencia(2); break;
      case 8: testEmergencia(3); break;
    }
  } else {
    // Pasados 15s cambiar de escenario
    paso = (paso + 1) % 9; // 9 pruebas
    inicioPaso = millis();
  }
}
