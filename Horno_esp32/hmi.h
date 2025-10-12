#ifndef HMI_H
#define HMI_H

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

#endif
