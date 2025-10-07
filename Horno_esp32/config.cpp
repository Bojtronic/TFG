#include "config.h"
#include <SPI.h>

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

unsigned long TempTanqueTime = 0;
unsigned long TempHornoTime = 0;
unsigned long TempCamaraTime = 0;
unsigned long TempSalidaTime = 0;
unsigned long Bomba1Time = 0;
unsigned long Bomba2Time = 0;
unsigned long Valv1Time = 0;
unsigned long Valv2Time = 0;
unsigned long NivelTime = 0;
unsigned long PresionTime = 0;
unsigned long EstadoTime = 0;


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
NexPage mainPage = NexPage(0, 0, "page0");
NexText temp1Tanque = NexText(0, 32, "t24");  // Temp tanque
NexText temp2Horno = NexText(0, 33, "t25");  // Temp horno
NexText temp3Camara = NexText(0, 34, "t26");  // Temp cámara
NexText temp4Salida = NexText(0, 35, "t27");  // Temp salida
NexText presion = NexText(0, 37, "t29");
NexText nivel = NexText(0, 36, "t28");
NexText estado = NexText(0, 15, "t14");
NexText valvula1Salida = NexText(0, 24, "t20");
NexText valvula2Entrada = NexText(0, 25, "t21");
NexText bomba1 = NexText(0, 26, "t22");
NexText bomba2 = NexText(0, 27, "t23");

NexButton startBtn = NexButton(0, 16, "b0");  // pagina, id, nombre
NexButton stopBtn = NexButton(0, 17, "b1");    // pagina, id, nombre
NexButton manualBtn = NexButton(0, 18, "b2"); // pagina, id, nombre
NexButton valvula1Btn = NexButton(0, 28, "b3");
NexButton valvula2Btn = NexButton(0, 29, "b4");
NexButton bomba1Btn = NexButton(0, 30, "b5");
NexButton bomba2Btn = NexButton(0, 31, "b6");

NexTouch *nex_listen_list[] = {
  &startBtn,
  &stopBtn,
  &manualBtn,
  &valvula1Btn,
  &valvula2Btn,
  &bomba1Btn,
  &bomba2Btn,
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
  pinMode(START_BTN, INPUT);
  pinMode(STOP_BTN, INPUT);
  pinMode(MANUAL_BTN, INPUT); 
}
