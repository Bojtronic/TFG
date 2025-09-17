#include "sensores.h"
#include "config.h"
#include "hmi.h"
#include <Adafruit_MAX31855.h>

// Variables globales definidas en el main
extern Adafruit_MAX31855 thermocouple1;
extern Adafruit_MAX31855 thermocouple2;
extern Adafruit_MAX31855 thermocouple3;
extern Adafruit_MAX31855 thermocouple4;
extern double temperaturas[4];
extern int niveles[3];
extern float presionActual;

// ================= FUNCIONES DE LECTURA DE SENSORES =================

void leerSensores() {
  leerTemperaturas();
  leerPresion();
  //leerNiveles();
}

void leerTemperaturas() {
  temperaturas[0] = leerTermocupla(thermocouple1, 1); // tanque
  temperaturas[1] = leerTermocupla(thermocouple2, 2); // horno
  temperaturas[2] = leerTermocupla(thermocouple3, 3); // camara
  temperaturas[3] = leerTermocupla(thermocouple4, 4); // salida

  Serial.println("Temperaturas (°C): ");
  Serial.println(temperaturas[0], 1);
  Serial.println(temperaturas[1], 1);
  Serial.println(temperaturas[2], 1);
  Serial.println(temperaturas[3], 1);
  Serial.println("--------------------------------------");
}

double leerTermocupla(Adafruit_MAX31855 &sensor, int numero) {
  double tempC = sensor.readCelsius();

  // Si la lectura no es válida
  if (isnan(tempC)) {
    Serial.print("Error lectura termocupla ");
    Serial.println(numero);
    // mostrar mensaje en HMI y enviar informacion al server

    // Revisar detalles del error
    uint8_t fault = sensor.readError();
    if (fault & MAX31855_FAULT_OPEN)       Serial.println("FALLA: Termocupla abierta o no conectada.");
    if (fault & MAX31855_FAULT_SHORT_GND)  Serial.println("FALLA: Termocupla en corto a GND.");
    if (fault & MAX31855_FAULT_SHORT_VCC)  Serial.println("FALLA: Termocupla en corto a VCC.");

    return -999.9;  // Valor de error
  }

  // Si no hubo fallos, devolver la temperatura en °C
  return tempC;
}


void leerNiveles() {
  // Leer valores crudos y convertir a porcentaje
  int raw1 = analogRead(NIVEL_1);
  int raw2 = analogRead(NIVEL_2);
  int raw3 = analogRead(NIVEL_3);
  
  // Aplicar filtro de promediado para reducir ruido
  static int raw1_buffer[5] = {0}, raw2_buffer[5] = {0}, raw3_buffer[5] = {0};
  static int index = 0;
  
  raw1_buffer[index] = raw1;
  raw2_buffer[index] = raw2;
  raw3_buffer[index] = raw3;
  
  index = (index + 1) % 5;
  
  int avg1 = 0, avg2 = 0, avg3 = 0;
  for (int i = 0; i < 5; i++) {
    avg1 += raw1_buffer[i];
    avg2 += raw2_buffer[i];
    avg3 += raw3_buffer[i];
  }
  avg1 /= 5;
  avg2 /= 5;
  avg3 /= 5;
  
  // Convertir a porcentaje (ajustar según calibración real)
  niveles[0] = map(avg1, 0, 4095, 0, 100); // Vacío
  niveles[1] = map(avg2, 0, 4095, 0, 100); // Mitad
  niveles[2] = map(avg3, 0, 4095, 0, 100); // Lleno
  
  // Asegurar que los valores estén dentro del rango 0-100%
  niveles[0] = constrain(niveles[0], 0, 100);
  niveles[1] = constrain(niveles[1], 0, 100);
  niveles[2] = constrain(niveles[2], 0, 100);
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

  Serial.println("********************************************************");
  Serial.print("🔧 Raw ADC: "); Serial.print(avg_pressure);
  Serial.print(" | Voltaje: "); Serial.print(voltage, 3);
  Serial.print(" V | Presión: "); Serial.print(presionActual, 2);
  Serial.println(" bar");
  Serial.println("********************************************************");
}


void inicializarTermocuplas() {
  Serial.println("Inicializando termocuplas...");
  
  // Variables booleanas para guardar estado de cada termocupla
  bool t1_ok = thermocouple1.begin();
  bool t2_ok = thermocouple2.begin();
  bool t3_ok = thermocouple3.begin();
  bool t4_ok = thermocouple4.begin();

  // Mensajes en Serial (para depuración detallada)
  if (!t1_ok) Serial.println("ERROR Termocupla 1 (Tanque)"); else Serial.println("Termocupla 1 (Tanque) OK");
  if (!t2_ok) Serial.println("ERROR Termocupla 2 (Horno)"); else Serial.println("Termocupla 2 (Horno) OK");
  if (!t3_ok) Serial.println("ERROR Termocupla 3 (Camara)"); else Serial.println("Termocupla 3 (Camara) OK");
  if (!t4_ok) Serial.println("ERROR Termocupla 4 (Salida)"); else Serial.println("Termocupla 4 (Salida) OK");

  // Construir mensaje resumen para el HMI - CORREGIDO
  String resumen = "Termocuplas - " + 
                   String("Tanque:") + String(t1_ok ? "OK" : "ERR") +
                   String(" Horno:") + String(t2_ok ? "OK" : "ERR") +
                   String(" Camara:") + String(t3_ok ? "OK" : "ERR") +
                   String(" Salida:") + String(t4_ok ? "OK" : "ERR");

  // Mostrar resumen en Serial y en la HMI
  Serial.println("Resumen termocuplas -> " + resumen);
  mensajesHMI(resumen);
}

bool verificarSensoresTemperatura() {
  // Verificar que todas las termocuplas estén funcionando
  for (int i = 0; i < 4; i++) {
    if (temperaturas[i] <= -999.0) {
      return false;
    }
  }
  return true;
}


void leerPulsadores(){
  Serial.println("Leer pulsadores");
  // se debe implementar la funcionalidad para botones fisicos
}
