#ifndef SENSORES_H
#define SENSORES_H

#include <Adafruit_MAX31855.h>

// Inicialización
void inicializarTermocuplas();

// Lectura de sensores
void leerSensores();
void leerTemperaturas();
double leerTermocupla(Adafruit_MAX31855 &sensor, int numero);
void leerPresion();
void leerNiveles();
void leerPulsadores();

#endif
