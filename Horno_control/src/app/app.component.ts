import { Component, OnDestroy, OnInit } from '@angular/core';
import { IMqttMessage, MqttService, IPublishOptions } from 'ngx-mqtt';
import { MatSnackBar } from '@angular/material/snack-bar';
import { IClientSubscribeOptions } from 'mqtt-browser';
import { Subscription, Subject, takeUntil } from 'rxjs';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit, OnDestroy {
  private destroy$ = new Subject<void>();
  private curSubscription: Subscription | undefined;

  // Configuración MQTT
  connection = {
    hostname: 'broker.emqx.io',
    port: window.location.protocol === 'https:' ? 8084 : 8083,
    path: '/mqtt',
    clean: true,
    connectTimeout: 4000,
    reconnectPeriod: 4000,
    clientId: 'mqttx_' + Math.random().toString(16).substring(2, 10),
    username: '',
    password: '',
    protocol: window.location.protocol === 'https:' ? 'wss' : 'ws'
  };

  subscription = {
    topic: 'Salida/01',
    qos: 0,
  };

  publish = {
    topic: 'Entrada/01',
    qos: 0,
    payload: '0',
  };

  receiveNews = '';
  qosList = [
    { label: 0, value: 0 },
    { label: 1, value: 1 },
    { label: 2, value: 2 },
  ];

  isConnection = false;
  subscribeSuccess = false;

  constructor(
    private mqttService: MqttService,
    private snackBar: MatSnackBar
  ) {}

  ngOnInit(): void {
    this.setupMqttListeners();
  }

  ngOnDestroy(): void {
    this.destroyConnection();
    this.destroy$.next();
    this.destroy$.complete();
  }

  private setupMqttListeners(): void {
    this.mqttService.onConnect
      .pipe(takeUntil(this.destroy$))
      .subscribe(() => {
        this.isConnection = true;
        this.showNotification('Conectado al broker MQTT');
        console.log('Conexión exitosa');
      });

    this.mqttService.onError
      .pipe(takeUntil(this.destroy$))
      .subscribe((error: Error) => {
        this.isConnection = false;
        this.showNotification(`Error de conexión: ${error.message}`, 'error');
        console.error('Error MQTT:', error);
      });

    this.mqttService.onMessage
      .pipe(takeUntil(this.destroy$))
      .subscribe((packet: unknown) => {
        const message = this.parseMqttPacket(packet);
        if (message) {
          this.handleIncomingMessage(message);
        }
      });
  }

  private parseMqttPacket(packet: unknown): IMqttMessage | null {
    // Verificación de tipo segura para IMqttMessage
    if (typeof packet === 'object' && packet !== null) {
      const p = packet as Record<string, unknown>;
      if (p['topic'] && p['payload']) {
        return {
          topic: String(p['topic']),
          payload: p['payload'],
          qos: Number(p['qos']) || 0,
          retain: Boolean(p['retain']),
          dup: Boolean(p['dup'])
        } as IMqttMessage;
      }
    }
    return null;
  }

  private handleIncomingMessage(message: IMqttMessage): void {
    try {
      const payload = typeof message.payload === 'string' 
        ? message.payload 
        : message.payload.toString();
      
      this.receiveNews = `${payload}\n${this.receiveNews}`;
      console.log(`Mensaje recibido [${message.topic}]: ${payload}`);
    } catch (error) {
      console.error('Error procesando mensaje:', error);
    }
  }

  createConnection(): void {
    try {
      this.mqttService.connect();
    } catch (error) {
      this.handleError(error);
    }
  }

  doSubscribe(): void {
    if (!this.isConnection) {
      this.showNotification('Primero establece la conexión', 'warning');
      return;
    }

    this.curSubscription = this.mqttService.observe(
      this.subscription.topic,
      { qos: this.subscription.qos } as IClientSubscribeOptions
    ).subscribe({
      next: (message: IMqttMessage) => {
        this.subscribeSuccess = true;
        this.showNotification(`Mensaje recibido: ${message.payload.toString()}`);
      },
      error: (err) => this.handleError(err)
    });
  }

  doUnSubscribe(): void {
    this.curSubscription?.unsubscribe();
    this.subscribeSuccess = false;
    this.showNotification('Suscripción cancelada');
  }

  doPublish(): void {
    if (!this.isConnection) {
      this.showNotification('No hay conexión activa', 'warning');
      return;
    }

    try {
      const { topic, qos, payload } = this.publish;
      console.log('Publicando en:', topic, 'con payload:', payload);

      // Usando unsafePublish como en el código funcional
      this.mqttService.unsafePublish(topic, payload, { qos } as IPublishOptions);
      
      this.showNotification('Mensaje publicado correctamente');
      console.log('Publicación exitosa en', topic);
      
    } catch (error: unknown) {
      this.handleError(error);
      console.error('Error al publicar:', error);
    }
  }

  destroyConnection(): void {
    try {
      this.mqttService.disconnect(true);
      this.isConnection = false;
      this.curSubscription?.unsubscribe();
      this.showNotification('Desconectado del broker');
    } catch (error) {
      this.handleError(error);
    }
  }

  private showNotification(message: string, panelClass: string = 'success'): void {
    this.snackBar.open(message, 'Cerrar', {
      duration: 5000,
      panelClass: [`snackbar-${panelClass}`]
    });
  }

  private handleError(error: unknown): void {
    const errorMsg = error instanceof Error ? error.message : 'Error desconocido';
    console.error('Error:', error);
    this.showNotification(errorMsg, 'error');
  }
}