#ifndef CONFIG_H
#define CONFIG_H

#include <Adafruit_MAX31855.h> 
#include <Nextion.h>  

// ================= CONFIGURACIÓN DE PINES =================
// Termocuplas (MAX31855) - 4 sensores de temperatura

// SPI para MAX31855 (VSPI por defecto en ESP32)
#define MAX_CLK   18   // SCK (VSPI)
#define MAX_MISO  19   // MISO (compartido entre los 4 MAX31855)

// CS individuales para cada MAX31855
#define MAX_CS1   22
#define MAX_CS2   23
#define MAX_CS3   5
#define MAX_CS4   21

// Sensores analógicos 

// Sensores de nivel de agua
#define NIVEL_1   36   // ADC1_0 - Vacío
#define NIVEL_2   39   // ADC1_3 - Mitad
#define NIVEL_3   34   // ADC1_6 - Lleno

// Sensor de presión
#define PRESSURE_SENSOR 33 // ADC1_5   

// Electrovalvulas (salidas digitales)
#define VALVULA_1 25  // Salida agua caliente
#define VALVULA_2 4   // Entrada agua fría

// Bombas de agua (salidas digitales)
#define BOMBA_1   27  // Circulación principal
#define BOMBA_2   12  // Circulación redundante 

// Pulsadores de control
#define START_BTN   14
#define STOP_BTN    13 
#define MANUAL_BTN  35   

// Comunicación con HMI Nextion (UART2)
#define NEXTION_RX 16
#define NEXTION_TX 17


// ================= PARÁMETROS DE CONFIGURACIÓN =================
#define TEMP_AGUA_CALIENTE 60.0     // Temperatura objetivo de agua caliente para el usuario (°C) temperaturas[3]
#define TEMP_MAX_TANQUE    70.0     // Temperatura máxima en tanque (°C)
#define TEMP_MAX_HORNO     200.0    // Temperatura máxima en horno (°C)
#define TEMP_MAX_CAMARA    150.0    // Temperatura máxima en cámara (°C)
#define TEMP_MIN_HORNO     35.0     // Temperatura minima en horno (°C)
#define NIVEL_LLENO        80       // % para considerar tanque lleno
#define NIVEL_MITAD        40       // % para considerar tanque a la mitad
#define NIVEL_VACIO        10       // % para considerar tanque vacío
#define PRESION_MINIMA     1.0      // Presión para alerta (bar)

#define INTERVALO_CAMBIO_BOMBA 1200000 // 20 minutos 1200000 milisegundos
#define LECTURA_INTERVAL 1000

#define ESCRITURA_BOMBA1_INTERVAL 500 
#define ESCRITURA_BOMBA2_INTERVAL 700 
#define ESCRITURA_VALV1_INTERVAL 900 
#define ESCRITURA_VALV2_INTERVAL 1100 
#define ESCRITURA_ESTADO_INTERVAL 2000 
#define ESCRITURA_TEMPTANQUE_INTERVAL 3300  
#define ESCRITURA_TEMPHORNO_INTERVAL 3600   
#define ESCRITURA_TEMPCAMARA_INTERVAL 3900 
#define ESCRITURA_TEMPSALIDA_INTERVAL 4100 
#define ESCRITURA_NIVEL_INTERVAL 4400 
#define ESCRITURA_PRESION_INTERVAL 5000 


// ================= CONFIGURACIÓN SERVIDOR =================
extern const char* ssid;
extern const char* password;
extern const char* serverURL;
extern const char* commandsURL;

// ================= DECLARACIONES GLOBALES =================
// Termocuplas
extern Adafruit_MAX31855 thermocouple1; // tanque
extern Adafruit_MAX31855 thermocouple2; // horno
extern Adafruit_MAX31855 thermocouple3; // camara
extern Adafruit_MAX31855 thermocouple4; // salida

// Variables de sensores
extern double temperaturas[4];
extern int nivelTanque;
extern float presionActual;

// Variables para el control automático
extern bool valvula_1_auto; // Estado automático de la válvula 1
extern bool valvula_2_auto; // Estado automático de la válvula 2
extern bool bomba_1_auto;   // Estado automático de la bomba 1
extern bool bomba_2_auto;   // Estado automático de la bomba 2

// Variables para manejo de pulsadores
extern bool startPressed;
extern bool stopPressed;
extern bool manualPressed;

// Guardar el último estado de cada botón
extern bool lastStartState;
extern bool lastStopState;
extern bool lastManualState;


// Estados del sistema
enum EstadoSistema {
  APAGADO,
  DETENER,
  PROCESANDO,
  EMERGENCIA,
  MANUAL
};

// Mensajes del sistema como numeros para enviar a la aplicación web
enum MensajeSistema {
  APAGADO_0,
  DETENER_1,
  DETENER_2,
  PROCESANDO_1,
  PROCESANDO_2,
  PROCESANDO_3,
  PROCESANDO_4,
  PROCESANDO_5,
  EMERGENCIA_1,
  EMERGENCIA_2,
  EMERGENCIA_3,
  EMERGENCIA_4,
  EMERGENCIA_5,
  EMERGENCIA_6,
  EMERGENCIA_7,
  EMERGENCIA_8,
  MANUAL_0,
  DESCONOCIDO
};

extern EstadoSistema estadoActual;
extern MensajeSistema mensajeActual;
extern bool emergencia;
extern bool bombaPrincipalActiva;

// ===== TIMERS GLOBALES =====
extern unsigned long ultimoCambioBomba;
extern unsigned long lastReadTime;

extern unsigned long TempTanqueTime;
extern unsigned long TempHornoTime;
extern unsigned long TempCamaraTime;
extern unsigned long TempSalidaTime;
extern unsigned long Bomba1Time;
extern unsigned long Bomba2Time;
extern unsigned long Valv1Time;
extern unsigned long Valv2Time;
extern unsigned long NivelTime;
extern unsigned long PresionTime;
extern unsigned long EstadoTime;

// ================= VARIABLES PARA ALMACENAR EL ULTIMO VALOR MOSTRADO =================
extern char lastEstado[10];
extern float lastTanqueTemp;
extern float lastHornoTemp;
extern float lastCamaraTemp;
extern float lastSalidaTemp;
extern int lastNivel;
extern float lastPresion;
extern bool lastBomba1State;
extern bool lastBomba2State;
extern bool lastValv1State;
extern bool lastValv2State;

// Variables comunicación
extern unsigned long lastSendTime;
extern unsigned long lastCommandCheck;
extern int messageCount;

// HMI Nextion
extern HardwareSerial nextionSerial;
extern NexPage mainPage;
extern NexText temp1Tanque;
extern NexText temp2Horno;
extern NexText temp3Camara;
extern NexText temp4Salida;
extern NexText nivel;
extern NexText presion;
extern NexText estado;
extern NexText valvula1Salida;
extern NexText valvula2Entrada;
extern NexText bomba1;
extern NexText bomba2;
extern NexButton startBtn;
extern NexButton stopBtn;
extern NexButton manualBtn;

// Botones on/off
extern NexButton valvula1Btn; // id 34, name b3
extern NexButton valvula2Btn; // id 35, name b4
extern NexButton bomba1Btn;   // id 36, name b5
extern NexButton bomba2Btn;   // id 37, name b6

extern NexTouch *nex_listen_list[];


// ================= DECLARACIONES DE FUNCIONES =================
// Configuración
void configurarPines();

// Sensores
void inicializarTermocuplas();
void leerSensores();
void leerTemperaturas();
double leerTermocupla(Adafruit_MAX31855 &sensor, int numero);
void leerNiveles();
void leerPresion();
void leerPulsadores();

// Control
void controlarSistema();
void iniciarSistema();
void detenerSistema();
bool verificarCondicionesInicio();
bool verificarCondicionesApagado();
void activarCirculacion();
void alternarBombas();
void manual();

// Seguridad
void apagarTodo();
void verificarSeguridad();

// HMI
void actualizarEstadoSistemaHMI();
void actualizarTemperaturaTanque();
void actualizarTemperaturaHorno();
void actualizarTemperaturaCamara();
void actualizarTemperaturaSalida();
void actualizarNivel();
void actualizarPresion();
void actualizarBomba1();
void actualizarBomba2();
void actualizarValvula1();
void actualizarValvula2();
void startBtnCallback(void *ptr);
void stopBtnCallback(void *ptr);
void manualBtnCallback(void *ptr);

void valvula1BtnCallback(void *ptr);
void valvula2BtnCallback(void *ptr);
void bomba1BtnCallback(void *ptr); 
void bomba2BtnCallback(void *ptr);

// Comunicación
void connectToWiFi();
void checkForCommands();
void sendSystemData();
void handleServerCommunication();

#endif
