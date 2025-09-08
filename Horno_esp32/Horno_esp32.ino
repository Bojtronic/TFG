#include "config.h"
#include "sensores.h"
#include "control.h"
#include "seguridad.h"
#include "hmi.h"
#include "comunicacion.h"  // ← Añadido

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
  actualizarEstadoSistemaHMI();
}

// ================= BUCLE PRINCIPAL =================
void loop() {
  nexLoop(nex_listen_list);
  
  unsigned long now = millis();

  if (now - lastReadTime >= LECTURA_INTERVAL) {
    lastReadTime = now;
    
    leerSensores();
    actualizarHMI();
    
    if (estadoActual != SISTEMA_APAGADO && estadoActual != EMERGENCIA) {
      verificarSeguridad();
      controlarSistema();
      alternarBombas();
    }
  }
  
  leerPulsadores();
  
  // Manejo de comunicación con servidor
  handleServerCommunication();
  
  // Mostrar estado del sistema periódicamente
  /*
  static unsigned long lastStatusTime = 0;
  if (now - lastStatusTime > 15000) {
    lastStatusTime = now;
    Serial.print("💡 Estado: ");
    Serial.print(estadoActual);
    Serial.print(" | 🌡️ Temp: ");
    Serial.print(temperaturas[0], 1);
    Serial.print(" | 💧 Nivel: ");
    Serial.print(niveles[2]);
    Serial.print("% | 📶 RSSI: ");
    Serial.println(WiFi.RSSI());
  }
  */
  
  delay(100);
}
