#include "config.h"
#include "control.h"
#include "hmi.h"

// ================= FUNCIONES DE CONTROL PRINCIPAL =================


void controlarSistema() {
  switch (estadoActual) {
    case APAGADO:
      Serial.println("Estado APAGADO");
      if(verificarCondicionesApagado()){
        apagarTodo();
        mensajeActual = APAGADO_0;
      }
      else{
        detenerSistema();
      }
      break;
    case DETENER:
      Serial.println(">>>   Estado DETENER");
      detenerSistema();
      break;
    case PROCESANDO:
      Serial.println(">>>   Estado PROCESANDO");
      iniciarSistema();
      break;
    case EMERGENCIA:
      Serial.println(">>>   Estado EMERGENCIA");
      detenerSistema();
      break;
    case MANUAL:
      Serial.println(">>>   Estado MANUAL");
      manual();
      mensajeActual = MANUAL_0;
      break;
    default:
      Serial.println(">>>   ESTADO DESCONOCIDO");
      mensajeActual = DESCONOCIDO;
      detenerSistema();
      break;
  }
}

void iniciarSistema() {
  if (verificarCondicionesInicio()) {
    estadoActual = PROCESANDO;
    //mensajesHMI("Iniciando sistema");
    if ((temperaturas[1] > TEMP_MIN_HORNO) && (temperaturas[2] > TEMP_MIN_HORNO)) {
      if(nivelTanque < NIVEL_MITAD){
        mensajeActual = PROCESANDO_1;
        
        digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque

        // Apagar bombas y la llave de salida del agua caliente
        digitalWrite(BOMBA_1, LOW);
        digitalWrite(BOMBA_2, LOW);
        digitalWrite(VALVULA_1, LOW);
      }
      else if((nivelTanque >= NIVEL_MITAD) && (nivelTanque < NIVEL_LLENO)){
        mensajeActual = PROCESANDO_2;

        digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque
        // digitalWrite(VALVULA_1, LOW);
        alternarBombas(); // Encender bombas de manera alternada
      }
      else if(nivelTanque >= NIVEL_LLENO){
        mensajeActual = PROCESANDO_3;

        digitalWrite(VALVULA_2, LOW); // Cerrar la llave para llenar el tanque
        // digitalWrite(VALVULA_1, HIGH);
        alternarBombas();
      }
    } else {
      // La temperatura del horno está muy baja como para iniciar
      // Se debe mantener todo apagado mientras se calienta el horno
      digitalWrite(BOMBA_1, LOW);
      digitalWrite(BOMBA_2, LOW);
      digitalWrite(VALVULA_1, LOW);

      if(nivelTanque < NIVEL_MITAD){
        mensajeActual = PROCESANDO_4;

        digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque 
      }
      else if(nivelTanque >= NIVEL_MITAD){
        mensajeActual = PROCESANDO_5;

        digitalWrite(VALVULA_2, LOW); // Cerrar la llave cuando el tanque esta a la mitad
      }
    }
  }
}

bool verificarCondicionesInicio() {
  // Verificar que todas las termocuplas funcionen
  if (thermocouple1.readError()) {
    Serial.println("Error en termocupla 1");
    return false;
  }
  if (thermocouple2.readError()) {
    Serial.println("Error en termocupla 2");
    return false;
  }
  if (thermocouple3.readError()) {
    Serial.println("Error en termocupla 3");
    return false;
  }
  if (thermocouple4.readError()) {
    Serial.println("Error en termocupla 4");
    return false;
  }

  for (int i = 0; i < 4; i++) {
    if (temperaturas[i] <= -999.0) {
      //mensajesHMI("Error: Verificar sensores temp");
      Serial.println("Error: Verificar sensores temp");
      return false;
    }

    if (temperaturas[i] < 0 || temperaturas[i] > 600) {
      Serial.print("Lectura fuera de rango en sensor ");
      Serial.println(i+1);
      return false;
    }
  }
    
  
  // Verificar presión de agua mínima para iniciar
  if (presionActual <= PRESION_MINIMA) {
    //mensajesHMI("No hay agua suficiente para iniciar");
    Serial.println("No hay agua suficiente para iniciar");
    //detenerSistema(); // considerar ejecutar el apagado seguro en este punto
    return false;
  }

  // Verificar nivel mínimo para iniciar
  if (nivelTanque <= NIVEL_VACIO) {
    //mensajesHMI("Nivel muy bajo para iniciar");
    Serial.println("Nivel muy bajo para iniciar");
    digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
    return false;
  }
  
  return true;
}

void detenerSistema(){
  // Condiciones para apagar el sistema de forma segura
  if ((temperaturas[1] <= TEMP_MIN_HORNO) && (temperaturas[2] <= TEMP_MIN_HORNO)){
    apagarTodo();
    estadoActual = APAGADO;
  }
  else if((temperaturas[1] > TEMP_MIN_HORNO) && (temperaturas[2] > TEMP_MIN_HORNO) && (nivelTanque > NIVEL_MITAD)){
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, LOW);
    alternarBombas();   // circulacion de agua mientras se enfria el horno
    estadoActual = DETENER;
    mensajeActual = DETENER_1;
  }
  else if((presionActual > PRESION_MINIMA) && (temperaturas[1] >= TEMP_MIN_HORNO) && (temperaturas[2] >= TEMP_MIN_HORNO) && (nivelTanque <= NIVEL_MITAD) && (nivelTanque > NIVEL_VACIO)){
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
    alternarBombas();   // circulacion de agua mientras se enfria el horno
    estadoActual = DETENER;
    mensajeActual = DETENER_2;
  }
  else if((presionActual < PRESION_MINIMA) && (temperaturas[1] >= (TEMP_MIN_HORNO*2)) && (temperaturas[2] >= (TEMP_MIN_HORNO*2)) && (nivelTanque <= NIVEL_VACIO)){
    digitalWrite(BOMBA_1, LOW);
    digitalWrite(BOMBA_2, LOW);
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, HIGH); // Abrir la llave para que llegue un poco de agua fria al tanque si es que hay un poco de agua
    estadoActual = EMERGENCIA;
    mensajeActual = EMERGENCIA_1;
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
    if (estadoActual != APAGADO && estadoActual != MANUAL && estadoActual != EMERGENCIA) {
      activarCirculacion();
    } 
  }
}

void manual(){
  digitalWrite(VALVULA_1, valvula_1_auto ? HIGH : LOW);
  digitalWrite(VALVULA_2, valvula_2_auto ? HIGH : LOW);
  digitalWrite(BOMBA_1,   bomba_1_auto   ? HIGH : LOW);
  digitalWrite(BOMBA_2,   bomba_2_auto   ? HIGH : LOW);
  //verificarSeguridad();
}

bool verificarCondicionesApagado(){
  if ((temperaturas[1] <= TEMP_MIN_HORNO) && (temperaturas[2] <= TEMP_MIN_HORNO)) {
    return true;
  } else {
    return false;
  }
}
