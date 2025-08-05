// src/app/app.ts
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
  styleUrl: './app.css'
})
export class App implements OnInit, OnDestroy {
  protected readonly title = signal('Horno_control');
  private subscriptions: Subscription[] = [];
  receiveNews = '';
  
  connection = {
    hostname: 'broker.emqx.io',
    port: 8084,
    path: '/mqtt',
    clean: true,
    connectTimeout: 4000,
    reconnectPeriod: 4000,
    clientId: 'mqttx_' + Math.random().toString(16).substring(2, 10),
    username: '',
    password: '',
    protocol: 'ws' as 'ws' | 'wss' | 'mqtt' | 'mqtts'
  };

  subscription = {
    topic: 'Salida/01',
    qos: 0 as 0 | 1 | 2
  };

  publish = {
    topic: 'Entrada/01',
    qos: 0 as 0 | 1 | 2,
    payload: '{ "msg": "Hello, I am browser." }'
  };

  qosList = [
    { label: '0 - A lo sumo una vez', value: 0 },
    { label: '1 - Al menos una vez', value: 1 },
    { label: '2 - Exactamente una vez', value: 2 }
  ];

  isConnected = false;
  subscribeSuccess = false;

  constructor(private mqtt: Mqtt) {}

  ngOnInit(): void {
    this.subscriptions.push(
      this.mqtt.onConnectionStatus().subscribe(status => {
        this.isConnected = status;
      })
    );

    this.subscriptions.push(
      this.mqtt.onMessage().subscribe(packet => {
        this.receiveNews = `${new Date().toLocaleTimeString()}: ${packet.topic} - ${packet.message}\n${this.receiveNews}`;
      })
    );
  }

  createConnection(): void {
    this.mqtt.connect(this.connection);
  }

  doSubscribe(): void {
    this.subscriptions.push(
      this.mqtt.subscribe(this.subscription.topic, this.subscription.qos).subscribe({
        next: () => this.subscribeSuccess = true,
        error: () => this.subscribeSuccess = false
      })
    );
  }

  doUnsubscribe(): void {
    this.subscriptions.push(
      this.mqtt.unsubscribe(this.subscription.topic).subscribe({
        next: () => this.subscribeSuccess = false,
        error: () => this.subscribeSuccess = true
      })
    );
  }

  doPublish(): void {
    this.mqtt.publish(
      this.publish.topic,
      this.publish.payload,
      this.publish.qos
    );
  }

  destroyConnection(): void {
    this.mqtt.disconnect(true);
  }

  ngOnDestroy(): void {
    this.subscriptions.forEach(sub => sub.unsubscribe());
    this.mqtt.disconnect();
  }
}