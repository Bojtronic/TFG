import { NgModule, isDevMode } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { AppComponent } from './app.component';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { MatGridListModule } from '@angular/material/grid-list';
import { IMqttServiceOptions, MqttModule } from 'ngx-mqtt';
import { MatCardModule } from "@angular/material/card";
import { MatFormFieldModule } from "@angular/material/form-field";
import { MatInputModule } from "@angular/material/input";
import { MatButtonModule } from "@angular/material/button";
import { MatSelectModule } from "@angular/material/select";
import { FormsModule } from "@angular/forms";
import { MatSnackBarModule } from '@angular/material/snack-bar';

// Configuración dinámica para desarrollo/producción
export const getMqttConfig = (): IMqttServiceOptions => {
  const isHttps = typeof window !== 'undefined' ? 
                 window.location.protocol === 'https:' : 
                 true; // Asume HTTPS en producción

  return {
    hostname: 'broker.emqx.io',
    port: isHttps ? 8084 : 8083, // Usa WSS en producción, WS en desarrollo local
    path: '/mqtt',
    clean: true,
    connectTimeout: 4000,
    reconnectPeriod: 4000,
    clientId: 'mqttx_' + Math.random().toString(16).substring(2, 10),
    username: '',
    password: '',
    protocol: isHttps ? 'wss' : 'ws',
    connectOnCreate: false,
    // Solo para desarrollo (ignora certificados SSL)
    ...(isDevMode() && { 
      rejectUnauthorized: false 
    })
  };
};

@NgModule({
  declarations: [
    AppComponent
  ],
  imports: [
    BrowserModule,
    BrowserAnimationsModule,
    MatCardModule,
    FormsModule,
    MatFormFieldModule,
    MatInputModule,
    MatGridListModule,
    MatButtonModule,
    MatSelectModule,
    MatSnackBarModule,
    MqttModule.forRoot(getMqttConfig())
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }