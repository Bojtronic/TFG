#ifndef HMI_H
#define HMI_H

#include "config.h"

void actualizarEstadoSistemaHMI();
void actualizarTemperaturas();
void actualizarNivel();
void actualizarPresion();
void actualizarActuadores();
void startBtnCallback(void *ptr);
void stopBtnCallback(void *ptr);
void manualBtnCallback(void *ptr);

#endif
