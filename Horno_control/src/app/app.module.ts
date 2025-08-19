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
import { HttpClientModule } from '@angular/common/http';

// Configuración para Arduino IoT Cloud
export const getMqttConfig = (): IMqttServiceOptions => {
  return {
    hostname: 'mqtts-sa.iot.arduino.cc', // Servidor seguro de Arduino
    port: 8884, // Puerto seguro MQTTS
    path: '/',
    clean: true,
    connectTimeout: 4000,
    reconnectPeriod: 4000,
    clientId: 'angular_' + Math.random().toString(16).substring(2, 10),
    username: '', // Se establecerá dinámicamente
    password: '', // Se establecerá dinámicamente
    protocol: 'wss',
    connectOnCreate: false,
    rejectUnauthorized: true // Importante para Arduino Cloud
  };
};

@NgModule({
  declarations: [
    AppComponent
  ],
  imports: [
    BrowserModule,
    BrowserAnimationsModule,
    HttpClientModule, // Necesario para obtener credenciales
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

