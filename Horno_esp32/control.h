#ifndef CONTROL_H
#define CONTROL_H

#include "config.h"

// Control principal
void controlarSistema();

// Gestión de estados
void iniciarSistema();
void detenerSistema();
bool verificarCondicionesInicio();

// Sub-estados
void controlarLlenado();
void controlarCalentamiento();
void controlarCirculacion();
void controlarEntregaAgua();

// Control de actuadores
void activarCirculacion();
void alternarBombas();

#endif
