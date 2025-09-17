#ifndef CONTROL_H
#define CONTROL_H

#include "config.h"

// Control principal
void controlarSistema();
void iniciarSistema();
bool verificarCondicionesInicio();
void detenerSistema();

// Control de actuadores
//void activarCirculacion();
void alternarBombas();

#endif
