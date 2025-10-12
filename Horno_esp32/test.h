#ifndef TEST_H
#define TEST_H

#include "config.h"

// Prueba de conexion
void testServerConnection();

// Pruebas por estado
void testApagado();
void testProcesando(int caso);
void testDetener(int caso);
void testManual();
void testEmergencia(int caso);

// Controlador de pruebas (ejecuta cada escenario 10s)
void ejecutarPruebas();

#endif
