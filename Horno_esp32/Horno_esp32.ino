#include "config.h"
#include "sensores.h"
#include "control.h"
#include "seguridad.h"
#include "comunicacion.h"
#include "hmi.h"
#include "test.h"


// ================= CONFIGURACIÓN INICIAL =================
void setup() {
  //Serial.begin(115200);
  //delay(1500);

  nextionSerial.begin(9600, SERIAL_8N1, NEXTION_RX, NEXTION_TX);
  nexInit();

  configurarPines();
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
  /*
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  Serial.println("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  */
}

int numTest = 0;


// ================= BUCLE PRINCIPAL =================
void loop() {
  nexLoop(nex_listen_list);
  
  leerPulsadores();

  unsigned long now = millis();

  if (now - lastReadTime >= LECTURA_INTERVAL) {
    lastReadTime = now;
    
    
    if(numTest < 4){
      numTest++;
    }
    else{
      numTest = 0;
    }
    testProcesando(numTest);
    
    
    //ejecutarPruebas();

    leerNiveles();
    //leerSensores();
    verificarSeguridad();
    controlarSistema();
    
    // Manejo de comunicación con servidor
    handleServerCommunication(); 

    

    //Serial.println("("**************** PROCESO ****************"); 
  }


  if (now - Bomba1Time >= ESCRITURA_BOMBA1_INTERVAL) {
    Bomba1Time = now;
    actualizarBomba1();  
  }

  if (now - Bomba2Time >= ESCRITURA_BOMBA2_INTERVAL) {
    Bomba2Time = now;
    actualizarBomba2();  
  }

  if (now - Valv1Time >= ESCRITURA_VALV1_INTERVAL) {
    Valv1Time = now;
    actualizarValvula1();  
  }

  if (now - Valv2Time >= ESCRITURA_VALV2_INTERVAL) {
    Valv2Time = now;
    actualizarValvula2();  
  }

  if (now - EstadoTime >= ESCRITURA_ESTADO_INTERVAL) {
    EstadoTime = now;
    actualizarEstadoSistemaHMI();
  }

  if (now - NivelTime >= ESCRITURA_NIVEL_INTERVAL) {
    NivelTime = now;
    actualizarNivel();  
  }

  if (now - PresionTime >= ESCRITURA_PRESION_INTERVAL) {
    PresionTime = now;
    actualizarPresion();  
  }

  if (now - TempTanqueTime >= ESCRITURA_TEMPTANQUE_INTERVAL) {
    TempTanqueTime = now;
    actualizarTemperaturaTanque();  
  }

  if (now - TempHornoTime >= ESCRITURA_TEMPHORNO_INTERVAL) {
    TempHornoTime = now;
    actualizarTemperaturaHorno();
  }

  if (now - TempCamaraTime >= ESCRITURA_TEMPCAMARA_INTERVAL) {
    TempCamaraTime = now;
    actualizarTemperaturaCamara();  
  }

  if (now - TempSalidaTime >= ESCRITURA_TEMPSALIDA_INTERVAL) {
    TempSalidaTime = now;
    actualizarTemperaturaSalida();  
  }

  //Serial.println("-------------- LOOP --------------");
  
  delay(10);
  
}


