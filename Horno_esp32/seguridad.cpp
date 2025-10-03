#include "seguridad.h"
#include "config.h"
//#include "hmi.h"

// ================= FUNCIONES DE SEGURIDAD =================

// Apaga todas las válvulas y bombas para un estado seguro
void apagarTodo() {
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, LOW);
    digitalWrite(BOMBA_1, LOW);
    digitalWrite(BOMBA_2, LOW);
}

// Verifica las condiciones de seguridad del sistema
void verificarSeguridad() {
    // 1. No hay flujo de agua para llenar el tanque(no hay presión)
    if ((presionActual < PRESION_MINIMA) && (estadoActual != APAGADO)) {
        if((nivelTanque <= NIVEL_VACIO) && (temperaturas[2] > TEMP_MIN_HORNO)){
            mensajeActual = EMERGENCIA_2;

            digitalWrite(BOMBA_1, LOW);
            digitalWrite(BOMBA_2, LOW);
            digitalWrite(VALVULA_1, LOW);
            digitalWrite(VALVULA_2, LOW);

            // Enviar mensaje por whatsapp
            Serial.println("Emergencia severa no hay agua en la entrada ni en el tanque y el horno está caliente");
        }
        else if((nivelTanque > NIVEL_VACIO) && (temperaturas[2] > TEMP_MIN_HORNO)){
            mensajeActual = EMERGENCIA_3;

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
        estadoActual = EMERGENCIA;
        return;
    }

    // 2. Sobretemperatura en tanque
    if (temperaturas[0] >= TEMP_MAX_TANQUE) {
        if((nivelTanque <= NIVEL_LLENO) && (temperaturas[1] > TEMP_MIN_HORNO) && (temperaturas[2] > TEMP_MIN_HORNO)){
            mensajeActual = EMERGENCIA_4;
            
            digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }
            Serial.println("Emergencia leve, Tanque muy caliente: el tanque no está lleno, el horno está caliente y la cámara está caliente");
        }
        else if((nivelTanque <= NIVEL_LLENO) && (temperaturas[1] < TEMP_MIN_HORNO) && (temperaturas[2] < TEMP_MIN_HORNO)){
            mensajeActual = EMERGENCIA_5;
            
            digitalWrite(VALVULA_2, LOW); // No hace falta abrir la llave para llenar el tanque con agua fria porque el horno esta frio
            digitalWrite(BOMBA_1, LOW);
            digitalWrite(BOMBA_2, LOW);
            Serial.println("Emergencia leve, Tanque muy caliente: el tanque no está lleno, pero el horno y la cámara están fríos");
        }
        estadoActual = EMERGENCIA;
        return;
    }

    // 3. Sobretemperatura en horno
    if (temperaturas[1] > TEMP_MAX_HORNO) {
        if((nivelTanque < NIVEL_LLENO)){
            mensajeActual = EMERGENCIA_6;
            
            digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }
            Serial.println("Emergencia leve: Horno muy caliente y el tanque no está lleno");
        }
        else if ((nivelTanque >= NIVEL_LLENO)){
            mensajeActual = EMERGENCIA_7;
            
            digitalWrite(VALVULA_2, LOW); // No hace falta abrir la llave para llenar el tanque con agua fria porque el horno esta frio
            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }
            Serial.println("Emergencia leve: Horno muy caliente y el tanque está lleno");
        }
        estadoActual = EMERGENCIA;
        return;
    }

    // 4. Sobretemperatura en cámara
    if (temperaturas[2] > TEMP_MAX_CAMARA) {
        if(nivelTanque <= NIVEL_LLENO){
            mensajeActual = EMERGENCIA_8;
            
            digitalWrite(VALVULA_2, HIGH); // Abrir la llave para llenar el tanque con agua fria
            // Activar la bomba principal o redundante según alternancia
            if (bombaPrincipalActiva) {
                digitalWrite(BOMBA_1, HIGH);
                digitalWrite(BOMBA_2, LOW);
            } else {
                digitalWrite(BOMBA_1, LOW);
                digitalWrite(BOMBA_2, HIGH);
            }
            Serial.println("Emergencia leve: Cámara muy caliente y el tanque no está lleno");
        }
        estadoActual = EMERGENCIA;
        return;
    }
}

