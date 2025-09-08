#include "sensores.h"
#include "config.h"
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
  leerNiveles();
  leerPresion();
}

void leerTemperaturas() {
  temperaturas[0] = leerTermocupla(thermocouple1, 1);
  temperaturas[1] = leerTermocupla(thermocouple2, 2);
  temperaturas[2] = leerTermocupla(thermocouple3, 3);
  temperaturas[3] = leerTermocupla(thermocouple4, 4);
}

double leerTermocupla(Adafruit_MAX31855 &sensor, int numero) {
  double tempC = sensor.readCelsius();

  // Si la lectura no es válida
  if (isnan(tempC)) {
    Serial.print("Error lectura termocupla ");
    Serial.println(numero);

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
  
  // Convertir lectura analógica a presión en bar (ajustar según sensor y calibración)
  // Ejemplo: sensor 0-5V = 0-10 bar -> 0-4095 = 0-10 bar
  float voltage = (avg_pressure / 4095.0) * 3.3; // ESP32 ADC reference voltage is 3.3V
  
  // Calibración específica del sensor (ajustar estos valores según las especificaciones del sensor)
  const float PRESSURE_MIN_VOLTAGE = 0.5;    // Voltage at 0 bar
  const float PRESSURE_MAX_VOLTAGE = 4.5;    // Voltage at 10 bar
  const float PRESSURE_MIN_BAR = 0.0;
  const float PRESSURE_MAX_BAR = 10.0;
  
  // Aplicar calibración lineal
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
  
  // Asegurar que la presión no sea negativas
  presionActual = max(0.0f, presionActual);
}

void inicializarTermocuplas() {
  Serial.println("Inicializando termocuplas...");
  
  if (!thermocouple1.begin()) {
    Serial.println("ERROR Termocupla 1 (Tanque)");
  } else {
    Serial.println("Termocupla 1 OK");
  }
  
  if (!thermocouple2.begin()) {
    Serial.println("ERROR Termocupla 2 (Horno)");
  } else {
    Serial.println("Termocupla 2 OK");
  }
  
  if (!thermocouple3.begin()) {
    Serial.println("ERROR Termocupla 3 (Camara)");
  } else {
    Serial.println("Termocupla 3 OK");
  }
  
  if (!thermocouple4.begin()) {
    Serial.println("ERROR Termocupla 4 (Salida)");
  } else {
    Serial.println("Termocupla 4 OK");
  }
  
  Serial.println("Termocuplas inicializadas");
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

void calibrarSensores() {
  // Rutina de calibración para sensores (ejecutar en modo mantenimiento)
  Serial.println("Iniciando calibración de sensores...");
  
  // Calibración de sensores de nivel (leer valores en vacío y lleno)
  Serial.println("Calibrando sensores de nivel...");
  Serial.println("Vaciar tanque y presionar cualquier tecla para continuar");
  while (!Serial.available()) delay(100);
  Serial.read(); // Limpiar buffer
  
  int vacio1 = analogRead(NIVEL_1);
  int vacio2 = analogRead(NIVEL_2);
  int vacio3 = analogRead(NIVEL_3);
  
  Serial.println("Llenar tanque y presionar cualquier tecla para continuar");
  while (!Serial.available()) delay(100);
  Serial.read(); // Limpiar buffer
  
  int lleno1 = analogRead(NIVEL_1);
  int lleno2 = analogRead(NIVEL_2);
  int lleno3 = analogRead(NIVEL_3);
  
  Serial.println("Valores de calibración:");
  Serial.print("Vacío: "); Serial.print(vacio1); Serial.print(", "); Serial.print(vacio2); Serial.print(", "); Serial.println(vacio3);
  Serial.print("Lleno: "); Serial.print(lleno1); Serial.print(", "); Serial.print(lleno2); Serial.print(", "); Serial.println(lleno3);
  
  Serial.println("Calibración completada");
}

void leerPulsadores(){
  Serial.println("Leer pulsadores");
}
