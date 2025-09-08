#include "hmi.h"
#include "config.h"
#include "control.h"
#include "seguridad.h"

// ================= FUNCIONES DE ACTUALIZACIÓN HMI =================

void actualizarHMI() {
  // Actualizar temperaturas
  actualizarTextoHMI(temp1Text, temperaturas[0], "°C");
  actualizarTextoHMI(temp2Text, temperaturas[1], "°C");
  actualizarTextoHMI(temp3Text, temperaturas[2], "°C");
  actualizarTextoHMI(temp4Text, temperaturas[3], "°C");
  
  // Actualizar niveles
  actualizarTextoHMI(nivel1Text, niveles[0], "%");
  actualizarTextoHMI(nivel2Text, niveles[1], "%");
  actualizarTextoHMI(nivel3Text, niveles[2], "%");
  
  // Actualizar presión
  char presionStr[10];
  dtostrf(presionActual, 4, 1, presionStr);
  strcat(presionStr, " bar");
  presionText.setText(presionStr);
  
  // Actualizar estados de válvulas y bombas
  actualizarEstadoComponente(valvula1State, digitalRead(VALVULA_1) == HIGH);
  actualizarEstadoComponente(valvula2State, digitalRead(VALVULA_2) == HIGH);
  actualizarEstadoComponente(bomba1State, digitalRead(BOMBA_1) == HIGH);
  actualizarEstadoComponente(bomba2State, digitalRead(BOMBA_2) == HIGH);
  
  // Actualizar estado del sistema
  actualizarEstadoSistemaHMI();
}

void actualizarTextoHMI(NexText &componente, double valor, const char* unidad) {
  char buffer[20];
  
  if (valor <= -999.0) {
    // Error en sensor
    strcpy(buffer, "ERROR");
  } else if (valor < -100.0) {
    // Valor fuera de rango bajo
    strcpy(buffer, "LOW");
  } else if (valor > 300.0) {
    // Valor fuera de rango alto
    strcpy(buffer, "HIGH");
  } else {
    // Valor normal, formatear con unidad
    dtostrf(valor, 5, 1, buffer);
    strcat(buffer, unidad);
  }
  
  componente.setText(buffer);
}

void actualizarEstadoComponente(NexPicture &componente, bool estado) {
  // 1 = ON/Activo, 0 = OFF/Inactivo
  // Asegurar que los pics en Nextion estén en este orden:
  // pic0 = componente apagado
  // pic1 = componente encendido
  componente.setPic(estado ? 1 : 0);
}

void actualizarEstadoSistemaHMI() {
  const char* estadoStr;
  uint32_t color = 0; // 0 = Negro (default)
  
  switch (estadoActual) {
    case SISTEMA_APAGADO:
      estadoStr = "APAGADO";
      color = 63488; // Rojo para apagado
      break;
    case LLENADO_TANQUE:
      estadoStr = "LLENANDO";
      color = 65504; // Amarillo para llenado
      break;
    case CALENTAMIENTO:
      estadoStr = "CALENTANDO";
      color = 64512; // Naranja para calentamiento
      break;
    case CIRCULACION:
      estadoStr = "CIRCULANDO";
      color = 1024;  // Verde para circulación
      break;
    case ENTREGA_AGUA:
      estadoStr = "ENTREGANDO";
      color = 2016;  // Azul claro para entrega
      break;
    case EMERGENCIA:
      estadoStr = "EMERGENCIA!";
      color = 63488; // Rojo brillante para emergencia
      break;
    case MANTENIMIENTO:
      estadoStr = "MANTENIMIENTO";
      color = 50712; // Morado para mantenimiento
      break;
    default:
      estadoStr = "DESCONOCIDO";
      color = 0;     // Negro para desconocido
      break;
  }
  
  estadoText.setText(estadoStr);
  
  // Opcional: Cambiar color del texto según estado
  // estadoText.setFontColor(color);
}
void mensajesHMI(const String& mensaje) {
//void mensajesHMI(const char* mensaje) {
  // Esta función puede implementarse de diferentes formas dependiendo
  // de cómo tengas configurado el display Nextion
  
  // Método 1: Usar un componente de texto específico para mensajes
  // NexText mensajeText(0, 20, "mensaje"); // Si tienes un componente para mensajes
  // mensajeText.setText(mensaje);
  
  // Método 2: Usar la consola serial de Nextion (si está disponible)
  // nextionSerial.print("tmsg.txt=\"");
  // nextionSerial.print(mensaje);
  // nextionSerial.print("\"");
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
  
  // Método 3: Mostrar en pantalla principal (depende de tu diseño HMI)
  Serial.print("HMI: ");
  Serial.println(mensaje);
  
  // Para una implementación más avanzada, podrías tener un sistema
  // de cola de mensajes que muestre los mensajes por un tiempo limitado
}

// ================= CALLBACKS NEXTION =================

void startBtnCallback(void *ptr) {
  Serial.println("🟢 Botón START presionado en HMI");
  
  if (estadoActual == SISTEMA_APAGADO) {
    if (!emergencia) {
      iniciarSistema();
    } else {
      mensajesHMI("No se puede iniciar: Emergencia activa");
      Serial.println("❌ No se puede iniciar - Emergencia activa");
    }
  } else {
    mensajesHMI("Sistema ya está en funcionamiento");
    Serial.println("ℹ️  Sistema ya está en funcionamiento");
  }
  
  // Feedback visual opcional
  // startBtn.setPic(2); // Cambiar a imagen de botón presionado
  // delay(200);
  // startBtn.setPic(0); // Volver a imagen normal
}

void stopBtnCallback(void *ptr) {
  Serial.println("🔴 Botón STOP presionado en HMI");
  
  if (estadoActual != SISTEMA_APAGADO && estadoActual != EMERGENCIA) {
    detenerSistema();
  } else if (estadoActual == EMERGENCIA) {
    mensajesHMI("Use RESET para salir de emergencia");
    Serial.println("ℹ️  Sistema en emergencia - Use RESET");
  } else {
    mensajesHMI("Sistema ya está apagado");
    Serial.println("ℹ️  Sistema ya está apagado");
  }
  
  // Feedback visual opcional
  // stopBtn.setPic(2); // Cambiar a imagen de botón presionado
  // delay(200);
  // stopBtn.setPic(0); // Volver a imagen normal
}

void resetBtnCallback(void *ptr) {
  Serial.println("🔄 Botón RESET presionado en HMI");
  
  if (estadoActual == EMERGENCIA) {
    // Verificar que todas las condiciones de emergencia se hayan resuelto
    bool condicionesSeguras = true;
    String mensajeError = "";
    
    if (presionActual > PRESION_MAXIMA) {
      condicionesSeguras = false;
      mensajeError = "Presión alta";
    } else if (temperaturas[0] > TEMP_MAX_TANQUE) {
      condicionesSeguras = false;
      mensajeError = "Temp tanque alta";
    } else if (temperaturas[1] > TEMP_MAX_HORNO) {
      condicionesSeguras = false;
      mensajeError = "Temp horno alta";
    } else if (temperaturas[2] > TEMP_MAX_CAMARA) {
      condicionesSeguras = false;
      mensajeError = "Temp cámara alta";
    } else if (digitalRead(EMO_BTN) == LOW) {
      condicionesSeguras = false;
      mensajeError = "Pulsador EMO activado";
    }
    
    if (condicionesSeguras) {
      emergencia = false;
      estadoActual = SISTEMA_APAGADO;
      mensajesHMI("Emergencia reseteda - Sistema apagado");
      Serial.println("✅ Emergencia reseteda");
    } else {
      mensajesHMI("No se puede resetear: " + mensajeError);
      Serial.print("❌ No se puede resetear: ");
      Serial.println(mensajeError);
    }
  } else {
    mensajesHMI("Reset solo disponible en emergencia");
    Serial.println("ℹ️  Reset solo funciona en modo emergencia");
  }
  
  // Feedback visual opcional
  // resetBtn.setPic(2); // Cambiar a imagen de botón presionado
  // delay(200);
  // resetBtn.setPic(0); // Volver a imagen normal
}

// ================= FUNCIONES AUXILIARES HMI =================

void actualizarTodasLasTemperaturas() {
  // Función auxiliar para actualizar solo temperaturas
  actualizarTextoHMI(temp1Text, temperaturas[0], "°C");
  actualizarTextoHMI(temp2Text, temperaturas[1], "°C");
  actualizarTextoHMI(temp3Text, temperaturas[2], "°C");
  actualizarTextoHMI(temp4Text, temperaturas[3], "°C");
}

void actualizarTodosLosNiveles() {
  // Función auxiliar para actualizar solo niveles
  actualizarTextoHMI(nivel1Text, niveles[0], "%");
  actualizarTextoHMI(nivel2Text, niveles[1], "%");
  actualizarTextoHMI(nivel3Text, niveles[2], "%");
}

void actualizarTodosLosActuadores() {
  // Función auxiliar para actualizar solo actuadores
  actualizarEstadoComponente(valvula1State, digitalRead(VALVULA_1) == HIGH);
  actualizarEstadoComponente(valvula2State, digitalRead(VALVULA_2) == HIGH);
  actualizarEstadoComponente(bomba1State, digitalRead(BOMBA_1) == HIGH);
  actualizarEstadoComponente(bomba2State, digitalRead(BOMBA_2) == HIGH);
}

void mostrarMensajeTemporal(const char* mensaje, unsigned long duracionMs) {
  // Función para mostrar mensajes temporales (si el HMI lo soporta)
  mensajesHMI(mensaje);
  
  // Podrías implementar un sistema de temporizador aquí
  // para limpiar el mensaje después de duracionMs
}

void cambiarPaginaHMI(uint8_t paginaId) {
  // Función para cambiar de página en el HMI
  // nextionSerial.print("page ");
  // nextionSerial.print(paginaId);
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
}

// ================= MANEJO DE ALARMAS VISUALES =================

void indicarAlarma(bool estadoAlarma) {
  // Función para indicar alarma visual en HMI
  if (estadoAlarma) {
    // Activar indicador de alarma (parpadeo, color rojo, etc.)
    // nextionSerial.print("vis alarm_icon,1");
    // nextionSerial.write(0xFF);
    // nextionSerial.write(0xFF);
    // nextionSerial.write(0xFF);
  } else {
    // Desactivar indicador de alarma
    // nextionSerial.print("vis alarm_icon,0");
    // nextionSerial.write(0xFF);
    // nextionSerial.write(0xFF);
    // nextionSerial.write(0xFF);
  }
}

void parpadearComponente(const char* nombreComponente, int velocidad) {
  // Función para hacer parpadear un componente (para alertas)
  // nextionSerial.print(nombreComponente);
  // nextionSerial.print(".bco=65535");
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
  // delay(velocidad);
  // nextionSerial.print(nombreComponente);
  // nextionSerial.print(".bco=0");
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
  // nextionSerial.write(0xFF);
}
