#include "config.h"

// ================= CONFIGURACIÓN SERVIDOR =================
const char* ssid = "Familia G.V";
const char* password = "VerseK3r98";
const char* serverURL = "https://horno-tecnelectro.onrender.com/api/message";
const char* commandsURL = "https://horno-tecnelectro.onrender.com/api/esp32-commands";

// ================= DEFINICIÓN DE VARIABLES GLOBALES =================
// Termocuplas
Adafruit_MAX31855 thermocouple1(MAX_CLK, MAX_CS1, MAX_DO1);
Adafruit_MAX31855 thermocouple2(MAX_CLK, MAX_CS2, MAX_DO2);
Adafruit_MAX31855 thermocouple3(MAX_CLK, MAX_CS3, MAX_DO3);
Adafruit_MAX31855 thermocouple4(MAX_CLK, MAX_CS4, MAX_DO4);

// Variables de sensores
double temperaturas[4] = { 0, 0, 0, 0 };
int niveles[3] = { 0, 0, 0 };
float presionActual = 0.0;

// Estados del sistema
EstadoSistema estadoActual = SISTEMA_APAGADO;
bool emergencia = false;
bool bombaPrincipalActiva = true;
unsigned long ultimoCambioBomba = 0;
unsigned long lastReadTime = 0;

// Variables comunicación
unsigned long lastSendTime = 0;
unsigned long lastCommandCheck = 0;
int messageCount = 0;

// ================= CONFIGURACIÓN HMI NEXTION =================
HardwareSerial nextionSerial(2);

// Componentes Nextion
NexPage mainPage(0, 0, "mainPage");
NexText temp1Text(0, 1, "temp1");  // Temp tanque
NexText temp2Text(0, 2, "temp2");  // Temp horno
NexText temp3Text(0, 3, "temp3");  // Temp cámara
NexText temp4Text(0, 4, "temp4");  // Temp salida
NexText nivel1Text(0, 5, "nivel1");
NexText nivel2Text(0, 6, "nivel2");
NexText nivel3Text(0, 7, "nivel3");
NexText presionText(0, 8, "presion");
NexText estadoText(0, 9, "estado");
NexPicture valvula1State(0, 10, "valv1State");
NexPicture valvula2State(0, 11, "valv2State");
NexPicture bomba1State(0, 12, "bomba1State");
NexPicture bomba2State(0, 13, "bomba2State");
NexButton startBtn(0, 14, "startBtn");  // pagina, id, nombre
NexButton stopBtn(0, 15, "stopBtn");    // pagina, id, nombre
NexButton resetBtn(0, 16, "resetBtn");  // pagina, id, nombre

NexTouch* nex_listen_list[] = {
  &startBtn,
  &stopBtn,
  &resetBtn,
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

  //Serial.println("✅ Pines configurados correctamente");
}
