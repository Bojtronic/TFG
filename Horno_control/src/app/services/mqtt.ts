import { Injectable } from '@angular/core';
import { IMqttMessage, MqttService, IOnConnectEvent, IOnErrorEvent } from 'ngx-mqtt';
import { Observable, Subject, BehaviorSubject } from 'rxjs';
import { MatSnackBar } from '@angular/material/snack-bar';

export interface MqttMessage {
  topic: string;
  message: string;
}

@Injectable({
  providedIn: 'root'
})
export class Mqtt {
  private messageSubject = new Subject<MqttMessage>();
  private connectionStatusSubject = new BehaviorSubject<boolean>(false);
  private currentConnectionStatus = false;

  constructor(
    private mqttService: MqttService,
    private snackBar: MatSnackBar
  ) {
    this.setupConnectionListeners();
  }

  /**
   * Suscribe a un topic MQTT
   * @param topic - Topic al que suscribirse
   * @param qos - Nivel de Calidad de Servicio (0, 1 o 2)
   * @returns Observable que emite el topic cuando la suscripción es exitosa
   */
  subscribe(topic: string, qos: 0 | 1 | 2 = 0): Observable<string> {
    return new Observable<string>(subscriber => {
      const subscription = this.mqttService.observe(topic).subscribe({
        next: (message: IMqttMessage) => {
          this.messageSubject.next({
            topic: message.topic,
            message: message.payload.toString()
          });
        },
        error: (err: Error) => {
          this.showSnackbar(`Error de suscripción: ${err.message}`, 5000);
          subscriber.error(err);
        }
      });

      subscriber.next(topic);
      subscriber.complete();

      return () => {
        subscription.unsubscribe();
      };
    });
  }

  /**
   * Cancela la suscripción a un topic
   * @param topic - Topic del que desuscribirse
   * @returns Observable que emite el topic cuando la desuscripción es exitosa
   */
  unsubscribe(topic: string): Observable<string> {
    return new Observable<string>(subscriber => {
      try {
        subscriber.next(topic);
        subscriber.complete();
        this.showSnackbar(`Desuscrito de ${topic}`);
      } catch (error) {
        const err = error instanceof Error ? error : new Error('Error desconocido al desuscribirse');
        this.showSnackbar(err.message, 5000);
        subscriber.error(err);
      }
      return () => {}; // Función de limpieza vacía
    });
  }

  /**
   * Publica un mensaje MQTT
   * @param topic - Topic de destino
   * @param payload - Contenido del mensaje
   * @param qos - Nivel de Calidad de Servicio
   * @param retain - Si el mensaje debe ser retenido por el broker
   */
  publish(topic: string, payload: string, qos: 0 | 1 | 2 = 0, retain: boolean = false): void {
    try {
      this.mqttService.unsafePublish(topic, payload, { qos, retain });
      this.showSnackbar(`Mensaje publicado en ${topic}`);
    } catch (error) {
      const err = error instanceof Error ? error : new Error('Error desconocido al publicar');
      this.showSnackbar(`Error al publicar: ${err.message}`, 5000);
    }
  }

  /**
   * Obtiene un observable de mensajes recibidos
   * @returns Observable de mensajes MQTT
   */
  onMessage(): Observable<MqttMessage> {
    return this.messageSubject.asObservable();
  }

  /**
   * Obtiene un observable del estado de conexión
   * @returns Observable que emite el estado de conexión (true/false)
   */
  onConnectionStatus(): Observable<boolean> {
    return this.connectionStatusSubject.asObservable();
  }

  /**
   * Desconecta del broker MQTT
   */
  disconnect(): void {
    this.mqttService.disconnect();
    this.currentConnectionStatus = false;
    this.connectionStatusSubject.next(false);
    this.showSnackbar('Desconectado del broker MQTT');
  }

  /**
   * Verifica el estado de conexión actual
   * @returns true si está conectado, false en caso contrario
   */
  isConnected(): boolean {
    return this.currentConnectionStatus;
  }

  // ==================== MÉTODOS PRIVADOS ====================

  private setupConnectionListeners(): void {
    this.mqttService.onConnect.subscribe((event: IOnConnectEvent) => {
      this.updateConnectionStatus(true, 'Conexión MQTT establecida');
    });

    this.mqttService.onError.subscribe((event: IOnErrorEvent) => {
      const errorMsg = this.getErrorMessage(event);
      this.updateConnectionStatus(false, errorMsg);
    });

    this.mqttService.onClose.subscribe(() => {
      this.updateConnectionStatus(false, 'Conexión MQTT cerrada');
    });

    this.mqttService.onReconnect.subscribe(() => {
      this.updateConnectionStatus(true, 'Reconectado al broker MQTT');
    });

    this.mqttService.onOffline.subscribe(() => {
      this.updateConnectionStatus(false, 'Conexión MQTT offline');
    });
  }

  private getErrorMessage(event: IOnErrorEvent): string {
    if (event.type === 'close') {
      return 'Conexión cerrada por el servidor';
    }
    return `Error MQTT (${event.type}): ${event.message || 'Sin detalles'}`;
  }

  private updateConnectionStatus(status: boolean, message?: string): void {
    if (this.currentConnectionStatus !== status) {
      this.currentConnectionStatus = status;
      this.connectionStatusSubject.next(status);
      if (message) {
        this.showSnackbar(message);
      }
    }
  }

  private showSnackbar(message: string, duration: number = 3000): void {
    this.snackBar.open(message, 'Cerrar', { 
      duration,
      panelClass: ['mqtt-snackbar']
    });
  }
}