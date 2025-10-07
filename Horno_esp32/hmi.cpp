#include "hmi.h"
#include "config.h"
#include "control.h"
#include "seguridad.h"

// ================= FUNCIONES DE ACTUALIZACIÓN =================

void actualizarEstadoSistemaHMI() {
  const char* estadoStr;

  switch (estadoActual) {
    case APAGADO:
      estadoStr = "APAGADO";
      break;
    case DETENER:
      estadoStr = "DETENER";
      break;
    case PROCESANDO:
      estadoStr = "PROCESANDO";
      break;
    case EMERGENCIA:
      estadoStr = "EMERGENCIA";
      break;
    case MANUAL:
      estadoStr = "MANUAL";
      break;
    default:
      estadoStr = "DESCONOCIDO";
      break;
  }

  estado.setText(estadoStr);  
}


void actualizarTemperaturaTanque() {
  char buffer[6];
  
  snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[0]);
  temp1Tanque.setText(buffer);
}

void actualizarTemperaturaHorno() {
  char buffer[6];
  
  snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[1]);
  temp2Horno.setText(buffer);
}

void actualizarTemperaturaCamara() {
  char buffer[6];
  
  snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[2]); 
  temp3Camara.setText(buffer);
}

void actualizarTemperaturaSalida() {
  char buffer[6];
  
  snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[3]);
  temp4Salida.setText(buffer);
}


void actualizarNivel() {
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%d", nivelTanque);
  nivel.setText(buffer);
}


void actualizarPresion() {
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%.1f", presionActual);
  presion.setText(buffer);
}

void actualizarBomba1() {
  bomba1.setText(digitalRead(BOMBA_1) ? "ON" : "OFF");
}

void actualizarBomba2() {
  bomba2.setText(digitalRead(BOMBA_2) ? "ON" : "OFF");
}

void actualizarValvula1() {
  valvula1Salida.setText(digitalRead(VALVULA_1) ? "ON" : "OFF");
}

void actualizarValvula2() {
  valvula2Entrada.setText(digitalRead(VALVULA_2) ? "ON" : "OFF");
}


// ================= CALLBACKS DE BOTONES =================

void startBtnCallback(void *ptr) {
  Serial.println("🟢 Botón START presionado en HMI");

  if (estadoActual != EMERGENCIA && estadoActual != APAGADO) {
    estadoActual = PROCESANDO;
  }
}

void stopBtnCallback(void *ptr) {
  Serial.println("🔴 Botón STOP presionado en HMI");

  if (estadoActual != EMERGENCIA && estadoActual != APAGADO) {
    estadoActual = DETENER;
  }
}

void manualBtnCallback(void *ptr) {
  Serial.println("🔄 Botón MANUAL presionado en HMI");

  if (estadoActual != EMERGENCIA && estadoActual != APAGADO) {
    estadoActual = MANUAL;
  }
}

// Válvula 1 ON/OFF
void valvula1BtnCallback(void *ptr) {
  Serial.println("💧 Botón VÁLVULA 1 presionado en HMI");
  
  if (estadoActual == MANUAL) {
    valvula_1_auto = !valvula_1_auto;  // Alternar estado
    digitalWrite(VALVULA_1, valvula_1_auto ? HIGH : LOW);
  }
}

// Válvula 2 ON/OFF
void valvula2BtnCallback(void *ptr) {
  Serial.println("💧 Botón VÁLVULA 2 presionado en HMI");
  
  if (estadoActual == MANUAL) {
    valvula_2_auto = !valvula_2_auto;
    digitalWrite(VALVULA_2, valvula_2_auto ? HIGH : LOW);
  }
}

// Bomba 1 ON/OFF
void bomba1BtnCallback(void *ptr) {
  Serial.println("💦 Botón BOMBA 1 presionado en HMI");
  
  if (estadoActual == MANUAL) {
    bomba_1_auto = !bomba_1_auto;
    digitalWrite(BOMBA_1, bomba_1_auto ? HIGH : LOW);
  }
}

// Bomba 2 ON/OFF
void bomba2BtnCallback(void *ptr) {
  Serial.println("💦 Botón BOMBA 2 presionado en HMI");
  
  if (estadoActual == MANUAL) {
    bomba_2_auto = !bomba_2_auto;
    digitalWrite(BOMBA_2, bomba_2_auto ? HIGH : LOW);
  }
}