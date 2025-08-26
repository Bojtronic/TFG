import { Component, OnDestroy, OnInit } from '@angular/core';
import { IMqttMessage, MqttService, IPublishOptions } from 'ngx-mqtt';
import { MatSnackBar } from '@angular/material/snack-bar';
import { Subscription, Subject, takeUntil } from 'rxjs';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { ThemePalette } from '@angular/material/core';

interface ArduinoCloudConfig {
  clientId: string;
  clientSecret: string;
  deviceId: string;
  thingId: string;
}

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit, OnDestroy {

  lastUpdate: Date = new Date();

  private destroy$ = new Subject<void>();
  private accessToken = '';
  private mqttSubscription: Subscription | undefined;

  // Configuración - RELLENA ESTOS DATOS
  readonly config: ArduinoCloudConfig = {
    clientId: 'se obtiene de generar un API KEY en Arduino Cloud',
    clientSecret: 'se obtiene de generar un API KEY en Arduino Cloud',
    deviceId: 'f70e133e-27b0-4b96-87af-769e588d64f5',
    thingId: 'b47152d2-46b8-4bcf-9cd3-2434f5d91b2d'
  };

  // Variables para la UI
  isConnected = false;
  isLoading = false;
  fotoresistorValue: number = 0;
  ledStatus: boolean = false;

  // Topics para Arduino Cloud
  private readonly topics = {
    fotoresistor: `/a/t/${this.config.thingId}/s/${this.config.deviceId}/property/fotoresistor`,
    led: `/a/t/${this.config.thingId}/s/${this.config.deviceId}/property/lED`,
    ledUpdate: `/a/t/${this.config.thingId}/s/${this.config.deviceId}/property/update`
  };

  constructor(
    private mqttService: MqttService,
    private snackBar: MatSnackBar,
    private http: HttpClient
  ) {}

  async ngOnInit(): Promise<void> {
    await this.initializeConnection();
  }

  ngOnDestroy(): void {
    this.disconnect();
    this.destroy$.next();
    this.destroy$.complete();
  }

  async initializeConnection(): Promise<void> {
    this.isLoading = true;
    try {
      await this.getAuthToken();
      await this.getMqttCredentials();
      this.setupMqttListeners();
    } catch (error) {
      this.showNotification('Error inicializando conexión', 'error');
      console.error('Error inicializando:', error);
    } finally {
      this.isLoading = false;
    }
  }

  private async getAuthToken(): Promise<void> {
    const body = new URLSearchParams();
    body.set('grant_type', 'client_credentials');
    body.set('client_id', this.config.clientId);
    body.set('client_secret', this.config.clientSecret);
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
      console.log('Token obtenido correctamente');
    } catch (error) {
      console.error('Error obteniendo token:', error);
      throw error;
    }
  }

  private async getMqttCredentials(): Promise<void> {
    const headers = new HttpHeaders()
      .set('Authorization', `Bearer ${this.accessToken}`);

    try {
      const response: any = await this.http.get(
        `https://api2.arduino.cc/iot/v2/devices/${this.config.deviceId}/mqtt-config`,
        { headers }
      ).toPromise();

      console.log('Credenciales MQTT obtenidas (pero no se usan en conexión estática)');
      console.log('Username:', response.username);
      console.log('Password:', '••••••••'); // No mostrar password real

    } catch (error) {
      console.error('Error obteniendo credenciales MQTT:', error);
      throw error;
    }
  }

  private setupMqttListeners(): void {
    // Escuchar eventos de conexión
    this.mqttService.onConnect
      .pipe(takeUntil(this.destroy$))
      .subscribe(() => {
        this.isConnected = true;
        this.showNotification('Conectado a Arduino IoT Cloud');
        this.subscribeToTopics();
      });

    this.mqttService.onError
      .pipe(takeUntil(this.destroy$))
      .subscribe((error: any) => {
        this.isConnected = false;
        this.showNotification(`Error de conexión: ${error.message}`, 'error');
        console.error('Error MQTT:', error);
      });

    // Manejar mensajes entrantes
    this.mqttService.onMessage
      .pipe(takeUntil(this.destroy$))
      .subscribe((packet: any) => {
        this.handleMqttPacket(packet);
      });
  }

  private handleMqttPacket(packet: any): void {
    try {
      // Convertir el paquete MQTT a IMqttMessage
      const message: IMqttMessage = {
        topic: packet.topic,
        payload: packet.payload,
        qos: packet.qos || 0,
        retain: packet.retain || false,
        dup: packet.dup || false
      };
      
      this.handleIncomingMessage(message);
    } catch (error) {
      console.error('Error procesando paquete MQTT:', error);
    }
  }

  private subscribeToTopics(): void {
    // Suscribirse a los topics de las propiedades
    this.mqttSubscription = this.mqttService.observe(this.topics.fotoresistor)
      .pipe(takeUntil(this.destroy$))
      .subscribe((message: IMqttMessage) => {
        this.handleFotoresistorUpdate(message);
      });

    this.mqttService.observe(this.topics.led)
      .pipe(takeUntil(this.destroy$))
      .subscribe((message: IMqttMessage) => {
        this.handleLedUpdate(message);
      });

    console.log('Suscripciones configuradas');
  }

  private handleIncomingMessage(message: IMqttMessage): void {
    try {
      const payload = this.parsePayload(message.payload);
      console.log(`Mensaje recibido [${message.topic}]:`, payload);
    } catch (error) {
      console.error('Error procesando mensaje:', error);
    }
  }

  private handleFotoresistorUpdate(message: IMqttMessage): void {
    try {
      const payload = this.parsePayload(message.payload);
      this.fotoresistorValue = payload.value;
      this.lastUpdate = new Date(); 
      console.log('Fotoresistor actualizado:', this.fotoresistorValue);
    } catch (error) {
      console.error('Error procesando fotoresistor:', error);
    }
  }

  private handleLedUpdate(message: IMqttMessage): void {
    try {
      const payload = this.parsePayload(message.payload);
      this.ledStatus = payload.value;
      console.log('LED actualizado:', this.ledStatus);
    } catch (error) {
      console.error('Error procesando LED:', error);
    }
  }

  private parsePayload(payload: any): any {
    try {
      if (typeof payload === 'string') {
        return JSON.parse(payload);
      } else if (payload instanceof ArrayBuffer) {
        return JSON.parse(new TextDecoder().decode(payload));
      } else {
        return JSON.parse(payload.toString());
      }
    } catch (error) {
      console.error('Error parsing payload:', error, payload);
      return {};
    }
  }

  // Método para controlar el LED
  toggleLed(): void {
    if (!this.isConnected) {
      this.showNotification('No hay conexión activa', 'warning');
      return;
    }

    const newLedStatus = !this.ledStatus;
    const payload = {
      value: newLedStatus
    };

    try {
      this.mqttService.unsafePublish(
        this.topics.ledUpdate,
        JSON.stringify(payload),
        { qos: 1 } as IPublishOptions
      );
      
      this.showNotification(`LED ${newLedStatus ? 'encendido' : 'apagado'}`);
    } catch (error) {
      this.showNotification('Error controlando LED', 'error');
      console.error('Error publicando:', error);
    }
  }

  // Método para forzar actualización de valores
  refreshValues(): void {
    this.showNotification('Sincronizando valores...');
  }

  private showNotification(message: string, panelClass: string = 'success'): void {
    this.snackBar.open(message, 'Cerrar', {
      duration: 3000,
      panelClass: [`snackbar-${panelClass}`]
    });
  }

  disconnect(): void {
    if (this.mqttSubscription) {
      this.mqttSubscription.unsubscribe();
    }
    this.mqttService.disconnect();
    this.isConnected = false;
    this.showNotification('Desconectado de Arduino Cloud');
  }

  getSensorPercentage(): number {
    return Math.min(100, (this.fotoresistorValue / 4095) * 100);
  }

  getSensorColor(): ThemePalette {
    const percentage = this.getSensorPercentage();
    if (percentage < 25) return 'primary';
    if (percentage < 50) return 'accent';
    if (percentage < 75) return 'warn';
    return 'warn';
  }
}