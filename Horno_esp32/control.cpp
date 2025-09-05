#include "config.h"
#include "control.h"
#include "hmi.h"

// ================= FUNCIONES DE CONTROL PRINCIPAL =================

void iniciarSistema() {
  if (!emergencia && verificarCondicionesInicio()) {
    estadoActual = LLENADO_TANQUE;
    mensajesHMI("Iniciando sistema - Llenando tanque");
    Serial.println("Sistema iniciado - Modo LLENADO");
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
  if (niveles[2] < NIVEL_VACIO) {
    mensajesHMI("Nivel muy bajo para iniciar");
    return false;
  }
  
  return true;
}

void detenerSistema() {
  estadoActual = SISTEMA_APAGADO;
  apagarTodo();
  mensajesHMI("Sistema detenido");
  Serial.println("Sistema detenido");
}

void controlarSistema() {
  switch (estadoActual) {
    case LLENADO_TANQUE:
      controlarLlenado();
      break;
    case CALENTAMIENTO:
      controlarCalentamiento();
      break;
    case CIRCULACION:
      controlarCirculacion();
      break;
    case ENTREGA_AGUA:
      controlarEntregaAgua();
      break;
    default:
      // No hacer nada para otros estados
      break;
  }
}

// ================= FUNCIONES DE SUB-ESTADOS =================

void controlarLlenado() {
  // Abrir válvula 2 (agua fría) si el nivel está bajo
  if (niveles[2] < NIVEL_LLENO) {
    digitalWrite(VALVULA_2, HIGH);
    mensajesHMI("Llenando tanque...");
  } else {
    // Tanque lleno, cerrar válvula y pasar a calentamiento
    digitalWrite(VALVULA_2, LOW);
    estadoActual = CALENTAMIENTO;
    mensajesHMI("Tanque lleno - Iniciando calentamiento");
    Serial.println("Modo CALENTAMIENTO");
  }
}

void controlarCalentamiento() {
  // Activar circulación para calentamiento
  activarCirculacion();
  
  // Verificar si se alcanzó la temperatura objetivo
  if (temperaturas[3] >= TEMP_AGUA_CALIENTE) {
    estadoActual = CIRCULACION;
    mensajesHMI("Temp alcanzada - Circulacion constante");
    Serial.println("Modo CIRCULACION");
  }
  
  // Seguridad: verificar sobrecalentamiento durante calentamiento
  if (temperaturas[0] > TEMP_MAX_TANQUE - 10) { // 10°C debajo del máximo
    mensajesHMI("ALERTA: Temp tanque subiendo mucho");
  }
}

void controlarCirculacion() {
  // Mantener circulación constante
  activarCirculacion();
  
  // Controlar nivel del tanque - rellenar si es necesario
  if (niveles[2] < NIVEL_MITAD) {
    // Nivel bajo, abrir válvula de llenado
    digitalWrite(VALVULA_2, HIGH);
    mensajesHMI("Rellenando tanque...");
  } else if (niveles[2] >= NIVEL_LLENO) {
    // Nivel lleno, cerrar válvula
    digitalWrite(VALVULA_2, LOW);
  }
  
  // Mantener temperatura - mensajes informativos
  if (temperaturas[3] < TEMP_AGUA_CALIENTE - 2.0) {
    mensajesHMI("Manteniendo temperatura...");
  }
  
  // Verificar si se solicita agua caliente (condiciones para entrega)
  if (temperaturas[3] >= TEMP_AGUA_CALIENTE && niveles[0] > NIVEL_VACIO) {
    estadoActual = ENTREGA_AGUA;
    mensajesHMI("Entregando agua caliente");
    Serial.println("Modo ENTREGA_AGUA");
  }
}

void controlarEntregaAgua() {
  // Abrir válvula 1 (salida agua caliente)
  digitalWrite(VALVULA_1, HIGH);
  
  // Mantener circulación durante la entrega
  activarCirculacion();
  
  // Verificar condiciones para finalizar la entrega
  bool nivelBajo = niveles[0] <= NIVEL_VACIO;
  bool tempBaja = temperaturas[3] < TEMP_AGUA_CALIENTE - 5.0;
  
  if (nivelBajo || tempBaja) {
    digitalWrite(VALVULA_1, LOW);
    estadoActual = CIRCULACION;
    
    if (nivelBajo) {
      mensajesHMI("Entrega completada - Nivel bajo");
    } else {
      mensajesHMI("Entrega completada - Temp baja");
    }
    Serial.println("Volviendo a modo CIRCULACION");
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
    
    Serial.print("Alternando bombas. Activa: ");
    Serial.println(bombaPrincipalActiva ? "Bomba 1" : "Bomba 2");
    
    // Aplicar el cambio inmediatamente si el sistema está activo
    if (estadoActual != SISTEMA_APAGADO && estadoActual != EMERGENCIA) {
      activarCirculacion();
    }
  }
}

// ================= FUNCIONES AUXILIARES DE CONTROL =================

void forzarLlenado() {
  // Función auxiliar para forzar llenado (podría usarse desde comandos)
  if (estadoActual == SISTEMA_APAGADO || estadoActual == EMERGENCIA) {
    digitalWrite(VALVULA_2, HIGH);
    mensajesHMI("Llenado forzado activo");
  }
}

void detenerLlenado() {
  // Función auxiliar para detener llenado
  digitalWrite(VALVULA_2, LOW);
  if (estadoActual == SISTEMA_APAGADO || estadoActual == EMERGENCIA) {
    mensajesHMI("Llenado forzado detenido");
  }
}

void forzarCirculacion() {
  // Función auxiliar para forzar circulación
  if (estadoActual == SISTEMA_APAGADO || estadoActual == EMERGENCIA) {
    activarCirculacion();
    mensajesHMI("Circulacion forzada activa");
  }
}

void detenerCirculacion() {
  // Función auxiliar para detener circulación
  digitalWrite(BOMBA_1, LOW);
  digitalWrite(BOMBA_2, LOW);
  if (estadoActual == SISTEMA_APAGADO || estadoActual == EMERGENCIA) {
    mensajesHMI("Circulacion forzada detenida");
  }
}

// ================= FUNCIONES DE ESTADO DEL SISTEMA =================

const char* obtenerNombreEstado(EstadoSistema estado) {
  switch (estado) {
    case SISTEMA_APAGADO: return "APAGADO";
    case LLENADO_TANQUE: return "LLENANDO";
    case CALENTAMIENTO: return "CALENTANDO";
    case CIRCULACION: return "CIRCULANDO";
    case ENTREGA_AGUA: return "ENTREGANDO";
    case EMERGENCIA: return "EMERGENCIA";
    case MANTENIMIENTO: return "MANTENIMIENTO";
    default: return "DESCONOCIDO";
  }
}

void imprimirEstadoActual() {
  Serial.print("Estado actual: ");
  Serial.println(obtenerNombreEstado(estadoActual));
  
  Serial.print("Temperaturas: ");
  Serial.print(temperaturas[0]); Serial.print("°C, ");
  Serial.print(temperaturas[1]); Serial.print("°C, ");
  Serial.print(temperaturas[2]); Serial.print("°C, ");
  Serial.print(temperaturas[3]); Serial.println("°C");
  
  Serial.print("Niveles: ");
  Serial.print(niveles[0]); Serial.print("%, ");
  Serial.print(niveles[1]); Serial.print("%, ");
  Serial.print(niveles[2]); Serial.println("%");
  
  Serial.print("Presión: ");
  Serial.print(presionActual); Serial.println(" bar");
  
  Serial.print("Válvulas: ");
  Serial.print(digitalRead(VALVULA_1)); Serial.print(", ");
  Serial.print(digitalRead(VALVULA_2)); Serial.println("");
  
  Serial.print("Bombas: ");
  Serial.print(digitalRead(BOMBA_1)); Serial.print(", ");
  Serial.print(digitalRead(BOMBA_2)); Serial.println("");
  
  Serial.println("------------------------");
}
