#ifndef COMUNICACION_H
#define COMUNICACION_H

#include "config.h"
#include <WiFiClientSecure.h>

// ===== CONFIGURACIÓN SERVIDOR =====
extern const char* ssid;
extern const char* password;
extern const char* serverURL;
extern const char* commandsURL;

// ===== VARIABLES COMUNICACIÓN =====
extern unsigned long lastSendTime;
extern unsigned long lastCommandCheck;
extern int messageCount;

// ===== FUNCIONES COMUNICACIÓN =====
void connectToWiFi();
void checkForCommands();
void sendSystemData();
void handleServerCommunication();

#endif
