#include "config.h"
#include "sensores.h"


void leerSensores() {
  leerTemperaturas();
  leerPresion();
  leerNiveles();
}

void inicializarTermocuplas() {
  //Serial.println("Inicializando termocuplas...");
  
  bool t1_ok = thermocouple1.begin();
  bool t2_ok = thermocouple2.begin();
  bool t3_ok = thermocouple3.begin();
  bool t4_ok = thermocouple4.begin();

  /*
  // Mensajes para depuración
  if (!t1_ok) Serial.println("ERROR Termocupla 1 (Tanque)"); else Serial.println("Termocupla 1 (Tanque) OK");
  if (!t2_ok) Serial.println("ERROR Termocupla 2 (Horno)"); else Serial.println("Termocupla 2 (Horno) OK");
  if (!t3_ok) Serial.println("ERROR Termocupla 3 (Camara)"); else Serial.println("Termocupla 3 (Camara) OK");
  if (!t4_ok) Serial.println("ERROR Termocupla 4 (Salida)"); else Serial.println("Termocupla 4 (Salida) OK");

  // Construir mensaje resumen
  String resumen = "Termocuplas - " + 
                   String("Tanque:") + String(t1_ok ? "OK" : "ERR") +
                   String(" Horno:") + String(t2_ok ? "OK" : "ERR") +
                   String(" Camara:") + String(t3_ok ? "OK" : "ERR") +
                   String(" Salida:") + String(t4_ok ? "OK" : "ERR");

  Serial.println("Resumen termocuplas -> " + resumen);
  */
}

void verificarErrorTermocupla(int numero, Adafruit_MAX31855 &termocupla) {
  uint8_t error = termocupla.readError();
  
  if (error) {
    Serial.print("Termocupla ");
    Serial.print(numero);
    Serial.print(" - Error: ");
    
    // Verificar usando las constantes correctas
    if (error == MAX31855_FAULT_OPEN) {
      Serial.println("CIRCUITO ABIERTO - Termocupla desconectada");
    } 
    else if (error == MAX31855_FAULT_SHORT_GND) {
      Serial.println("CORTOCIRCUITO A TIERRA");
    }
    else if (error == MAX31855_FAULT_SHORT_VCC) {
      Serial.println("CORTOCIRCUITO A VCC");
    }
    else if (error == MAX31855_FAULT_NONE) {
      // Este caso no debería entrar aquí ya que error != 0
      Serial.println("SIN ERROR (esto no debería aparecer)");
    }
    else {
      Serial.print("ERROR DESCONOCIDO - Código: 0x");
      Serial.println(error, HEX);
    }
  }
}

void diagnosticoCompletoTermocuplas() {
  Serial.println("\n=== DIAGNÓSTICO COMPLETO TERMOCUPLAS ===");
  
  // Verificar pines CS
  Serial.println("Pines CS configurados:");
  Serial.print("CS1 (Tanque): Pin ");
  Serial.println(MAX_CS1);
  Serial.print("CS2 (Horno): Pin ");
  Serial.println(MAX_CS2);
  Serial.print("CS3 (Camara): Pin ");
  Serial.println(MAX_CS3);
  Serial.print("CS4 (Salida): Pin ");
  Serial.println(MAX_CS4);
  
  // Verificar inicialización
  Serial.println("\nEstado de inicialización:");
  bool estados[4] = {
    thermocouple1.begin(),
    thermocouple2.begin(),
    thermocouple3.begin(),
    thermocouple4.begin()
  };
  
  for (int i = 0; i < 4; i++) {
    Serial.print("Termocupla ");
    Serial.print(i+1);
    Serial.print(": ");
    Serial.println(estados[i] ? "INICIALIZADA" : "FALLA INICIALIZACIÓN");
  }
  
  // Leer temperaturas y errores
  Serial.println("\nLecturas actuales:");
  Adafruit_MAX31855* termocuplas[4] = {
    &thermocouple1, &thermocouple2, &thermocouple3, &thermocouple4
  };
  
  for (int i = 0; i < 4; i++) {
    Serial.print("TC");
    Serial.print(i+1);
    Serial.print(": ");
    
    // Intentar lectura
    double temp = termocuplas[i]->readCelsius();
    uint8_t error = termocuplas[i]->readError();
    
    if (isnan(temp)) {
      Serial.print("TEMP=NAN, ");
    } else {
      Serial.print(temp);
      Serial.print("°C, ");
    }
    
    Serial.print("ERROR=0x");
    Serial.print(error, HEX);
    
    if (error == MAX31855_FAULT_NONE) {
      Serial.println(" (Ninguno)");
    } else if (error == MAX31855_FAULT_OPEN) {
      Serial.println(" (Circuito abierto)");
    } else if (error == MAX31855_FAULT_SHORT_GND) {
      Serial.println(" (Corto a tierra)");
    } else if (error == MAX31855_FAULT_SHORT_VCC) {
      Serial.println(" (Corto a VCC)");
    } else {
      Serial.println(" (Desconocido)");
    }
  }
  Serial.println("=====================================\n");
}

bool verificarSensoresTemperatura() {
  // El valor -999.0 indica error en la lectura
  for (int i = 0; i < 4; i++) {
    if (temperaturas[i] <= -999.0) {
      return false;
    }
  }
  return true;
}

void leerTemperaturas() {
  temperaturas[0] = leerTermocupla(thermocouple1, 1); // tanque
  temperaturas[1] = leerTermocupla(thermocouple2, 2); // horno
  temperaturas[2] = leerTermocupla(thermocouple3, 3); // camara
  temperaturas[3] = leerTermocupla(thermocouple4, 4); // salida

  verificarErrorTermocupla(1, thermocouple1);
  verificarErrorTermocupla(2, thermocouple2);
  verificarErrorTermocupla(3, thermocouple3);
  verificarErrorTermocupla(4, thermocouple4);

  //diagnosticoCompletoTermocuplas();
}

double leerTermocupla(Adafruit_MAX31855 &sensor, int numero) {
  double tempC = sensor.readCelsius();

  // Si la lectura no es válida
  if (isnan(tempC)) {

    /*
    Serial.print("Error lectura termocupla ");
    Serial.println(numero);

    // Detalles del error
    uint8_t fault = sensor.readError();
    if (fault & MAX31855_FAULT_OPEN)       Serial.println("FALLA: Termocupla abierta o no conectada.");
    if (fault & MAX31855_FAULT_SHORT_GND)  Serial.println("FALLA: Termocupla en corto a GND.");
    if (fault & MAX31855_FAULT_SHORT_VCC)  Serial.println("FALLA: Termocupla en corto a VCC.");
    */

    return -999.9;  // Valor de error
  }

  // Si no hubo fallos, devolver la temperatura en °C
  return tempC;
}

void leerPresion() {
  int rawPressure = analogRead(PRESSURE_SENSOR);
  
  // Aplicar filtro de promediado
  static int pressure_buffer[5] = {0};
  static int pressure_index = 0;
  
  pressure_buffer[pressure_index] = rawPressure;
  pressure_index = (pressure_index + 1) % 5;
  
  int avg_pressure = 0;
  for (int i = 0; i < 5; i++) {
    avg_pressure += pressure_buffer[i];
  }
  avg_pressure /= 5;
  
  // Convertir lectura analógica a voltaje (ESP32 -> 0-4095 equivale a 0-3.3V)
  float voltage = (avg_pressure / 4095.0f) * 3.3f;

  // Calibración específica de tu sensor (0–3.0 V = 0–10 bar)
  const float PRESSURE_MIN_VOLTAGE = 0.0f;   // Voltaje a 0 bar
  const float PRESSURE_MAX_VOLTAGE = 3.0f;   // Voltaje a 10 bar
  const float PRESSURE_MIN_BAR = 0.0f;
  const float PRESSURE_MAX_BAR = 10.0f;

  if (voltage <= PRESSURE_MIN_VOLTAGE) {
    presionActual = PRESSURE_MIN_BAR;
  } else if (voltage >= PRESSURE_MAX_VOLTAGE) {
    presionActual = PRESSURE_MAX_BAR;
  } else {
    presionActual = PRESSURE_MIN_BAR + 
                   ((voltage - PRESSURE_MIN_VOLTAGE) * 
                   (PRESSURE_MAX_BAR - PRESSURE_MIN_BAR)) / 
                   (PRESSURE_MAX_VOLTAGE - PRESSURE_MIN_VOLTAGE);
  }
  
  // Evitar valores negativos por ruido
  presionActual = max(0.0f, presionActual);
}


void leerNiveles() {
  bool s1 = digitalRead(NIVEL_1);  // contacto para nivel bajo
  bool s2 = digitalRead(NIVEL_2);  // contacto para nivel medio
  bool s3 = digitalRead(NIVEL_3);  // contacto para nivel alto

  // - Vacío: los 3 abiertos -> 0
  // - Solo bajo cerrado -> 30%
  // - bajo y medio cerrados -> 60%
  // - bajo, medio y alto cerrados -> 100%
  // (Si hubiera un caso inconsistente, se considera vacío para inducir al estado de EMERGENCIA)

  if (!s1 && !s2 && !s3) {
    nivelTanque = 0;      // Vacío
  } else if (s1 && !s2 && !s3) {
    nivelTanque = 30;     // Bajo
  } else if (s1 && s2 && !s3) {
    nivelTanque = 60;     // Medio
  } else if (s1 && s2 && s3) {
    nivelTanque = 100;    // Lleno
  } else {
    // Caso inconsistente 
    nivelTanque = 0;
  }
}

void leerPulsadores() {
  // Lógica negativa: LOW cuando está presionado
  bool startButton  = (digitalRead(START_BTN)  == HIGH);
  bool stopButton   = (digitalRead(STOP_BTN)   == HIGH);
  bool manualButton = (digitalRead(MANUAL_BTN) == HIGH);

  // En estado de emergencia no se puede cambiar el estado con los botones
  if(estadoActual != EMERGENCIA){ 
    
    // Flanco de HIGH -> LOW para cada botón
    if (startButton && lastStartState == HIGH && !stopButton && !manualButton) {
      estadoActual = PROCESANDO;
      //Serial.println("📌 Botón START presionado");
    }
    else if (stopButton && lastStopState == HIGH && !startButton && !manualButton) {
      estadoActual = DETENER;
      //Serial.println("📌 Botón STOP presionado");
    }
    else if (manualButton && lastManualState == HIGH && !startButton && !stopButton) {
      estadoActual = MANUAL;
      //Serial.println("📌 Botón MANUAL presionado");
    }
    
    // Actualizar últimas lecturas
    lastStartState  = startButton;
    lastStopState   = stopButton;
    lastManualState = manualButton;
  }
}