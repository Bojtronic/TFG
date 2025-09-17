#include "config.h"
#include "sensores.h"
#include "control.h"
#include "seguridad.h"
#include "hmi.h"
#include "comunicacion.h"

// ================= CONFIGURACIÓN INICIAL =================
void setup() {
  Serial.begin(115200);
  nextionSerial.begin(9600, SERIAL_8N1, NEXTION_TX, NEXTION_RX);
  
  configurarPines();
  nexInit();
  inicializarTermocuplas();
  
  // Configurar callbacks de Nextion
  startBtn.attachPush(startBtnCallback, &startBtn);
  stopBtn.attachPush(stopBtnCallback, &stopBtn);
  resetBtn.attachPush(resetBtnCallback, &resetBtn);
  
  //apagarTodo();
  
  // Conectar a WiFi
  connectToWiFi();
  /*
  if (WiFi.status() == WL_CONNECTED) {
    testServerConnection();
  }
  */
  
  Serial.println("Sistema inicializado - Modo APAGADO");
  mensajesHMI("Sistema listo - Modo APAGADO");
  actualizarEstadoSistemaHMI(); // esto se podria reemplazar con la funcion mensajesHMI(ESTADO) pero depende del layout en el hmi
}

// ================= BUCLE PRINCIPAL =================
void loop() {
  nexLoop(nex_listen_list);
  
  unsigned long now = millis();

  if (now - lastReadTime >= LECTURA_INTERVAL) {
    lastReadTime = now;
    
    leerSensores();
    /*
    actualizarHMI();
    
    if (estadoActual != SISTEMA_APAGADO && estadoActual != EMERGENCIA) {
      verificarSeguridad();
      controlarSistema();
      alternarBombas();
    }
    */
  }
  
  //leerPulsadores();
  
  // Manejo de comunicación con servidor
  handleServerCommunication();
  
  
  /*
  static unsigned long lastStatusTime = 0;
  if (now - lastStatusTime > 5000) {
    lastStatusTime = now;
    
    Serial.println(" | 🌡️ Temp: ");
    Serial.println(temperaturas[0], 1);
    Serial.println(temperaturas[1], 1);
    Serial.println(temperaturas[2], 1);
    Serial.println(temperaturas[3], 1);
  }
  */
  
  delay(100);
}
