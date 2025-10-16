#include "config.h"
#include "seguridad.h"
#include "control.h"

void controlarSistema() {
  switch (estadoActual) {
    case APAGADO:
      //Serial.println(">>>   Estado APAGADO");
      if(verificarCondicionesApagado()){
        apagarTodo();
        mensajeActual = APAGADO_0;
      }
      else{
        detenerSistema();
      }
      break;
    case DETENER:
      //Serial.println(">>>   Estado DETENER");
      detenerSistema();
      break;
    case PROCESANDO:
      //Serial.println(">>>   Estado PROCESANDO");
      iniciarSistema();
      break;
    case EMERGENCIA:
      //Serial.println(">>>   Estado EMERGENCIA");
      detenerSistema();
      break;
    case MANUAL:
      //Serial.println(">>>   Estado MANUAL");
      manual();
      mensajeActual = MANUAL_0;
      break;
    default:
      //Serial.println(">>>   ESTADO DESCONOCIDO");
      mensajeActual = DESCONOCIDO;
      detenerSistema();
      break;
  }
}

void iniciarSistema() {
  if (verificarCondicionesInicio()) {
    estadoActual = PROCESANDO;
    
    if ((temperaturas[1] > TEMP_MIN_HORNO) && (temperaturas[2] > TEMP_MIN_HORNO)) {
      if(nivelTanque < NIVEL_MITAD){
        mensajeActual = PROCESANDO_1;
        
        // Abrir la llave para llenar el tanque con agua fria
        digitalWrite(VALVULA_2, HIGH);

        // Apagar bombas y la llave de salida del agua caliente
        digitalWrite(BOMBA_1, LOW);
        digitalWrite(BOMBA_2, LOW);
        digitalWrite(VALVULA_1, LOW);
      }
      else if((nivelTanque >= NIVEL_MITAD) && (nivelTanque < NIVEL_LLENO)){
        mensajeActual = PROCESANDO_2;

        // Abrir la llave para llenar el tanque con agua fria
        digitalWrite(VALVULA_2, HIGH);

        // Cerrar la llave de salida del agua caliente
        digitalWrite(VALVULA_1, LOW);

        // Encender bombas de manera alternada
        alternarBombas(); 
      }
      else if(nivelTanque >= NIVEL_LLENO){
        mensajeActual = PROCESANDO_3;

        // Cerrar la llave de entrada de agua fria
        digitalWrite(VALVULA_2, LOW);
        
        // Encender bombas de manera alternada
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

        // Abrir la llave para llenar el tanque con agua fria
        digitalWrite(VALVULA_2, HIGH);
      }
      else if(nivelTanque >= NIVEL_MITAD){
        mensajeActual = PROCESANDO_5;

        // Cerrar la llave cuando el tanque esta a la mitad o lleno
        digitalWrite(VALVULA_2, LOW); 
      }
    }
  }
  else {
    // No se cumplen las condiciones para iniciar el sistema
    detenerSistema();
  }
}

bool verificarCondicionesInicio() {
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
    if (temperaturas[i] < 0 || temperaturas[i] >= 400.0) {
      //Serial.println("Error: Verificar sensores temp");
      //Serial.print("Lectura fuera de rango en sensor ");
      //Serial.println(i+1);
      return false;
    }
  }
    
  if (presionActual <= PRESION_MINIMA) {
    
    Serial.println("No hay agua suficiente para iniciar");
    return false;
  }

  if (nivelTanque <= NIVEL_VACIO) {
    
    //Serial.println("Nivel muy bajo para iniciar");

    // Abrir la llave para llenar el tanque con agua fria
    digitalWrite(VALVULA_2, HIGH); 
    return false;
  }
  
  return true;
}

void detenerSistema() {
  // Si el sistema está en emergencia, actuar según tipo
  if (estadoActual == EMERGENCIA) {
    switch (mensajeActual) {
      case EMERGENCIA_1:
      case EMERGENCIA_2:
        // No hay presión ni agua → apagar todo
        digitalWrite(BOMBA_1, LOW);
        digitalWrite(BOMBA_2, LOW);
        digitalWrite(VALVULA_1, LOW);
        digitalWrite(VALVULA_2, LOW);
        break;

      case EMERGENCIA_3:
        // No hay presión pero aún queda agua → mantener circulación interna
        digitalWrite(VALVULA_1, LOW);
        digitalWrite(VALVULA_2, LOW);
        alternarBombas();
        break;

      case EMERGENCIA_4:
        // Tanque y horno/cámara calientes → enfriar con agua fría
        digitalWrite(VALVULA_2, HIGH);
        alternarBombas();
        break;

      case EMERGENCIA_5:
        // Tanque caliente pero horno/cámara fríos → detener todo
        digitalWrite(VALVULA_1, LOW);
        digitalWrite(VALVULA_2, LOW);
        digitalWrite(BOMBA_1, LOW);
        digitalWrite(BOMBA_2, LOW);
        break;

      case EMERGENCIA_6:
        // Horno muy caliente, tanque no lleno → llenar y circular
        digitalWrite(VALVULA_2, HIGH);
        alternarBombas();
        break;

      case EMERGENCIA_7:
        // Horno muy caliente, tanque lleno → solo circulación
        digitalWrite(VALVULA_2, LOW);
        alternarBombas();
        break;

      case EMERGENCIA_8:
        // Cámara muy caliente → enfriar con agua fría
        digitalWrite(VALVULA_2, HIGH);
        alternarBombas();
        break;
    }
    return;
  }

  // Si no está en emergencia, controlar proceso de detención normal

  // Caso 1: horno y cámara aún calientes → mantener enfriamiento
  if ((temperaturas[1] > TEMP_MIN_HORNO || temperaturas[2] > TEMP_MIN_HORNO)) {

    // Si hay presión y nivel suficiente → continuar enfriamiento
    if (presionActual >= PRESION_MINIMA && nivelTanque > NIVEL_VACIO) {
      digitalWrite(VALVULA_1, LOW);
      digitalWrite(VALVULA_2, LOW);
      alternarBombas();
      estadoActual = DETENER;
      mensajeActual = DETENER_1;
      return;
    }

    // Si no hay presión o tanque vacío → apagar bombas
    if (presionActual < PRESION_MINIMA || nivelTanque <= NIVEL_VACIO) {
      digitalWrite(BOMBA_1, LOW);
      digitalWrite(BOMBA_2, LOW);
      digitalWrite(VALVULA_1, LOW);
      digitalWrite(VALVULA_2, LOW);
      estadoActual = DETENER;
      mensajeActual = DETENER_2; // sin circulación posible
      return;
    }
  }

  // Caso 2: sistema frío y condiciones seguras para apagar
  if (verificarCondicionesApagado()) {
    apagarTodo();
    estadoActual = APAGADO;
  }
}


// ================= FUNCIONES DE CONTROL DE ACTUADORES =================

void activarCirculacion() {
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
  verificarSeguridad();
}

bool verificarCondicionesApagado(){
  if ((temperaturas[1] <= TEMP_MIN_HORNO) && (temperaturas[2] <= TEMP_MIN_HORNO)) {
    return true;
  } else {
    return false;
  }
}
