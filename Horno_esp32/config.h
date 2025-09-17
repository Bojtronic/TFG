#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include "Adafruit_MAX31855.h"
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
#define START_BTN 32
#define STOP_BTN  13 

// Comunicación con HMI Nextion (UART2)
#define NEXTION_RX 16
#define NEXTION_TX 17


// ================= PARÁMETROS DE CONFIGURACIÓN =================
#define TEMP_AGUA_CALIENTE 60.0     // Temperatura objetivo agua caliente (°C)
#define TEMP_MAX_TANQUE    85.0     // Temperatura máxima en tanque (°C)
#define TEMP_MAX_HORNO     250.0    // Temperatura máxima en horno (°C)
#define TEMP_MAX_CAMARA    150.0    // Temperatura máxima en cámara (°C)
#define PRESION_MAXIMA     4.0      // Presión máxima permitida (bar)
#define NIVEL_LLENO        80       // % para considerar tanque lleno
#define NIVEL_MITAD        40       // % para considerar tanque a la mitad
#define NIVEL_VACIO        10       // % para considerar tanque vacío
#define PRESION_UMBRAL     3.5      // Presión para alerta (bar)

#define INTERVALO_CAMBIO_BOMBA 3600000 // 1 hora
#define LECTURA_INTERVAL 1000          // 1 segundo

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
extern int niveles[3];
extern float presionActual;

// Estados del sistema
enum EstadoSistema {
  SISTEMA_APAGADO,
  LLENADO_TANQUE,
  CALENTAMIENTO,
  CIRCULACION,
  ENTREGA_AGUA,
  EMERGENCIA,
  MANTENIMIENTO
};

extern EstadoSistema estadoActual;
extern bool emergencia;
extern bool bombaPrincipalActiva;
extern unsigned long ultimoCambioBomba;
extern unsigned long lastReadTime;

// Variables comunicación
extern unsigned long lastSendTime;
extern unsigned long lastCommandCheck;
extern int messageCount;

// HMI Nextion
extern HardwareSerial nextionSerial;
extern NexPage mainPage;
extern NexText temp1Text; // tanque
extern NexText temp2Text; // horno
extern NexText temp3Text; // camara
extern NexText temp4Text; // salida
extern NexText nivel1Text;
extern NexText nivel2Text;
extern NexText nivel3Text;
extern NexText presionText;
extern NexText estadoText;
extern NexPicture valvula1State;
extern NexPicture valvula2State;
extern NexPicture bomba1State;
extern NexPicture bomba2State;
extern NexButton startBtn;
extern NexButton stopBtn;
extern NexButton resetBtn;
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
void controlarLlenado();
void controlarCalentamiento();
void controlarCirculacion();
void controlarEntregaAgua();
void activarCirculacion();
void alternarBombas();

// Seguridad
void apagarTodo();
void verificarSeguridad();
void activarEmergencia(const char* mensaje);

// HMI
void actualizarHMI();
void actualizarTextoHMI(NexText &componente, double valor, const char* unidad);
void actualizarEstadoComponente(NexPicture &componente, bool estado);
void actualizarEstadoSistemaHMI();

// Comunicación
void connectToWiFi();
void checkForCommands();
void sendSystemData();
void testServerConnection();
void handleServerCommunication();

// Callbacks Nextion
void startBtnCallback(void *ptr);
void stopBtnCallback(void *ptr);
void resetBtnCallback(void *ptr);

#endif
