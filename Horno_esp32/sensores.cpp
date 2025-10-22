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

  //esta para logica negativa en el que 0 es cerrado y 1 abierto
  if (s1 && s2 && s3) {
    nivelTanque = 0;      // Vacío
  } else if (!s1 && s2 && s3) {
    nivelTanque = 30;     // Bajo
  } else if (!s1 && !s2 && s3) {
    nivelTanque = 60;     // Medio
  } else if (!s1 && !s2 && !s3) {
    nivelTanque = 100;    // Lleno
  } else {
    // Caso inconsistente 
    nivelTanque = 0;
  }
}

void leerPulsadores() {
  // Lógica negativa: LOW cuando está presionado
  bool startButton  = (digitalRead(START_BTN)  == LOW);
  bool stopButton   = (digitalRead(STOP_BTN)   == LOW);
  bool manualButton = (digitalRead(MANUAL_BTN) == LOW);

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