#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <Nextion.h>

// ================= CONFIGURACIÓN DE PINES =================
// Termocuplas (MAX31855) - 4 sensores de temperatura
#define MAX_CLK   18
#define MAX_DO1   19
#define MAX_CS1   5
#define MAX_DO2   23
#define MAX_CS2   22
#define MAX_DO3   16
#define MAX_CS3   17
#define MAX_DO4   4
#define MAX_CS4   15

// Sensores de nivel de agua (entradas analógicas)
#define NIVEL_1   36  // Vacío (GPIO36 - solo entrada)
#define NIVEL_2   39  // Mitad (GPIO39 - solo entrada)
#define NIVEL_3   34  // Lleno (GPIO34 - solo entrada)

// Sensor de presión (entrada analógica)
#define PRESSURE_SENSOR 35  // GPIO35 - solo entrada

// Electrovalvulas (salidas digitales)
#define VALVULA_1 25  // Salida agua caliente
#define VALVULA_2 26  // Entrada agua fría

// Bombas de agua (salidas digitales)
#define BOMBA_1   27  // Circulación principal
#define BOMBA_2   14  // Circulación redundante

// Pulsadores de control
#define START_BTN 32
#define STOP_BTN  33
#define EMO_BTN   13  // Paro de emergencia (con pull-up interno)

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

// ================= VARIABLES GLOBALES =================
// Termocuplas
Adafruit_MAX31855 thermocouple1(MAX_CLK, MAX_CS1, MAX_DO1);
Adafruit_MAX31855 thermocouple2(MAX_CLK, MAX_CS2, MAX_DO2);
Adafruit_MAX31855 thermocouple3(MAX_CLK, MAX_CS3, MAX_DO3);
Adafruit_MAX31855 thermocouple4(MAX_CLK, MAX_CS4, MAX_DO4);

double temperaturas[4] = {0, 0, 0, 0}; // Temperaturas de los 4 sensores
// [0] = Temp tanque (sensor 1)
// [1] = Temp horno (sensor 2)
// [2] = Temp cámara humo (sensor 3)
// [3] = Temp salida agua (sensor 4)

// Sensores de nivel
int niveles[3] = {0, 0, 0};           // Lecturas de nivel en porcentaje
// [0] = Nivel vacío
// [1] = Nivel mitad
// [2] = Nivel lleno

// Presión
float presionActual = 0.0;            // Presión actual en bar

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

EstadoSistema estadoActual = SISTEMA_APAGADO;
bool emergencia = false;
bool bombaPrincipalActiva = true;     // Alternancia entre bombas
unsigned long ultimoCambioBomba = 0;
const unsigned long INTERVALO_CAMBIO_BOMBA = 3600000; // 1 hora

// Tiempos de control
unsigned long lastReadTime = 0;
const unsigned long LECTURA_INTERVAL = 1000; // Lectura cada 1 segundo

// Configuración HMI Nextion
HardwareSerial nextionSerial(2);

// Componentes Nextion
NexPage mainPage = NexPage(0, 0, "mainPage");
NexText temp1Text = NexText(0, 1, "temp1"); // Temp tanque
NexText temp2Text = NexText(0, 2, "temp2"); // Temp horno
NexText temp3Text = NexText(0, 3, "temp3"); // Temp cámara
NexText temp4Text = NexText(0, 4, "temp4"); // Temp salida
NexText nivel1Text = NexText(0, 5, "nivel1");
NexText nivel2Text = NexText(0, 6, "nivel2");
NexText nivel3Text = NexText(0, 7, "nivel3");
NexText presionText = NexText(0, 8, "presion");
NexText estadoText = NexText(0, 9, "estado");
NexPicture valvula1State = NexPicture(0, 10, "valv1State");
NexPicture valvula2State = NexPicture(0, 11, "valv2State");
NexPicture bomba1State = NexPicture(0, 12, "bomba1State");
NexPicture bomba2State = NexPicture(0, 13, "bomba2State");
NexButton startBtn = NexButton(0, 14, "startBtn");
NexButton stopBtn = NexButton(0, 15, "stopBtn");
NexButton resetBtn = NexButton(0, 16, "resetBtn");

NexTouch *nex_listen_list[] = {
  &startBtn,
  &stopBtn,
  &resetBtn,
  NULL
};

// ================= CONFIGURACIÓN INICIAL =================
void setup() {
  Serial.begin(115200);
  nextionSerial.begin(9600, SERIAL_8N1, NEXTION_TX, NEXTION_RX);
  
  // 1. Configurar pines
  configurarPines();
  
  // 2. Inicializar Nextion
  nexInit();
  
  // 3. Inicializar termocuplas
  inicializarTermocuplas();
  
  // 4. Configurar callbacks de Nextion
  startBtn.attachPush(startBtnCallback, &startBtn);
  stopBtn.attachPush(stopBtnCallback, &stopBtn);
  resetBtn.attachPush(resetBtnCallback, &resetBtn);
  
  // 5. Estado inicial seguro
  apagarTodo();
  
  Serial.println("Sistema inicializado - Modo APAGADO");
  mensajesHMI("Sistema listo - Modo APAGADO");
  actualizarEstadoSistemaHMI();
}

// ================= BUCLE PRINCIPAL =================
void loop() {
  // 1. Procesar eventos de Nextion
  nexLoop(nex_listen_list);
  
  // 2. Leer sensores cada segundo
  if (millis() - lastReadTime >= LECTURA_INTERVAL) {
    lastReadTime = millis();
    
    leerSensores();
    actualizarHMI();
    
    if (estadoActual != SISTEMA_APAGADO && estadoActual != EMERGENCIA) {
      verificarSeguridad();
      controlarSistema();
      alternarBombas();
    }
  }
  
  // 3. Leer pulsadores de hardware
  leerPulsadores();
}

// ================= FUNCIONES DE CONFIGURACIÓN =================
void configurarPines() {
  // Configurar entradas
  pinMode(NIVEL_1, INPUT);
  pinMode(NIVEL_2, INPUT);
  pinMode(NIVEL_3, INPUT);
  pinMode(PRESSURE_SENSOR, INPUT);
  pinMode(START_BTN, INPUT);
  pinMode(STOP_BTN, INPUT);
  pinMode(EMO_BTN, INPUT_PULLUP);
  
  // Configurar salidas
  pinMode(VALVULA_1, OUTPUT);
  pinMode(VALVULA_2, OUTPUT);
  pinMode(BOMBA_1, OUTPUT);
  pinMode(BOMBA_2, OUTPUT);
  
  // Estado inicial seguro
  apagarTodo();
}

void inicializarTermocuplas() {
  Serial.println("Inicializando termocuplas...");
  
  if (!thermocouple1.begin()) Serial.println("ERROR Termocupla 1 (Tanque)");
  if (!thermocouple2.begin()) Serial.println("ERROR Termocupla 2 (Horno)");
  if (!thermocouple3.begin()) Serial.println("ERROR Termocupla 3 (Camara)");
  if (!thermocouple4.begin()) Serial.println("ERROR Termocupla 4 (Salida)");
  
  Serial.println("Termocuplas inicializadas");
}

// ================= FUNCIONES DE SEGURIDAD =================
void apagarTodo() {
  digitalWrite(VALVULA_1, LOW);
  digitalWrite(VALVULA_2, LOW);
  digitalWrite(BOMBA_1, LOW);
  digitalWrite(BOMBA_2, LOW);
}

void verificarSeguridad() {
  // 1. Verificar sobrepresión
  if (presionActual > PRESION_MAXIMA) {
    activarEmergencia("EMERGENCIA: Sobrepresion!");
    return;
  }
  
  // 2. Verificar sobretemperatura en tanque
  if (temperaturas[0] > TEMP_MAX_TANQUE) {
    activarEmergencia("EMERGENCIA: Temp tanque alta!");
    return;
  }
  
  // 3. Verificar sobretemperatura en horno
  if (temperaturas[1] > TEMP_MAX_HORNO) {
    activarEmergencia("EMERGENCIA: Temp horno alta!");
    return;
  }
  
  // 4. Verificar sobretemperatura en cámara
  if (temperaturas[2] > TEMP_MAX_CAMARA) {
    activarEmergencia("EMERGENCIA: Temp camara alta!");
    return;
  }
  
  // 5. Verificar nivel bajo con bomba activa
  if ((digitalRead(BOMBA_1) == HIGH || digitalRead(BOMBA_2) == HIGH) && 
      niveles[0] < NIVEL_VACIO) {
    activarEmergencia("EMERGENCIA: Nivel bajo con bomba!");
    return;
  }
  
  // 6. Verificar pulsador de emergencia
  if (digitalRead(EMO_BTN) == LOW) {
    activarEmergencia("EMERGENCIA: Pulsador activado!");
    return;
  }
}

void activarEmergencia(const char* mensaje) {
  estadoActual = EMERGENCIA;
  emergencia = true;
  apagarTodo();
  
  mensajesHMI(mensaje);
  Serial.println(mensaje);
  
  // Alertas audibles/visuales adicionales pueden agregarse aquí
}

// ================= FUNCIONES DE LECTURA =================
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
  
  if (isnan(tempC)) {
    Serial.print("Error lectura termocupla ");
    Serial.println(numero);
    return -999.9;
  }
  
  return tempC;
}

void leerNiveles() {
  // Leer valores crudos y convertir a porcentaje
  int raw1 = analogRead(NIVEL_1);
  int raw2 = analogRead(NIVEL_2);
  int raw3 = analogRead(NIVEL_3);
  
  niveles[0] = map(raw1, 0, 4095, 0, 100); // Vacío
  niveles[1] = map(raw2, 0, 4095, 0, 100); // Mitad
  niveles[2] = map(raw3, 0, 4095, 0, 100); // Lleno
}

void leerPresion() {
  int rawPressure = analogRead(PRESSURE_SENSOR);
  // Convertir lectura analógica a presión en bar (ajustar según sensor)
  // Ejemplo: sensor 0-5V = 0-10 bar -> 0-4095 = 0-10 bar
  presionActual = (rawPressure / 4095.0) * 10.0;
}

void leerPulsadores() {
  // Leer pulsador de inicio
  if (digitalRead(START_BTN) == LOW && estadoActual == SISTEMA_APAGADO) {
    delay(50);
    if (digitalRead(START_BTN) == LOW) {
      iniciarSistema();
    }
    while (digitalRead(START_BTN) == LOW) delay(10);
  }
  
  // Leer pulsador de parada
  if (digitalRead(STOP_BTN) == LOW && estadoActual != SISTEMA_APAGADO && estadoActual != EMERGENCIA) {
    delay(50);
    if (digitalRead(STOP_BTN) == LOW) {
      detenerSistema();
    }
    while (digitalRead(STOP_BTN) == LOW) delay(10);
  }
}

// ================= FUNCIONES DE CONTROL =================
void iniciarSistema() {
  if (!emergencia && verificarCondicionesInicio()) {
    estadoActual = LLENADO_TANQUE;
    mensajesHMI("Iniciando sistema - Llenando tanque");
    Serial.println("Sistema iniciado - Modo LLENADO");
  }
}

bool verificarCondicionesInicio() {
  // Verificar que todas las termocuplas funcionen
  for (int i = 0; i < 4; i++) {
    if (temperaturas[i] <= -999.0) {
      mensajesHMI("Error: Verificar sensores temp");
      return false;
    }
  }
  
  // Verificar que no haya emergencias activas
  if (emergencia) {
    mensajesHMI("No se puede iniciar: Emergencia");
    return false;
  }
  
  return true;
}

void detenerSistema() {
  estadoActual = SISTEMA_APAGADO;
  apagarTodo();
  mensajesHMI("Sistema detenido");
  Serial.println("Sistema detenido");
}

void controlarSistema() {
  switch (estadoActual) {
    case LLENADO_TANQUE:
      controlarLlenado();
      break;
    case CALENTAMIENTO:
      controlarCalentamiento();
      break;
    case CIRCULACION:
      controlarCirculacion();
      break;
    case ENTREGA_AGUA:
      controlarEntregaAgua();
      break;
    default:
      break;
  }
}

void controlarLlenado() {
  // Abrir válvula 2 (agua fría) si el nivel está bajo
  if (niveles[2] < NIVEL_LLENO) {
    digitalWrite(VALVULA_2, HIGH);
    mensajesHMI("Llenando tanque...");
  } else {
    // Tanque lleno, cerrar válvula y pasar a calentamiento
    digitalWrite(VALVULA_2, LOW);
    estadoActual = CALENTAMIENTO;
    mensajesHMI("Tanque lleno - Iniciando calentamiento");
    Serial.println("Modo CALENTAMIENTO");
  }
}

void controlarCalentamiento() {
  // Activar circulación para calentamiento
  activarCirculacion();
  
  // Verificar si se alcanzó la temperatura objetivo
  if (temperaturas[3] >= TEMP_AGUA_CALIENTE) {
    estadoActual = CIRCULACION;
    mensajesHMI("Temp alcanzada - Circulacion constante");
    Serial.println("Modo CIRCULACION");
  }
}

void controlarCirculacion() {
  // Mantener circulación constante
  activarCirculacion();
  
  // Controlar nivel del tanque
  if (niveles[2] < NIVEL_MITAD) {
    // Nivel bajo, abrir válvula de llenado
    digitalWrite(VALVULA_2, HIGH);
  } else if (niveles[2] >= NIVEL_LLENO) {
    // Nivel lleno, cerrar válvula
    digitalWrite(VALVULA_2, LOW);
  }
  
  // Verificar si se solicita agua caliente
  if (temperaturas[3] >= TEMP_AGUA_CALIENTE && niveles[0] > NIVEL_VACIO) {
    estadoActual = ENTREGA_AGUA;
    mensajesHMI("Entregando agua caliente");
  }
}

void controlarEntregaAgua() {
  // Abrir válvula 1 (salida agua caliente)
  digitalWrite(VALVULA_1, HIGH);
  
  // Mantener circulación
  activarCirculacion();
  
  // Verificar condiciones para cerrar
  if (niveles[0] <= NIVEL_VACIO || temperaturas[3] < TEMP_AGUA_CALIENTE - 5.0) {
    digitalWrite(VALVULA_1, LOW);
    estadoActual = CIRCULACION;
    mensajesHMI("Entrega completada - Circulando");
  }
}

void activarCirculacion() {
  // Activar la bomba principal o redundante según alternancia
  if (bombaPrincipalActiva) {
    digitalWrite(BOMBA_1, HIGH);
    digitalWrite(BOMBA_2, LOW);
  } else {
    digitalWrite(BOMBA_1, LOW);
    digitalWrite(BOMBA_2, HIGH);
  }
}

void alternarBombas() {
  // Alternar bombas cada intervalo definido
  if (millis() - ultimoCambioBomba >= INTERVALO_CAMBIO_BOMBA) {
    bombaPrincipalActiva = !bombaPrincipalActiva;
    ultimoCambioBomba = millis();
    
    Serial.print("Alternando bombas. Activa: ");
    Serial.println(bombaPrincipalActiva ? "Bomba 1" : "Bomba 2");
  }
}

// ================= FUNCIONES HMI NEXTION =================
void actualizarHMI() {
  // Actualizar temperaturas
  actualizarTextoHMI(temp1Text, temperaturas[0], "°C");
  actualizarTextoHMI(temp2Text, temperaturas[1], "°C");
  actualizarTextoHMI(temp3Text, temperaturas[2], "°C");
  actualizarTextoHMI(temp4Text, temperaturas[3], "°C");
  
  // Actualizar niveles
  actualizarTextoHMI(nivel1Text, niveles[0], "%");
  actualizarTextoHMI(nivel2Text, niveles[1], "%");
  actualizarTextoHMI(nivel3Text, niveles[2], "%");
  
  // Actualizar presión
  char presionStr[10];
  dtostrf(presionActual, 4, 1, presionStr);
  strcat(presionStr, " bar");
  presionText.setText(presionStr);
  
  // Actualizar estados de válvulas y bombas
  actualizarEstadoComponente(valvula1State, digitalRead(VALVULA_1) == HIGH);
  actualizarEstadoComponente(valvula2State, digitalRead(VALVULA_2) == HIGH);
  actualizarEstadoComponente(bomba1State, digitalRead(BOMBA_1) == HIGH);
  actualizarEstadoComponente(bomba2State, digitalRead(BOMBA_2) == HIGH);
  
  // Actualizar estado del sistema
  actualizarEstadoSistemaHMI();
}

void actualizarTextoHMI(NexText &componente, double valor, const char* unidad) {
  char buffer[15];
  if (valor <= -999.0) {
    strcpy(buffer, "ERROR");
  } else {
    dtostrf(valor, 5, 1, buffer);
    strcat(buffer, unidad);
  }
  componente.setText(buffer);
}

void actualizarEstadoComponente(NexPicture &componente, bool estado) {
  componente.setPic(estado ? 1 : 0);
}

void actualizarEstadoSistemaHMI() {
  const char* estadoStr;
  
  switch (estadoActual) {
    case SISTEMA_APAGADO: estadoStr = "APAGADO"; break;
    case LLENADO_TANQUE: estadoStr = "LLENANDO"; break;
    case CALENTAMIENTO: estadoStr = "CALENTANDO"; break;
    case CIRCULACION: estadoStr = "CIRCULANDO"; break;
    case ENTREGA_AGUA: estadoStr = "ENTREGANDO"; break;
    case EMERGENCIA: estadoStr = "EMERGENCIA"; break;
    case MANTENIMIENTO: estadoStr = "MANTENIMIENTO"; break;
    default: estadoStr = "DESCONOCIDO"; break;
  }
  
  estadoText.setText(estadoStr);
}

void mensajesHMI(const char* mensaje) {
  // Implementar según el componente de mensajes en tu HMI
  Serial.print("HMI: ");
  Serial.println(mensaje);
}

// ================= CALLBACKS NEXTION =================
void startBtnCallback(void *ptr) {
  if (estadoActual == SISTEMA_APAGADO) {
    iniciarSistema();
  }
}

void stopBtnCallback(void *ptr) {
  if (estadoActual != SISTEMA_APAGADO && estadoActual != EMERGENCIA) {
    detenerSistema();
  }
}

void resetBtnCallback(void *ptr) {
  if (estadoActual == EMERGENCIA) {
    // Verificar que todas las condiciones de emergencia se hayan resuelto
    if (presionActual <= PRESION_MAXIMA && 
        temperaturas[0] <= TEMP_MAX_TANQUE &&
        temperaturas[1] <= TEMP_MAX_HORNO &&
        temperaturas[2] <= TEMP_MAX_CAMARA &&
        digitalRead(EMO_BTN) == HIGH) {
      
      emergencia = false;
      estadoActual = SISTEMA_APAGADO;
      mensajesHMI("Emergencia reseteda - Sistema apagado");
      Serial.println("Emergencia reseteda");
    } else {
      mensajesHMI("No se puede resetear: Condiciones inseguras");
    }
  }
}
