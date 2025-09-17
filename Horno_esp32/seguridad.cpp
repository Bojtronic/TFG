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

    estadoActual = APAGADO;

    Serial.println("Todos los actuadores apagados - Estado apagado");
}

// Verifica las condiciones de seguridad del sistema
void verificarSeguridad() {
    // 1. No hay flujo de agua para llenar el tanque(no hay presión)
    if (presionActual < PRESION_MINIMA) {
        activarEmergencia("EMERGENCIA: Sin agua entrando!");
        if((nivelTanque <= NIVEL_VACIO) && (temperaturas[2] > TEMP_MIN_HORNO)){
            
            digitalWrite(BOMBA_1, LOW);
            digitalWrite(BOMBA_2, LOW);
            digitalWrite(VALVULA_1, LOW);
            digitalWrite(VALVULA_2, LOW);

            // Enviar mensaje por whatsapp
            Serial.println("Emergencia severa no hay agua en la entrada ni en el tanque y el horno está caliente");
        }
        else if((nivelTanque > NIVEL_VACIO) && (temperaturas[2] > TEMP_MIN_HORNO)){
            
            digitalWrite(VALVULA_1, LOW);
            digitalWrite(VALVULA_2, LOW);

            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }

            // Enviar mensaje por whatsapp
            Serial.println("Emergencia moderada no hay agua en la entrada y el horno está caliente, pero queda agua en el tanque");
        }
        return;
    }

    // 2. Sobretemperatura en tanque
    if (temperaturas[0] > TEMP_MAX_TANQUE) {
        activarEmergencia("EMERGENCIA: Tanque muy caliente!");
        if((nivelTanque <= NIVEL_LLENO) && (temperaturas[1] >= TEMP_MIN_HORNO) && (temperaturas[2] >= TEMP_MIN_HORNO)){
            digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }
        }
        else if((nivelTanque <= NIVEL_LLENO) && (temperaturas[1] < TEMP_MIN_HORNO) && (temperaturas[2] < TEMP_MIN_HORNO)){
            digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
            digitalWrite(BOMBA_1, LOW);
            digitalWrite(BOMBA_2, LOW);
        }
        return;
    }

    // 3. Sobretemperatura en horno
    if (temperaturas[1] > TEMP_MAX_HORNO) {
        activarEmergencia("EMERGENCIA: Horno muy caliente!");
        if(nivelTanque <= NIVEL_LLENO){
            digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }
        }
        return;
    }

    // 4. Sobretemperatura en cámara
    if (temperaturas[2] > TEMP_MAX_CAMARA) {
        activarEmergencia("EMERGENCIA: Camara muy caliente!");
        if(nivelTanque <= NIVEL_LLENO){
            digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }
        }
        return;
    }
}

// Activa el modo de emergencia, apaga el sistema y notifica HMI
// En realidad se deben tomar diferentes medidas de emergencia
// Si el horno esta calentando debe haber agua circulando
// si el tanque esta vacio se debe llenar el tanque
// si no hay presion de agua a la entrada se debe apagar todo y no permitir salida de agua caliente para que siga circulando mientras el horno se enfria
void activarEmergencia(const char* mensaje) {
    estadoActual = EMERGENCIA;
    emergencia = true;

    // Mensaje a HMI y Serial
    mensajesHMI(mensaje);
    Serial.println(mensaje);

}
