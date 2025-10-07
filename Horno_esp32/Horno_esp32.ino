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
  delay(1500);

  nextionSerial.begin(9600, SERIAL_8N1, NEXTION_RX, NEXTION_TX);
  
  configurarPines();
  nexInit();
  inicializarTermocuplas();
  
  startBtn.attachPush(startBtnCallback, &startBtn);
  stopBtn.attachPush(stopBtnCallback, &stopBtn);
  manualBtn.attachPush(manualBtnCallback, &manualBtn);

  valvula1Btn.attachPush(valvula1BtnCallback, &valvula1Btn);
  valvula2Btn.attachPush(valvula2BtnCallback, &valvula2Btn);
  bomba1Btn.attachPush(bomba1BtnCallback, &bomba1Btn);
  bomba2Btn.attachPush(bomba2BtnCallback, &bomba2Btn);


  
  // Conectar a WiFi
  //connectToWiFi();
  
  
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
    
    // Manejo de comunicación con servidor
    //handleServerCommunication(); 

    
    actualizarEstadoSistemaHMI();
    actualizarTemperaturas();
    actualizarNivel();
    actualizarPresion();
    actualizarActuadores();

    Serial.print("Temp tanque: ");
    Serial.println(temperaturas[0]);

    Serial.print("Temp horno: ");
    Serial.println(temperaturas[1]);

    Serial.print("Temp camara: ");
    Serial.println(temperaturas[2]);

    Serial.print("Temp salida: ");
    Serial.println(temperaturas[3]);

    Serial.print("Nivel tanque: ");
    Serial.println(nivelTanque);

    Serial.print("Presion: ");
    Serial.println(presionActual);

    //Serial.println("("**************** PROCESO ****************"); 
  }
  
  
  //Serial.println("-------------- LOOP --------------");
  
  delay(10);
}


