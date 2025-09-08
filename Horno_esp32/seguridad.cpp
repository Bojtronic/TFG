#include "seguridad.h"
#include "config.h"
#include "hmi.h"

// ================= FUNCIONES DE SEGURIDAD =================

// Apaga todas las válvulas y bombas para un estado seguro
void apagarTodo() {
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, LOW);
    digitalWrite(BOMBA_1, LOW);
    digitalWrite(BOMBA_2, LOW);

    Serial.println("✅ Todos los actuadores apagados");
}

// Verifica las condiciones de seguridad del sistema
void verificarSeguridad() {
    // 1. Sobrepresión
    if (presionActual > PRESION_MAXIMA) {
        activarEmergencia("EMERGENCIA: Sobrepresion!");
        return;
    }

    // 2. Sobretemperatura en tanque
    if (temperaturas[0] > TEMP_MAX_TANQUE) {
        activarEmergencia("EMERGENCIA: Temp tanque alta!");
        return;
    }

    // 3. Sobretemperatura en horno
    if (temperaturas[1] > TEMP_MAX_HORNO) {
        activarEmergencia("EMERGENCIA: Temp horno alta!");
        return;
    }

    // 4. Sobretemperatura en cámara
    if (temperaturas[2] > TEMP_MAX_CAMARA) {
        activarEmergencia("EMERGENCIA: Temp camara alta!");
        return;
    }

    // 5. Nivel bajo con bomba activa
    if ((digitalRead(BOMBA_1) == HIGH || digitalRead(BOMBA_2) == HIGH) &&
        niveles[0] < NIVEL_VACIO) {
        activarEmergencia("EMERGENCIA: Nivel bajo con bomba!");
        return;
    }
}

// Activa el modo de emergencia, apaga el sistema y notifica HMI
void activarEmergencia(const char* mensaje) {
    estadoActual = EMERGENCIA;
    emergencia = true;

    // Apagar todo por seguridad
    apagarTodo();

    // Mensaje a HMI y Serial
    mensajesHMI(mensaje);
    Serial.println(mensaje);

}
