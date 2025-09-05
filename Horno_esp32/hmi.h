#ifndef HMI_H
#define HMI_H

#include "config.h"

// Actualización HMI
void actualizarHMI();
void actualizarTextoHMI(NexText &componente, double valor, const char* unidad);
void actualizarEstadoComponente(NexPicture &componente, bool estado);
void actualizarEstadoSistemaHMI();
void mensajesHMI(const char* mensaje);

// Callbacks Nextion
void startBtnCallback(void *ptr);
void stopBtnCallback(void *ptr);
void resetBtnCallback(void *ptr);

#endif
