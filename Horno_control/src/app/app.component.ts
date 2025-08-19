import { Component, OnDestroy, OnInit } from '@angular/core';
import { IMqttMessage, MqttService, IPublishOptions, IMqttServiceOptions } from 'ngx-mqtt';
import { MatSnackBar } from '@angular/material/snack-bar';
import { IClientSubscribeOptions } from 'mqtt-browser';
import { Subscription, Subject, takeUntil } from 'rxjs';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import ArduinoIotClient from '@arduino/arduino-iot-client';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})

export class AppComponent implements OnInit, OnDestroy {
  private destroy$ = new Subject<void>();
  private curSubscription: Subscription | undefined;
  private accessToken = '';

  private arduinoClient: any;
  private devicesApi: any;
  
  // Tus credenciales (deberías usar environment variables en producción)
  private readonly deviceId = 'f70e133e-27b0-4b96-87af-769e588d64f5';
  private readonly thingId = 'b47152d2-46b8-4bcf-9cd3-2434f5d91b2d';
  private readonly clientId = 'TU_CLIENT_ID'; // Obtener de Arduino Cloud
  private readonly clientSecret = 'TU_CLIENT_SECRET'; // Obtener de Arduino Cloud
  private readonly deviceKey = 'SECRET_DEVICE_KEY'; // De tu configuración

  // Configuración MQTT actualizada con tipo correcto
  connection: IMqttServiceOptions = {
    hostname: 'mqtts-sa.iot.arduino.cc',
    port: 8884,
    path: '/',
    clean: true,
    connectTimeout: 4000,
    reconnectPeriod: 4000,
    clientId: 'angular_' + Math.random().toString(16).substring(2, 10),
    username: '', // Se establecerá después
    password: '', // Se establecerá después
    protocol: 'wss' as const, // Tipo correcto para IMqttServiceOptions
    rejectUnauthorized: true
  };

  // Topics para Arduino Cloud
  subscription = {
    topic: `/a/t/${this.thingId}/s/${this.deviceId}/property/+`,
    qos: 0,
  };

  publish = {
    topic: `/a/t/${this.thingId}/s/${this.deviceId}/property/update`,
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
    private snackBar: MatSnackBar,
    private http: HttpClient
  ) {
    this.initializeArduinoClient();
  }

  private initializeArduinoClient(): void {
    this.arduinoClient = ArduinoIotClient.ApiClient.instance;
    this.devicesApi = new ArduinoIotClient.DevicesV2Api(this.arduinoClient);
  }

  async ngOnInit(): Promise<void> {
    await this.getAuthToken();
    await this.setupArduinoCloudConnection();
    this.setupMqttListeners();
  }

   private async setupArduinoCloudConnection(): Promise<void> {
    try {
      // Ejemplo de uso del cliente Arduino IoT
      const devices = await this.devicesApi.devicesV2List();
      console.log('Dispositivos disponibles:', devices);
    } catch (error) {
      console.error('Error obteniendo dispositivos:', error);
      this.handleError(error);
    }
  }

  private async getAuthToken(): Promise<void> {
    const body = new URLSearchParams();
    body.set('grant_type', 'client_credentials');
    body.set('client_id', this.clientId);
    body.set('client_secret', this.clientSecret);
    body.set('audience', 'https://api2.arduino.cc/iot');

    const headers = new HttpHeaders()
      .set('Content-Type', 'application/x-www-form-urlencoded');

    try {
      const response: any = await this.http.post(
        'https://api2.arduino.cc/iot/v1/clients/token',
        body.toString(),
        { headers }
      ).toPromise();

      this.accessToken = response.access_token;
      this.arduinoClient.authentications['oauth2'].accessToken = this.accessToken;
    } catch (error) {
      console.error('Error obteniendo token:', error);
      throw error;
    }
  }


  ngOnDestroy(): void {
    this.destroyConnection();
    this.destroy$.next();
    this.destroy$.complete();
  }

  private async initializeConnection(): Promise<void> {
    try {
      await this.getAuthToken();
      await this.getMqttCredentials();
    } catch (error) {
      this.showNotification('Error inicializando conexión', 'error');
      console.error('Error inicializando:', error);
    }
  }

  private async getMqttCredentials(): Promise<void> {
    const headers = new HttpHeaders()
      .set('Authorization', `Bearer ${this.accessToken}`);

    try {
      const response: any = await this.http.get(
        `https://api2.arduino.cc/iot/v2/devices/${this.deviceId}/mqtt-config`,
        { headers }
      ).toPromise();

      this.connection.username = response.username;
      this.connection.password = response.password;
    } catch (error) {
      console.error('Error obteniendo credenciales MQTT:', error);
      throw error;
    }
  }

  private setupMqttListeners(): void {
    this.mqttService.onConnect
      .pipe(takeUntil(this.destroy$))
      .subscribe(() => {
        this.isConnection = true;
        this.showNotification('Conectado a Arduino IoT Cloud');
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
      this.showNotification('Error procesando mensaje', 'error');
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

  createConnection(): void {
    try {
      this.mqttService.connect(this.connection);
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

      const arduinoPayload = {
        value: payload
      };

      this.mqttService.unsafePublish(
        topic, 
        JSON.stringify(arduinoPayload), 
        { qos } as IPublishOptions
      );
      
      this.showNotification('Mensaje publicado correctamente');
    } catch (error: unknown) {
      this.handleError(error);
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
}
