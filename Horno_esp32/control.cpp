#include "config.h"
#include "control.h"
#include "hmi.h"

// ================= FUNCIONES DE CONTROL PRINCIPAL =================


void controlarSistema() {
  switch (estadoActual) {
    case APAGADO:
      apagarTodo();
      break;
    case DETENER:
      detenerSistema();
      break;
    case PROCESANDO:
      iniciarSistema();
      break;
    case EMERGENCIA:
      break;
    case MANUAL:
      manual();
      break;
    default:
      Serial.println("ESTADO DESCONOCIDO");
      break;
  }
}

void iniciarSistema() {
  if (!emergencia && verificarCondicionesInicio()) {
    
    mensajesHMI("Iniciando sistema");
    if ((temperaturas[1] > TEMP_MIN_HORNO) && (temperaturas[2] > TEMP_MIN_HORNO)) {
      if(nivelTanque < NIVEL_MITAD){
        digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque

        // Apagar bombas y la llave de salida del agua caliente
        digitalWrite(BOMBA_1, LOW);
        digitalWrite(BOMBA_2, LOW);
        digitalWrite(VALVULA_1, LOW);
      }
      else if((nivelTanque >= NIVEL_MITAD) && (nivelTanque < NIVEL_MITAD)){
        digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque
        // digitalWrite(VALVULA_1, LOW);
        alternarBombas(); // Encender bombas de manera alternada
      }
      else if(nivelTanque >= NIVEL_MITAD){
        digitalWrite(VALVULA_2, LOW); // Cerrar la llave para llenar el tanque
        // digitalWrite(VALVULA_1, HIGH);
        alternarBombas();
      }
    } else {
      // La temperatura del horno está muy baja como para iniciar
      // Se debe mantener todo apagado mientras se calienta el horno
      digitalWrite(BOMBA_1, LOW);
      digitalWrite(BOMBA_2, LOW);
      //digitalWrite(VALVULA_1, LOW);

      if(nivelTanque < NIVEL_MITAD){
        digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque 
      }
      else if(nivelTanque >= NIVEL_MITAD){
        digitalWrite(VALVULA_2, LOW); // Cerrar la llave para llenar el tanque
      }
    }
  }
}

bool verificarCondicionesInicio() {
  // Verificar que todas las termocuplas funcionen
  for (int i = 0; i < 4; i++) {
    if (temperaturas[i] <= -999.0) {
      mensajesHMI("Error: Verificar sensores temp");
      return false;
    }
  }
  
  // Verificar que no haya emergencias activas
  if (emergencia) {
    mensajesHMI("No se puede iniciar: Emergencia");
    return false;
  }
  
  // Verificar nivel mínimo para iniciar
  if (nivelTanque < NIVEL_VACIO) {
    mensajesHMI("Nivel muy bajo para iniciar");
    return false;
  }
  
  return true;
}

void detenerSistema(){
  estadoActual = DETENER;

  if ((temperaturas[1] < TEMP_MIN_HORNO) && (temperaturas[2] < TEMP_MIN_HORNO)){
    apagarTodo();
  }
  else if((temperaturas[1] >= TEMP_MIN_HORNO) && (temperaturas[2] >= TEMP_MIN_HORNO) && (temperaturas[0] < TEMP_MAX_TANQUE) && (nivelTanque > NIVEL_VACIO)){
    alternarBombas();
  }
  else if((temperaturas[1] >= TEMP_MIN_HORNO) && (temperaturas[2] >= TEMP_MIN_HORNO) && (temperaturas[0] >= TEMP_MAX_TANQUE) && (nivelTanque > NIVEL_VACIO)){
    digitalWrite(VALVULA_2, HIGH);
    alternarBombas();
  }
}

// ================= FUNCIONES DE CONTROL DE ACTUADORES =================

void activarCirculacion() {
  // Activar la bomba principal o redundante según alternancia
  if (bombaPrincipalActiva) {
    digitalWrite(BOMBA_1, HIGH);
    digitalWrite(BOMBA_2, LOW);
  } else {
    digitalWrite(BOMBA_1, LOW);
    digitalWrite(BOMBA_2, HIGH);
  }
}

void alternarBombas() {
  // Alternar bombas cada intervalo definido
  if (millis() - ultimoCambioBomba >= INTERVALO_CAMBIO_BOMBA) {
    bombaPrincipalActiva = !bombaPrincipalActiva;
    ultimoCambioBomba = millis();
    
    // Aplicar el cambio inmediatamente si el sistema está activo
    if (estadoActual != APAGADO && estadoActual != EMERGENCIA) {
      activarCirculacion();
    }
    
  }
}

void manual(){
  estadoActual = AUTOMATICO;

  if (!emergencia) {
    
    digitalWrite(VALVULA_1, valvula_1_auto ? HIGH : LOW);
    digitalWrite(VALVULA_2, valvula_2_auto ? HIGH : LOW);
    digitalWrite(BOMBA_1,   bomba_1_auto   ? HIGH : LOW);
    digitalWrite(BOMBA_2,   bomba_2_auto   ? HIGH : LOW);

  } else {
    detenerSistema();
  }
}

