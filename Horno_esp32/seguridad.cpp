#include "config.h"
#include "seguridad.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <UrlEncode.h>

void apagarTodo()
{
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, LOW);
    digitalWrite(BOMBA_1, LOW);
    digitalWrite(BOMBA_2, LOW);
}

void enviarWhatsapp(String mensaje)
{
    WiFiClient client;
    HTTPClient http;
    String contenido = "http://api.callmebot.com/whatsapp.php?phone=" + numeroCelular + "&apikey=" + apiKey + "&text=" + urlEncode(mensaje);
    http.begin(client, contenido);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(contenido);
    if (httpResponseCode == 200)
    {
        Serial.print("Mensaje enviado correctamente: ");
    }
    else
    {
        Serial.println("Error al enviar el mensaje.");
        Serial.print("Código de respuesta: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void verificarSeguridad()
{
    // Emergencia 1: No hay presión y horno muy caliente y tanque vacío
    if ((presionActual < PRESION_MINIMA) &&
        (temperaturas[1] >= (TEMP_MIN_HORNO * 2)) &&
        (temperaturas[2] >= (TEMP_MIN_HORNO * 2)) &&
        (nivelTanque <= NIVEL_VACIO))
    {

        mensajeActual = EMERGENCIA_1;
        estadoActual = EMERGENCIA;
        //enviarWhatsapp(MENSAJES[mensajeActual]);
        return;
    }

    // Emergencia 2: No hay presión y horno caliente, tanque vacío
    if ((presionActual < PRESION_MINIMA) &&
        (nivelTanque <= NIVEL_VACIO) &&
        (temperaturas[2] > TEMP_MIN_HORNO))
    {

        mensajeActual = EMERGENCIA_2;
        estadoActual = EMERGENCIA;
        //enviarWhatsapp(MENSAJES[mensajeActual]);
        return;
    }

    // Emergencia 3: No hay presión pero aún queda agua
    if ((presionActual < PRESION_MINIMA) &&
        (nivelTanque > NIVEL_VACIO) &&
        (temperaturas[2] > TEMP_MIN_HORNO))
    {

        mensajeActual = EMERGENCIA_3;
        estadoActual = EMERGENCIA;
        //enviarWhatsapp(MENSAJES[mensajeActual]);
        return;
    }

    // Emergencia 4–5: Sobretemperatura en tanque
    if (temperaturas[0] >= TEMP_MAX_TANQUE)
    {
        if ((temperaturas[1] > TEMP_MIN_HORNO) && (temperaturas[2] > TEMP_MIN_HORNO))
            mensajeActual = EMERGENCIA_4;
        else
            mensajeActual = EMERGENCIA_5;

        estadoActual = EMERGENCIA;
        //enviarWhatsapp(MENSAJES[mensajeActual]);
        return;
    }

    // Emergencia 6–7: Sobretemperatura en horno
    if (temperaturas[1] > TEMP_MAX_HORNO)
    {
        if (nivelTanque < NIVEL_LLENO)
            mensajeActual = EMERGENCIA_6;
        else
            mensajeActual = EMERGENCIA_7;

        estadoActual = EMERGENCIA;
        //enviarWhatsapp(MENSAJES[mensajeActual]);
        return;
    }

    // Emergencia 8: Sobretemperatura en cámara
    if (temperaturas[2] > TEMP_MAX_CAMARA)
    {
        mensajeActual = EMERGENCIA_8;
        estadoActual = EMERGENCIA;
        //enviarWhatsapp(MENSAJES[mensajeActual]);
        return;
    }
}
