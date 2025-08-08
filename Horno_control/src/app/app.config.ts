import { ApplicationConfig, InjectionToken } from '@angular/core';
import { provideAnimations } from '@angular/platform-browser/animations';
import { provideRouter } from '@angular/router';
import { routes } from './app.routes';
import { provideHttpClient } from '@angular/common/http';
import { MAT_SNACK_BAR_DEFAULT_OPTIONS } from '@angular/material/snack-bar';
import { MqttService } from 'ngx-mqtt';
import { IMqttServiceOptions } from 'ngx-mqtt';

// Define el token de inyección que espera ngx-mqtt
export const MQTT_SERVICE_OPTIONS = new InjectionToken<IMqttServiceOptions>('NgxMqttServiceConfig');

export const appConfig: ApplicationConfig = {
  providers: [
    provideRouter(routes),
    provideAnimations(),
    provideHttpClient(),
    { provide: MAT_SNACK_BAR_DEFAULT_OPTIONS, useValue: { duration: 3000 } },
    
    // Configuración MQTT
    { 
      provide: MQTT_SERVICE_OPTIONS,
      useValue: {
        hostname: 'broker.emqx.io',
        port: 8084,
        path: '/mqtt',
        protocol: 'wss',
        clientId: 'mqttx_' + Math.random().toString(16).substring(2, 10),
        clean: true,
        connectTimeout: 4000,
        reconnectPeriod: 4000,
        username: '',
        password: ''
      } as IMqttServiceOptions
    },
    MqttService
  ]
};