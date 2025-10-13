#include "config.h"
#include "hmi.h"

// ================= FUNCIONES DE ACTUALIZACIÓN =================

void actualizarEstadoSistemaHMI()
{
  const char *estadoStr;

  switch (estadoActual)
  {
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

  if (strcmp(lastEstado, estadoStr) != 0)
  {
    estado.setText(estadoStr);
    strcpy(lastEstado, estadoStr);
  }
}

void actualizarTemperaturaTanque()
{
  if (temperaturas[0] != lastTanqueTemp)
  {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[0]);
    temp1Tanque.setText(buffer);
    lastTanqueTemp = temperaturas[0];
  }
}

void actualizarTemperaturaHorno()
{
  if (temperaturas[1] != lastHornoTemp)
  {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[1]);
    temp2Horno.setText(buffer);
    lastHornoTemp = temperaturas[1];
  }
}

void actualizarTemperaturaCamara()
{
  if (temperaturas[2] != lastCamaraTemp)
  {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[2]);
    temp3Camara.setText(buffer);
    lastCamaraTemp = temperaturas[2];
  }
}

void actualizarTemperaturaSalida()
{
  if (temperaturas[3] != lastSalidaTemp)
  {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%.2f", temperaturas[3]);
    temp4Salida.setText(buffer);
    lastSalidaTemp = temperaturas[3];
  }
}

void actualizarNivel()
{
  if (nivelTanque != lastNivel)
  {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%d", nivelTanque);
    nivel.setText(buffer);
    lastNivel = nivelTanque;
  }
}

void actualizarPresion()
{
  if (presionActual != lastPresion)
  {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%.2f", presionActual);
    presion.setText(buffer);
    lastPresion = presionActual;
  }
}

void actualizarBomba1()
{
  bool current = digitalRead(BOMBA_1);

  if (current != lastBomba1State)
  {
    bomba1.setText(current ? "ON" : "OFF");
    lastBomba1State = current;
  }
}

void actualizarBomba2()
{
  bool current = digitalRead(BOMBA_2);

  if (current != lastBomba2State)
  {
    bomba2.setText(current ? "ON" : "OFF");
    lastBomba2State = current;
  }
}

void actualizarValvula1()
{
  bool current = digitalRead(VALVULA_1);

  if (current != lastValv1State)
  {
    valvula1Salida.setText(current ? "ON" : "OFF");
    lastValv1State = current;
  }
}

void actualizarValvula2()
{
  bool current = digitalRead(VALVULA_2);

  if (current != lastValv2State)
  {
    valvula2Entrada.setText(current ? "ON" : "OFF");
    lastValv2State = current;
  }
}

// ================= CALLBACKS DE BOTONES =================

void startBtnCallback(void *ptr)
{
  //Serial.println("🟢 Botón START presionado en HMI");
  if (estadoActual != EMERGENCIA && estadoActual != APAGADO)
  {
    estadoActual = PROCESANDO;
  }
}

void stopBtnCallback(void *ptr)
{
  //Serial.println("🔴 Botón STOP presionado en HMI");
  if (estadoActual != EMERGENCIA && estadoActual != APAGADO)
  {
    estadoActual = DETENER;
  }
}

void manualBtnCallback(void *ptr)
{
  //Serial.println("🔄 Botón MANUAL presionado en HMI");
  if (estadoActual != EMERGENCIA && estadoActual != APAGADO)
  {
    estadoActual = MANUAL;
  }
}

void valvula1BtnCallback(void *ptr)
{
  //Serial.println("💧 Botón VÁLVULA 1 presionado en HMI");
  if (estadoActual == MANUAL)
  {
    valvula_1_auto = !valvula_1_auto; // Alternar estado
    digitalWrite(VALVULA_1, valvula_1_auto ? HIGH : LOW);
  }
}

void valvula2BtnCallback(void *ptr)
{
  //Serial.println("💧 Botón VÁLVULA 2 presionado en HMI");
  if (estadoActual == MANUAL)
  {
    valvula_2_auto = !valvula_2_auto;
    digitalWrite(VALVULA_2, valvula_2_auto ? HIGH : LOW);
  }
}

void bomba1BtnCallback(void *ptr)
{
  //Serial.println("💦 Botón BOMBA 1 presionado en HMI");
  if (estadoActual == MANUAL)
  {
    bomba_1_auto = !bomba_1_auto;
    digitalWrite(BOMBA_1, bomba_1_auto ? HIGH : LOW);
  }
}

void bomba2BtnCallback(void *ptr)
{
  //Serial.println("💦 Botón BOMBA 2 presionado en HMI");
  if (estadoActual == MANUAL)
  {
    bomba_2_auto = !bomba_2_auto;
    digitalWrite(BOMBA_2, bomba_2_auto ? HIGH : LOW);
  }
}