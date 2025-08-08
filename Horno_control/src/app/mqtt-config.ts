import { IMqttServiceOptions } from 'ngx-mqtt';

export const MQTT_SERVICE_OPTIONS: IMqttServiceOptions = {
  hostname: 'broker.emqx.io',
  port: 8084,
  path: '/mqtt',
  protocol: 'wss',
  clientId: 'mqttx_' + Math.random().toString(16).substring(2, 10), // ID único por sesión
  clean: true,
  connectTimeout: 4000,
  reconnectPeriod: 4000,
  username: '',
  password: ''
};

// Define el token de inyección si es necesario
export const MQTT_SERVICE_OPTIONS_TOKEN = 'MQTT_SERVICE_OPTIONS';