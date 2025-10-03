#include "config.h"

// ================= CONFIGURACIÓN SERVIDOR =================
const char* ssid = "DIGICONTROL";
const char* password = "7012digi19";
const char* serverURL = "https://tfg-server-ecoview.onrender.com/api/message";
const char* commandsURL = "https://tfg-server-ecoview.onrender.com/api/esp32-commands";

// ================= DEFINICIÓN DE VARIABLES GLOBALES =================
// Termocuplas
Adafruit_MAX31855 thermocouple1(MAX_CLK, MAX_CS1, MAX_MISO);
Adafruit_MAX31855 thermocouple2(MAX_CLK, MAX_CS2, MAX_MISO);
Adafruit_MAX31855 thermocouple3(MAX_CLK, MAX_CS3, MAX_MISO);
Adafruit_MAX31855 thermocouple4(MAX_CLK, MAX_CS4, MAX_MISO);

// Variables de sensores
double temperaturas[4] = { 0, 0, 0, 0 };
int nivelTanque = 0;
int niveles[3] = { 0, 0, 0 };
float presionActual = 0.0;

// Variables para el control automático
bool valvula_1_auto; // Estado automático de la válvula 1
bool valvula_2_auto; // Estado automático de la válvula 2
bool bomba_1_auto;   // Estado automático de la bomba 1
bool bomba_2_auto;   // Estado automático de la bomba 2

// Estados del sistema
EstadoSistema estadoActual = APAGADO;
MensajeSistema mensajeActual = APAGADO_0;
bool emergencia = false;
bool bombaPrincipalActiva = true;
unsigned long ultimoCambioBomba = 0;
unsigned long lastReadTime = 0;

// Variables para manejo de pulsadores
bool startPressed;
bool stopPressed;
bool manualPressed;

// Guardar el último estado de cada botón
bool lastStartState = HIGH;
bool lastStopState = HIGH;
bool lastManualState = HIGH;

// Variables comunicación
unsigned long lastSendTime = 0;
unsigned long lastCommandCheck = 0;
int messageCount = 0;

// ================= CONFIGURACIÓN HMI NEXTION =================
HardwareSerial nextionSerial(2);

// Componentes Nextion
NexPage mainPage(0, 0, "page0");
NexNumber temp1Tanque(0, 19, "x0");  // Temp tanque
NexNumber temp2Horno(0, 20, "x1");  // Temp horno
NexNumber temp3Camara(0, 21, "x2");  // Temp cámara
NexNumber temp4Salida(0, 22, "x3");  // Temp salida
NexNumber presion(0, 23, "x4");
NexNumber nivel(0, 24, "n0");
NexText estado(0, 15, "t14");
NexText valvula1Salida(0, 10, "valv1State");
NexText valvula2Entrada(0, 11, "valv2State");
NexText bomba1(0, 12, "bomba1State");
NexText bomba2(0, 13, "bomba2State");
NexButton startBtn(0, 14, "startBtn");  // pagina, id, nombre
NexButton stopBtn(0, 15, "stopBtn");    // pagina, id, nombre
NexButton manualBtn(0, 16, "manualBtn"); // pagina, id, nombre

NexTouch* nex_listen_list[] = {
  &startBtn,
  &stopBtn,
  &manualBtn,
  NULL
};

// ================= FUNCIÓN DE CONFIGURACIÓN DE PINES =================
void configurarPines() {
  // Configurar entradas
  pinMode(NIVEL_1, INPUT);
  pinMode(NIVEL_2, INPUT);
  pinMode(NIVEL_3, INPUT);
  pinMode(PRESSURE_SENSOR, INPUT);
  pinMode(START_BTN, INPUT);
  pinMode(STOP_BTN, INPUT);

  // Configurar salidas
  pinMode(VALVULA_1, OUTPUT);
  pinMode(VALVULA_2, OUTPUT);
  pinMode(BOMBA_1, OUTPUT);
  pinMode(BOMBA_2, OUTPUT);

  // Estado inicial seguro
  digitalWrite(VALVULA_1, LOW);
  digitalWrite(VALVULA_2, LOW);
  digitalWrite(BOMBA_1, LOW);
  digitalWrite(BOMBA_2, LOW);

  // Configurar botones 
  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(STOP_BTN, INPUT);
  pinMode(MANUAL_BTN, INPUT); 
}
