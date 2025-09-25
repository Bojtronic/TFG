#include "config.h"
#include "sensores.h"
#include "control.h"
#include "seguridad.h"
#include "hmi.h"
#include "comunicacion.h"
#include "test.h"

// ================= CONFIGURACIÓN INICIAL =================
void setup() {
  Serial.begin(115200);
  nextionSerial.begin(9600, SERIAL_8N1, NEXTION_TX, NEXTION_RX);
  
  configurarPines();
  nexInit();
  inicializarTermocuplas();
  
  // Configurar callbacks de Nextion
  //startBtn.attachPush(startBtnCallback, &startBtn);
  //stopBtn.attachPush(stopBtnCallback, &stopBtn);
  //resetBtn.attachPush(resetBtnCallback, &resetBtn);
  
  //apagarTodo();
  
  // Conectar a WiFi
  connectToWiFi();
  /*
  if (WiFi.status() == WL_CONNECTED) {
    testServerConnection();
  }
  */
  
  
  //actualizarEstadoSistemaHMI(); // esto se podria reemplazar con la funcion mensajesHMI(ESTADO) pero depende del layout en el hmi
  estadoActual = APAGADO;

  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
}

//int numTest = 0;

// ================= BUCLE PRINCIPAL =================
void loop() {
  nexLoop(nex_listen_list);
  
  leerPulsadores();

  unsigned long now = millis();

  if (now - lastReadTime >= LECTURA_INTERVAL) {
    lastReadTime = now;
    
    /*
    if(numTest < 4){
      numTest++;
    }
    else{
      numTest = 0;
    }
    testProcesando(numTest);
    */

    ejecutarPruebas();

    //leerSensores();
    verificarSeguridad();
    controlarSistema();
    
    //actualizarHMI();
    
    //leerPulsadores();

    // Manejo de comunicación con servidor
    handleServerCommunication(); 

    //Serial.println("("**************** PROCESO ****************"); 
  }
  
  
  Serial.println("-------------- LOOP --------------");
  
  delay(10);
}
