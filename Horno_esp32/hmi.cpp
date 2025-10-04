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
      estadoStr = "EMERGENCIA!";
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


void actualizarTemperaturas() {
  // Nextion Numbers: configurar en el editor con 2 decimales
  temp1Tanque.setValue((int)(temperaturas[0] * 100));
  temp2Horno.setValue((int)(temperaturas[1] * 100));
  temp3Camara.setValue((int)(temperaturas[2] * 100));
  temp4Salida.setValue((int)(temperaturas[3] * 100));
}


void actualizarNivel() {
  nivel.setValue((int)(nivelTanque));  
}


void actualizarPresion() {
  // Presión como NexNumber con 1 decimal
  presion.setValue((int)(presionActual * 10));  
}


void actualizarActuadores() {
  // Actualizamos textos según estado ON/OFF
  valvula1Salida.setText(digitalRead(VALVULA_1) ? "ON" : "OFF");
  valvula2Entrada.setText(digitalRead(VALVULA_2) ? "ON" : "OFF");
  bomba1.setText(digitalRead(BOMBA_1) ? "ON" : "OFF");
  bomba2.setText(digitalRead(BOMBA_2) ? "ON" : "OFF");
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