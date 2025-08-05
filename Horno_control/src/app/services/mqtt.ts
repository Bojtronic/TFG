import { Injectable } from '@angular/core';
import * as mqtt from 'mqtt'; // Cambio en la importación
import { Observable, Subject } from 'rxjs';
import { MatSnackBar } from '@angular/material/snack-bar';

// Definimos nuestras propias interfaces
export interface MqttMessage {
  topic: string;
  message: string;
  packet?: mqtt.Packet;
}

export interface MqttPublishOptions {
  qos: 0 | 1 | 2;
  retain?: boolean;
  dup?: boolean;
}

export interface MqttConnectionOptions {
  hostname: string;
  port?: number;
  path?: string;
  clean?: boolean;
  connectTimeout?: number;
  reconnectPeriod?: number;
  clientId: string;
  username?: string;
  password?: string;
  protocol?: 'ws' | 'wss' | 'mqtt' | 'mqtts';
}

@Injectable({
  providedIn: 'root'
})
export class Mqtt {
  private client: mqtt.MqttClient | null = null;
  private messageSubject = new Subject<MqttMessage>();
  private connectionStatusSubject = new Subject<boolean>();

  constructor(private snackBar: MatSnackBar) { }

  connect(options: MqttConnectionOptions): void {
    const protocol = options.protocol || 'ws';
    const port = options.port || (protocol === 'wss' ? 443 : 80);
    const url = `${protocol}://${options.hostname}:${port}${options.path || ''}`;

    const mqttOptions: mqtt.IClientOptions = {
      clientId: options.clientId,
      username: options.username,
      password: options.password,
      clean: options.clean,
      connectTimeout: options.connectTimeout,
      reconnectPeriod: options.reconnectPeriod
    };

    this.client = mqtt.connect(url, mqttOptions); // Cambio en el uso de connect

    this.client.on('connect', () => {
      this.connectionStatusSubject.next(true);
      this.snackBar.open('Conexión exitosa!', 'Cerrar', { duration: 3000 });
    });

    this.client.on('message', (topic, message, packet) => {
      this.messageSubject.next({
        topic,
        message: message.toString(),
        packet
      });
    });

    this.client.on('error', (error) => {
      this.connectionStatusSubject.next(false);
      this.snackBar.open(`Error de conexión: ${error.message}`, 'Cerrar', { duration: 5000 });
    });

    this.client.on('close', () => {
      this.connectionStatusSubject.next(false);
    });
  }

  // ... (el resto de los métodos permanecen igual)
  subscribe(topic: string, qos: 0 | 1 | 2 = 0): Observable<string> {
    const result = new Subject<string>();
    
    if (this.client && this.client.connected) {
      this.client.subscribe(topic, { qos }, (error, granted) => {
        if (error) {
          this.snackBar.open(`Error de suscripción: ${error.message}`, 'Cerrar', { duration: 5000 });
          result.error(error);
        } else {
          this.snackBar.open(`Suscrito a ${topic}`, 'Cerrar', { duration: 3000 });
          result.next(topic);
          result.complete();
        }
      });
    } else {
      const error = 'Cliente no conectado';
      this.snackBar.open(error, 'Cerrar', { duration: 5000 });
      result.error(error);
    }

    return result.asObservable();
  }

  unsubscribe(topic: string): Observable<string> {
    const result = new Subject<string>();
    
    if (this.client && this.client.connected) {
      this.client.unsubscribe(topic, (error) => {
        if (error) {
          this.snackBar.open(`Error al cancelar suscripción: ${error.message}`, 'Cerrar', { duration: 5000 });
          result.error(error);
        } else {
          this.snackBar.open(`Suscripción cancelada para ${topic}`, 'Cerrar', { duration: 3000 });
          result.next(topic);
          result.complete();
        }
      });
    } else {
      const error = 'Cliente no conectado';
      this.snackBar.open(error, 'Cerrar', { duration: 5000 });
      result.error(error);
    }

    return result.asObservable();
  }

  publish(topic: string, payload: string, qos: 0 | 1 | 2 = 0, retain: boolean = false): void {
    if (this.client && this.client.connected) {
      const options: MqttPublishOptions = { qos, retain };
      this.client.publish(topic, payload, options, (error) => {
        if (error) {
          this.snackBar.open(`Error al publicar: ${error.message}`, 'Cerrar', { duration: 5000 });
        } else {
          this.snackBar.open(`Mensaje publicado en ${topic}`, 'Cerrar', { duration: 3000 });
        }
      });
    } else {
      this.snackBar.open('Cliente no conectado', 'Cerrar', { duration: 5000 });
    }
  }

  onMessage(): Observable<MqttMessage> {
    return this.messageSubject.asObservable();
  }

  onConnectionStatus(): Observable<boolean> {
    return this.connectionStatusSubject.asObservable();
  }

  disconnect(force: boolean = false): void {
    if (this.client) {
      this.client.end(force, () => {
        this.snackBar.open('Desconexión exitosa!', 'Cerrar', { duration: 3000 });
        this.connectionStatusSubject.next(false);
      });
    }
  }

  isConnected(): boolean {
    return this.client?.connected || false;
  }
}