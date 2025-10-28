#include "config.h"
#include <SPI.h>

// ================= CONFIGURACIÓN SERVIDOR =================
const char* ssid = "DIGICONTROL";
const char* password = "7012digi19";
const char* serverURL = "https://tfg-server-ecoview.onrender.com/api/message";
const char* commandsURL = "https://tfg-server-ecoview.onrender.com/api/esp32-commands";

// ================= WHATSAPP =================
String numeroCelular = "+506XXXXXXXX";
String apiKey = "REPLAZAR_CON_API_KEY";

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

// ===== TIMERS GLOBALES =====
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

// ================= VARIABLES PARA ALMACENAR EL ULTIMO VALOR MOSTRADO =================
char lastEstado[10] = "";
float lastTanqueTemp = -2000;
float lastHornoTemp = -2000;
float lastCamaraTemp = -2000;
float lastSalidaTemp = -2000;
int lastNivel = -1;
float lastPresion = -1;
bool lastBomba1State = false;
bool lastBomba2State = false;
bool lastValv1State = false;
bool lastValv2State = false;



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

// Mensajes que se envían por WhatsApp
const char* MENSAJES[] = {
    "SISTEMA APAGADO",                                                        // 0
    "Horno y camara calientes, Nivel del tanque mas de la mitad",            // 1
    "Hay presion de agua, Horno y camara calientes, Nivel del tanque está entre vacio y mitad", // 2
    "Horno y camara calientes, Nivel del tanque menor a la mitad",           // 3
    "Horno y camara calientes, Nivel del tanque mayor a la mitad",           // 4
    "Horno y camara calientes, Nivel del tanque lleno",                      // 5
    "Horno y camara frios, Nivel del tanque menor a la mitad",               // 6
    "Horno y camara frios, Nivel del tanque mayor a la mitad",               // 7
    "GRAVE: No hay presion de agua, Horno y camara DEMASIADO calientes, Tanque vacio", // 8
    "No hay presion de agua, Nivel de tanque vacio, Horno caliente",         // 9
    "No hay presion de agua, El tanque tiene algo de agua, Horno caliente",  // 10
    "Tanque de agua esta muy caliente y no esta lleno, Horno y camara calientes", // 11
    "Tanque de agua esta muy caliente y no esta lleno, Horno y camara frios", // 12
    "Horno caliente, El tanque no esta lleno",                               // 13
    "Horno caliente, El tanque esta lleno",                                  // 14
    "Camara de humo caliente, El tanque no esta lleno",                      // 15
    "Modo MANUAL activado",                                                  // 16
    "DESCONOCIDO"                                                            // 17
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
