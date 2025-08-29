#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <Nextion.h>

// ---------------------------
// Definición de pines
// ---------------------------
#define MAXDO   19
#define MAXCLK  18
#define MAXCS1  5
#define MAXCS2  21
#define MAXCS3  22
#define MAXCS4  23

#define NIVEL1    32
#define NIVEL2    33
#define NIVEL3    34
#define BTN_START 25
#define BTN_STOP  26
#define BTN_EMO   27

#define VALV1  12
#define VALV2  13
#define BOMBA1 14
#define BOMBA2 15

// ---------------------------
// Comunicación con Nextion
// ---------------------------
HardwareSerial nextionSerial(2); // UART2 RX=16, TX=17
NexText tTemp1(0, 1, "t0");
NexText tTemp2(0, 2, "t1");
NexText tTemp3(0, 3, "t2");
NexText tTemp4(0, 4, "t3");
NexText tEstado(0, 5, "t4");

NexTouch *nex_listen_list[] = { NULL };

// ---------------------------
// Objetos de sensores
// ---------------------------
Adafruit_MAX31855 term1(MAXCLK, MAXCS1, MAXDO);
Adafruit_MAX31855 term2(MAXCLK, MAXCS2, MAXDO);
Adafruit_MAX31855 term3(MAXCLK, MAXCS3, MAXDO);
Adafruit_MAX31855 term4(MAXCLK, MAXCS4, MAXDO);

// ---------------------------
// Variables globales
// ---------------------------
double temp1, temp2, temp3, temp4;
bool estadoValv1 = false, estadoValv2 = false;
bool estadoBomba1 = false, estadoBomba2 = false;

// Máquina de estados
enum Estado { INICIO, ESPERA, PROCESO, EMERGENCIA };
Estado estadoActual = INICIO;

// Temporizadores con millis()
unsigned long lastUpdateTemp = 0;
unsigned long lastUpdateEstado = 0;
const unsigned long INTERVALO_TEMP = 1000;   // 1s
const unsigned long INTERVALO_ESTADO = 500;  // 0.5s

// ---------------------------
// Funciones
// ---------------------------
void enviarNextion(NexText &obj, const String &msg) {
  char buffer[40];
  msg.toCharArray(buffer, sizeof(buffer));
  obj.setText(buffer);
}

void leerTermocuplas() {
  temp1 = term1.readCelsius();
  temp2 = term2.readCelsius();
  temp3 = term3.readCelsius();
  temp4 = term4.readCelsius();

  if (isnan(temp1)) temp1 = -100;
  if (isnan(temp2)) temp2 = -100;
  if (isnan(temp3)) temp3 = -100;
  if (isnan(temp4)) temp4 = -100;
}

void mostrarTemperaturasHMI() {
  char buffer[10];
  dtostrf(temp1, 4, 1, buffer); tTemp1.setText(buffer);
  dtostrf(temp2, 4, 1, buffer); tTemp2.setText(buffer);
  dtostrf(temp3, 4, 1, buffer); tTemp3.setText(buffer);
  dtostrf(temp4, 4, 1, buffer); tTemp4.setText(buffer);
}

void mostrarEstadosHMI() {
  String estado = "V1:" + String(estadoValv1) +
                  " V2:" + String(estadoValv2) +
                  " B1:" + String(estadoBomba1) +
                  " B2:" + String(estadoBomba2);
  enviarNextion(tEstado, estado);
}

void controlarSalidas() {
  digitalWrite(VALV1, estadoValv1);
  digitalWrite(VALV2, estadoValv2);
  digitalWrite(BOMBA1, estadoBomba1);
  digitalWrite(BOMBA2, estadoBomba2);
}

void actualizarEstado() {
  int start = digitalRead(BTN_START);
  int stop  = digitalRead(BTN_STOP);
  int emo   = digitalRead(BTN_EMO);

  switch (estadoActual) {
    case INICIO:
      estadoValv1 = estadoValv2 = estadoBomba1 = estadoBomba2 = false;
      estadoActual = ESPERA;
      break;

    case ESPERA:
      if (emo == LOW) estadoActual = EMERGENCIA;
      else if (start == LOW) estadoActual = PROCESO;
      break;

    case PROCESO:
      estadoValv1 = estadoBomba1 = true;
      if (stop == LOW) estadoActual = ESPERA;
      if (emo == LOW) estadoActual = EMERGENCIA;

      // Sensores de nivel
      if (digitalRead(NIVEL1) == LOW) estadoBomba1 = false;
      if (digitalRead(NIVEL2) == LOW) estadoBomba2 = false;
      break;

    case EMERGENCIA:
      estadoValv1 = estadoValv2 = estadoBomba1 = estadoBomba2 = false;
      if (emo == HIGH) estadoActual = ESPERA; // salir de emergencia
      break;
  }
}

// ---------------------------
// Setup
// ---------------------------
void setup() {
  Serial.begin(115200);
  nextionSerial.begin(9600, SERIAL_8N1, 16, 17);
  nexInit();

  pinMode(NIVEL1, INPUT_PULLUP);
  pinMode(NIVEL2, INPUT_PULLUP);
  pinMode(NIVEL3, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  pinMode(BTN_EMO, INPUT_PULLUP);

  pinMode(VALV1, OUTPUT);
  pinMode(VALV2, OUTPUT);
  pinMode(BOMBA1, OUTPUT);
  pinMode(BOMBA2, OUTPUT);

  Serial.println("Sistema iniciado");
}

// ---------------------------
// Loop principal
// ---------------------------
void loop() {
  nexLoop(nex_listen_list);

  unsigned long ahora = millis();

  actualizarEstado();
  controlarSalidas();

  if (ahora - lastUpdateTemp >= INTERVALO_TEMP) {
    lastUpdateTemp = ahora;
    leerTermocuplas();
    mostrarTemperaturasHMI();
  }

  if (ahora - lastUpdateEstado >= INTERVALO_ESTADO) {
    lastUpdateEstado = ahora;
    mostrarEstadosHMI();
  }
}
