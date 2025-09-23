#ifndef TEST_H
#define TEST_H

#include "config.h"

// Pruebas por estado
void testApagado();
void testProcesando();
void testDetener(int caso);
void testManual();
void testEmergencia(int caso);

// Controlador de pruebas (ejecuta cada escenario 10s)
void ejecutarPruebas();

#endif
