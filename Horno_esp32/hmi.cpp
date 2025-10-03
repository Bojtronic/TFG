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
  nivel.setValue((int)(niveles[0]));  
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

