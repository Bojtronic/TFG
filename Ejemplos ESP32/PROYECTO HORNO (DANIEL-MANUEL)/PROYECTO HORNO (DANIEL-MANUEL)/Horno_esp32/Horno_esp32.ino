#include "arduino_secrets.h"
/* 
  Sketch generado por el Arduino IoT Cloud para el Thing asociado.
  Variables en la nube (ThingProperties.h) controlan o reportan:
    - float temperatura4;
    - bool indication_fault;
    - bool indication_star;
    - bool start;
    - bool stop;
*/

// ====== Librerías necesarias ======
#include <Nextion.h>             // Comunicación con pantallas HMI Nextion
#include <SPI.h>                 // Comunicación SPI para el sensor de temperatura
#include "Adafruit_MAX31855.h"   // Librería para termopares tipo K
#include <PID_v1.h>              // Librería de control PID
#include "thingProperties.h"     // Configuración de variables de Arduino IoT Cloud

// ====== Definiciones de pines y variables globales ======
#define MAXDO 19
#define MAXCS 5
#define MAXCLK 18
#define PIN_OUTPUT 25
#define pilotoStart 27
#define pilotoFalla 12
#define turbina 21

uint32_t receivedValue = 0;  // Valor numérico recibido desde la HMI
double temperatura = 0;      // Lectura de temperatura actual
double temperatura2 = 0;
double temperatura3 = 0;     // Variable para usar en PID
double Setpoint, Input, Output;
double Kp = 2, Ki = 5, Kd = 1; // Parámetros iniciales del PID

// Buffers para enviar texto a la HMI
char tempHMI[6];
char corrienteHMI[6];

// Variables de control de tiempo
unsigned long previousMillis = 0, currentMillis = 0;
const long tiempoTemp = 2000;       // Intervalo para actualizar temperatura en HMI
unsigned long previousMillis1 = 0, currentMillis1 = 0;
const long tiempoTemp1 = 500;       // Intervalo para actualizar corriente en HMI

// Variables de entradas/sensores
int EMO = 0, estadoEMO = 0, estadolsw = 0, estadotempFalla = 0;
int lsw = 0, tempFalla = 0;
int sensorCorriente = 0;
float corrienteActual = 0;
int val = 0;

// Control de tiempo acumulado
const int pausePin = 36;        
unsigned long startTime = 0, elapsedTime = 0, tiempoAnterior = 0;

// Puerto serial para Nextion en ESP32 (UART2)
HardwareSerial nextionSerial(2);

// ====== Objetos de hardware ======
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);  // Sensor de temperatura
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// ====== Elementos de la HMI Nextion ======
NexPage p0 = NexPage(0, 0, "page0");
NexPage p1 = NexPage(1, 0, "page1");
NexText NexTemperatura = NexText(0, 3, "t0");
NexNumber n0 = NexNumber(0, 1, "n0"); // Temperatura de setpoint desde HMI
NexNumber n1 = NexNumber(0, 12, "n1"); // Horas
NexNumber n2 = NexNumber(0, 13, "n2"); // Minutos
NexScrolltext g0 = NexScrolltext(0, 5, "g0");
NexText t4 = NexText(0, 7, "t4"); // Corriente
NexWaveform s0 = NexWaveform(1, 1, "s0"); // Gráfica de variables

// Lista de elementos que escuchan eventos desde la HMI
NexTouch *nex_listen_list[] = { &n0, NULL };

// ====== Setup ======
void setup() {
  Serial.begin(9600);
  analogReadResolution(12);

  // Inicializa comunicación con Nextion
  nextionSerial.begin(9600, SERIAL_8N1, 16, 17);
  nexInit();

  // Configuración de IoT Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Configuración del PID
  Input = temperatura3;
  myPID.SetMode(AUTOMATIC);

  // Mensajes de inicialización
  mensajes("MAX31855 test");
  delay(5000);
  mensajes("Inicializando sensor...");
  delay(5000);

  // Validar conexión del sensor de temperatura
  if (!thermocouple.begin()) {
    mensajes("ERROR! Modulo Temperatura");
    while (1) delay(10); // Bloquea si no hay sensor
  }
  mensajes("DONE! Modulo Temperatura");
  delay(5000);
  mensajes("Sin ERRORES");

  // Asocia evento de lectura de temperatura desde HMI
  n0.attachPop(setTemperatura, &n0);

  // Configuración de pines de salida y entrada
  pinMode(pilotoStart, OUTPUT);
  pinMode(pilotoFalla, OUTPUT);
  pinMode(turbina, OUTPUT);
  pinMode(13, INPUT); // Start
  pinMode(14, INPUT); // EMO
  pinMode(37, INPUT); // LSW
  pinMode(38, INPUT); // tempFalla
  digitalWrite(pilotoFalla, HIGH);
  pinMode(pausePin, INPUT);
}

// ====== Loop principal ======
void loop() {
  ArduinoCloud.update(); // Actualiza conexión IoT Cloud
  actualTemperatura();   // Lee y muestra temperatura
  Corriente();           // Lee y muestra corriente
  acciones();            // Procesa acciones HMI
  erroresTemperatura();  // Revisa fallos del sensor
  lecturaEntradas();     // Lee botones y sensores
  graficas();            // Envía datos al gráfico
  onIndicationStarChange(); // Lógica de arranque
}

// ====== Funciones auxiliares ======

// Muestra mensajes en el scroll text de la HMI
void mensajes(String mensaje) {
  char mensajesText[30];
  mensaje.toCharArray(mensajesText, 30);
  g0.setText(mensajesText);
  // ⚠ Si el mensaje es muy largo o se envía muy rápido, podría provocar recvRetCommandFinished err
}

// Procesa los eventos recibidos desde la HMI
void acciones() {
  nexLoop(nex_listen_list);
  // ⚠ Si se llama sin dar tiempo al Nextion para responder, puede dar error
}

// Verifica errores de lectura del termopar
void erroresTemperatura() {
  if (isnan(temperatura)) {
    uint8_t e = thermocouple.readError();
    if (e & MAX31855_FAULT_OPEN) Serial.println("FALLA: Termocupla abierta.");
    if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FALLA: Corto a GND.");
    if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FALLA: Corto a VCC.");
  }
}

// Lee y actualiza temperatura
void actualTemperatura() {
  currentMillis = millis();
  temperatura = thermocouple.readCelsius();
  if (currentMillis - previousMillis >= tiempoTemp) {
    previousMillis = currentMillis;
    String TempString = String(temperatura);
    temperatura4 = temperatura;
    TempString.toCharArray(tempHMI, 6);
    NexTemperatura.setText(tempHMI);
  }
}

// Calcula y aplica PID
void PID() {
  Setpoint = 100;
  temperatura2 = int(Setpoint);
  temperatura3 = temperatura2;
  Input = temperatura;
  myPID.Compute();
  analogWrite(PIN_OUTPUT, Output);
}

// Evento cuando se cambia temperatura desde la HMI
void setTemperatura(void *ptr) {
  delay(6000); // ⚠ Este delay tan largo puede causar recvRetCommandFinished err
  if (n0.getValue(&receivedValue)) {
    Setpoint = 100; // ⚠ Está fijo, no usa el valor recibido
    mensajes("Temperatura Seteada");
  }
}

// Lee entradas digitales
void lecturaEntradas() {
  start = digitalRead(13);
  EMO = digitalRead(14);
  lsw = digitalRead(37);
  tempFalla = digitalRead(38);
}

// Lógica de arranque y fallos
void onIndicationStarChange() {
  estadotempFalla = 0;
  indication_fault = false;
  mensajes("");

  if (EMO == LOW) { estadotempFalla = 3; onIndicationFaultChange(); return; }
  if (lsw == LOW) { estadotempFalla = 2; onIndicationFaultChange(); return; }
  if (tempFalla == LOW) { estadotempFalla = 1; onIndicationFaultChange(); return; }

  if (start == LOW && estadoEMO == 0 && estadolsw == 0 && estadotempFalla == 0) {
    digitalWrite(pilotoStart, LOW);
    PID();
    tiempo();
    digitalWrite(turbina, LOW);
    indication_star = true;
  }
}

// Manejo de fallos y mensajes en HMI
void onIndicationFaultChange() {
  indication_fault = true;
  switch (estadotempFalla) {
    case 1: mensajes("Temperatura Máxima."); break;
    case 2: mensajes("Puerta abierta, CERRAR"); break;
    case 3: mensajes("EMO presionado, REINICIAR SISTEMA"); break;
    default: mensajes(""); return;
  }
  digitalWrite(turbina, HIGH);
  digitalWrite(pilotoStart, HIGH);
  digitalWrite(pilotoFalla, LOW);
  delay(500);
  digitalWrite(pilotoFalla, HIGH);
  delay(500);
}

// Lectura y cálculo de corriente
void Corriente() {
  previousMillis1 = millis();
  val = 0;
  while (millis() - previousMillis1 < 100) {
    sensorCorriente = analogRead(26);
    if (sensorCorriente > val) { val = sensorCorriente; }
  }
  if (val != 0) { val += 800; }
  corrienteActual = val / (4095 / 8.3333);
  String corrienteActual1 = String(corrienteActual);
  corrienteActual1.toCharArray(corrienteHMI, 6);
  t4.setText(corrienteHMI);
}

// Envía datos al gráfico de la HMI
void graficas() {
  s0.addValue(0, temperatura);
  s0.addValue(1, corrienteActual);
}

// Control de tiempo de funcionamiento
void tiempo() {
  if (digitalRead(pausePin) == LOW) {
    delay(50);
    if (digitalRead(pausePin) == LOW) { startTime = elapsedTime; }
    while (digitalRead(pausePin) == LOW) { delay(10); }
    elapsedTime = startTime;
  }
  if (millis() - tiempoAnterior > 1000) {
    elapsedTime++;
    tiempoAnterior = millis();
    unsigned long totalSeconds = elapsedTime;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    n1.setValue(hours);
    n2.setValue(minutes);
  }
}

// Acciones de parada
void onStopChange() {
  digitalWrite(pilotoStart, HIGH);
  digitalWrite(turbina, HIGH);
  analogWrite(PIN_OUTPUT, 0);
  indication_star = false;
}
