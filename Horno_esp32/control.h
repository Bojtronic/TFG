#ifndef CONTROL_H
#define CONTROL_H

// Control principal
void controlarSistema();
void iniciarSistema();
bool verificarCondicionesInicio();
void detenerSistema();
bool verificarCondicionesApagado();

// Control de actuadores
void activarCirculacion();
void alternarBombas();
void manual();

#endif
