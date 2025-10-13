#ifndef TEST_H
#define TEST_H

// Prueba de conexion
void testServerConnection();

// Pruebas por estado
void testApagado();
void testProcesando(int caso);
void testDetener(int caso);
void testManual();
void testEmergencia(int caso);

// Controlador de pruebas (ejecuta cada escenario Xs)
void ejecutarPruebas();

#endif
