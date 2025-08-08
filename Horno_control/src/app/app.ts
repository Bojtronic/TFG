import { Component, OnInit, OnDestroy, signal } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { MatCardModule } from '@angular/material/card';
import { MatGridListModule } from '@angular/material/grid-list';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatInputModule } from '@angular/material/input';
import { MatButtonModule } from '@angular/material/button';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MatSelectModule } from '@angular/material/select';
import { MatOptionModule } from '@angular/material/core';
import { MatSnackBarModule } from '@angular/material/snack-bar';
import { Subscription } from 'rxjs';
import { Mqtt } from './services/mqtt';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [
    RouterOutlet, 
    MatCardModule,
    MatGridListModule,
    MatFormFieldModule,
    MatInputModule,
    MatButtonModule,
    CommonModule,
    FormsModule,
    MatSelectModule,
    MatOptionModule,
    MatSnackBarModule
  ],
  templateUrl: './app.html',
  styleUrls: ['./app.css']
})
export class App implements OnInit, OnDestroy {
  protected readonly title = signal('Control de Horno - ESP32');
  private mqttSubscriptions: Subscription[] = [];
  
  // Configuración MQTT (ahora es solo para referencia visual)
  readonly connectionSettings = {
    broker: 'broker.emqx.io',
    port: 8084,
    protocol: 'wss' as const
  };

  // Configuración de suscripción
  subscriptionConfig = {
    topic: 'Salida/01',
    qos: 0 as const
  };

  // Configuración de publicación
  publishConfig = {
    topic: 'Entrada/01',
    qos: 0 as const,
    payload: JSON.stringify({
      device: 'Prueba ESP32',
      command: 'status',
      timestamp: new Date().toISOString()
    })
  };

  // Opciones de QoS para el select
  readonly qosOptions = [
    { label: '0 - At most once', value: 0 },
    { label: '1 - At least once', value: 1 },
    { label: '2 - Exactly once', value: 2 }
  ] as const;

  // Estado de la aplicación
  connectionStatus = false;
  isSubscribed = false;
  receivedMessages: string[] = [];
  readonly maxMessagesToShow = 50;

  constructor(private mqtt: Mqtt) {}

  ngOnInit(): void {
    this.setupMqttListeners();
  }

  private setupMqttListeners(): void {
    // Suscripción al estado de conexión
    this.mqttSubscriptions.push(
      this.mqtt.onConnectionStatus().subscribe({
        next: (status) => this.handleConnectionStatusChange(status),
        error: (err) => console.error('Error en estado de conexión:', err)
      })
    );

    // Suscripción a mensajes entrantes
    this.mqttSubscriptions.push(
      this.mqtt.onMessage().subscribe({
        next: (message) => this.handleIncomingMessage(message),
        error: (err) => console.error('Error en mensajes entrantes:', err)
      })
    );
  }

  private handleConnectionStatusChange(status: boolean): void {
    this.connectionStatus = status;
    if (!status) {
      this.isSubscribed = false;
    }
  }

  private handleIncomingMessage(message: { topic: string; message: string }): void {
    try {
      const timestamp = new Date().toLocaleTimeString();
      const formattedMessage = `${timestamp} [${message.topic}]: ${message.message}`;
      
      // Agregar nuevo mensaje al inicio del array
      this.receivedMessages.unshift(formattedMessage);
      
      // Limitar el número de mensajes mostrados
      if (this.receivedMessages.length > this.maxMessagesToShow) {
        this.receivedMessages.pop();
      }
    } catch (error) {
      console.error('Error procesando mensaje:', error);
    }
  }

  /**
   * Establece conexión con el broker MQTT
   * (La conexión real se maneja automáticamente por ngx-mqtt)
   */
  connect(): void {
    // No necesita implementación ya que la conexión es automática
    // Se mantiene el método por compatibilidad con la interfaz
  }

  /**
   * Desconecta del broker MQTT
   */
  disconnect(): void {
    this.mqtt.disconnect();
  }

  /**
   * Suscribe al topic configurado
   */
  subscribe(): void {
    if (!this.connectionStatus) {
      console.error('No se puede suscribir: no hay conexión MQTT');
      return;
    }

    const sub = this.mqtt.subscribe(
      this.subscriptionConfig.topic, 
      this.subscriptionConfig.qos
    ).subscribe({
      next: () => this.isSubscribed = true,
      error: (err) => {
        this.isSubscribed = false;
        console.error('Error en suscripción:', err);
      }
    });
    
    this.mqttSubscriptions.push(sub);
  }

  /**
   * Cancela la suscripción al topic configurado
   */
  unsubscribe(): void {
    const unsub = this.mqtt.unsubscribe(
      this.subscriptionConfig.topic
    ).subscribe({
      next: () => this.isSubscribed = false,
      error: (err) => console.error('Error al desuscribirse:', err)
    });
    
    this.mqttSubscriptions.push(unsub);
  }

  /**
   * Publica el mensaje configurado
   */
  publish(): void {
    if (!this.connectionStatus) {
      console.error('No se puede publicar: no hay conexión MQTT');
      return;
    }

    try {
      this.mqtt.publish(
        this.publishConfig.topic,
        this.publishConfig.payload,
        this.publishConfig.qos
      );
    } catch (error) {
      console.error('Error al publicar mensaje:', error);
    }
  }

  ngOnDestroy(): void {
    this.cleanupSubscriptions();
    this.disconnect();
  }

  private cleanupSubscriptions(): void {
    this.mqttSubscriptions.forEach(sub => {
      try {
        sub.unsubscribe();
      } catch (err) {
        console.error('Error al limpiar suscripción:', err);
      }
    });
    this.mqttSubscriptions = [];
  }

  /**
   * Limpia los mensajes recibidos
   */
  clearMessages(): void {
    this.receivedMessages = [];
  }
}