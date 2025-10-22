#include "config.h"
#include "seguridad.h"

void apagarTodo() {
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, LOW);
    digitalWrite(BOMBA_1, LOW);
    digitalWrite(BOMBA_2, LOW);
}

void verificarSeguridad() {
    // Emergencia 1: No hay presión y horno muy caliente y tanque vacío
    if ((presionActual < PRESION_MINIMA) && 
        (temperaturas[1] >= (TEMP_MIN_HORNO * 2)) && 
        (temperaturas[2] >= (TEMP_MIN_HORNO * 2)) && 
        (nivelTanque <= NIVEL_VACIO)) {
        
        mensajeActual = EMERGENCIA_1;
        estadoActual = EMERGENCIA;
        return;
    }

    // Emergencia 2: No hay presión y horno caliente, tanque vacío
    if ((presionActual < PRESION_MINIMA) && 
        (nivelTanque <= NIVEL_VACIO) && 
        (temperaturas[2] > TEMP_MIN_HORNO)) {
        
        mensajeActual = EMERGENCIA_2;
        estadoActual = EMERGENCIA;
        return;
    }

    // Emergencia 3: No hay presión pero aún queda agua
    if ((presionActual < PRESION_MINIMA) && 
        (nivelTanque > NIVEL_VACIO) && 
        (temperaturas[2] > TEMP_MIN_HORNO)) {
        
        mensajeActual = EMERGENCIA_3;
        estadoActual = EMERGENCIA;
        return;
    }

    // Emergencia 4–5: Sobretemperatura en tanque
    if (temperaturas[0] >= TEMP_MAX_TANQUE) {
        if ((temperaturas[1] > TEMP_MIN_HORNO) && (temperaturas[2] > TEMP_MIN_HORNO))
            mensajeActual = EMERGENCIA_4;
        else
            mensajeActual = EMERGENCIA_5;

        estadoActual = EMERGENCIA;
        return;
    }

    // Emergencia 6–7: Sobretemperatura en horno
    if (temperaturas[1] > TEMP_MAX_HORNO) {
        if (nivelTanque < NIVEL_LLENO)
            mensajeActual = EMERGENCIA_6;
        else
            mensajeActual = EMERGENCIA_7;

        estadoActual = EMERGENCIA;
        return;
    }

    // Emergencia 8: Sobretemperatura en cámara
    if (temperaturas[2] > TEMP_MAX_CAMARA) {
        mensajeActual = EMERGENCIA_8;
        estadoActual = EMERGENCIA;
        return;
    }
}


