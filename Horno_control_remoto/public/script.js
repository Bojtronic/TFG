class HornoBiomasaControl {
  constructor() {
    this.isConnected = false;
    this.sensorData = {
      tempTanque: 0,
      tempHorno: 0,
      tempCamara: 0,
      tempSalida: 0,
      nivelVacio: 0,
      nivelMitad: 0,
      nivelLleno: 0,
      presion: 0,
      valvula1: false,
      valvula2: false,
      bomba1: false,
      bomba2: false,
      estadoSistema: 'SISTEMA_APAGADO',
      emergencia: false,
      bombaActiva: 'PRINCIPAL'
    };
    this.init();
  }

  async init() {
    this.updateDisplay();
    await this.checkBrokerStatus();
    this.connectToEventStream();
    this.startAutoRefresh();

    // Verificar estado del broker periódicamente
    setInterval(() => this.checkBrokerStatus(), 30000);
  }

  async checkBrokerStatus() {
    try {
      const response = await fetch('/api/broker-status');
      const data = await response.json();

      const statusElement = document.getElementById('broker-status');
      if (data.success) {
        statusElement.textContent = 'Broker: Conectado';
        this.updateConnectionStatus(true);
      } else {
        statusElement.textContent = 'Broker: Desconectado';
        this.updateConnectionStatus(false);
      }
    } catch (error) {
      document.getElementById('broker-status').textContent = 'Broker: Error';
      this.updateConnectionStatus(false);
    }
  }

  connectToEventStream() {
    try {
      this.eventSource = new EventSource('/api/events');

      this.eventSource.onmessage = (event) => {
        this.processEvent(JSON.parse(event.data));
      };

      this.eventSource.onerror = (error) => {
        console.log('Error en conexión SSE, reconectando...');
        this.updateConnectionStatus(false);
        setTimeout(() => this.connectToEventStream(), 5000);
      };

      this.updateConnectionStatus(true);
      console.log('Conectado al stream de eventos');

    } catch (error) {
      console.error('Error connecting to SSE:', error);
      setTimeout(() => this.connectToEventStream(), 5000);
    }
  }

  processEvent(eventData) {
    console.log('Evento recibido:', eventData);

    // Actualizar datos según el evento recibido
    if (eventData.temperaturas) {
      this.sensorData.tempTanque = eventData.temperaturas[0] || 0;
      this.sensorData.tempHorno = eventData.temperaturas[1] || 0;
      this.sensorData.tempCamara = eventData.temperaturas[2] || 0;
      this.sensorData.tempSalida = eventData.temperaturas[3] || 0;
    }

    if (eventData.niveles) {
      this.sensorData.nivelVacio = eventData.niveles[0] || 0;
      this.sensorData.nivelMitad = eventData.niveles[1] || 0;
      this.sensorData.nivelLleno = eventData.niveles[2] || 0;
    }

    if (eventData.presion !== undefined) {
      this.sensorData.presion = eventData.presion || 0;
    }

    if (eventData.estado !== undefined) {
      this.sensorData.estadoSistema = eventData.estado || 'SISTEMA_APAGADO';
    }

    if (eventData.emergencia !== undefined) {
      this.sensorData.emergencia = eventData.emergencia || false;
    }

    if (eventData.valvula1 !== undefined) {
      this.sensorData.valvula1 = eventData.valvula1 || false;
    }

    if (eventData.valvula2 !== undefined) {
      this.sensorData.valvula2 = eventData.valvula2 || false;
    }

    if (eventData.bomba1 !== undefined) {
      this.sensorData.bomba1 = eventData.bomba1 || false;
    }

    if (eventData.bomba2 !== undefined) {
      this.sensorData.bomba2 = eventData.bomba2 || false;
    }

    if (eventData.bombaActiva !== undefined) {
      this.sensorData.bombaActiva = eventData.bombaActiva || 'PRINCIPAL';
    }

    if (eventData.mensaje) {
      this.addMessage(eventData.mensaje);
    }

    this.updateDisplay();
  }

  async sendCommand(command) {
    try {
      console.log(`Enviando comando: ${command}`);

      const response = await fetch('/api/send-message', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          topic: 'horno/control',
          message: command
        })
      });

      const data = await response.json();

      if (data.success) {
        console.log('Comando enviado exitosamente:', command);
        this.addMessage(`Comando enviado: ${command}`);
      } else {
        console.error('Error enviando comando:', data.message);
        this.addMessage(`Error: ${data.message}`);
      }

    } catch (error) {
      console.error('Error de conexión:', error.message);
      this.updateConnectionStatus(false);
      this.addMessage('Error de conexión al enviar comando');
    }
  }

  updateDisplay() {
    // Actualizar temperaturas
    this.updateSensorDisplay('temp-tanque', this.sensorData.tempTanque, '°C', 85);
    this.updateSensorDisplay('temp-horno', this.sensorData.tempHorno, '°C', 250);
    this.updateSensorDisplay('temp-camara', this.sensorData.tempCamara, '°C', 150);
    this.updateSensorDisplay('temp-salida', this.sensorData.tempSalida, '°C', 85);

    // Actualizar niveles
    this.updateSensorDisplay('nivel-vacio', this.sensorData.nivelVacio, '%', 10, true);
    this.updateSensorDisplay('nivel-mitad', this.sensorData.nivelMitad, '%', 40, true);
    this.updateSensorDisplay('nivel-lleno', this.sensorData.nivelLleno, '%', 80, true);

    // Actualizar presión
    this.updateSensorDisplay('presion', this.sensorData.presion, 'bar', 3.5);

    // Actualizar estados de actuadores
    this.updateSwitch('valv1-switch', this.sensorData.valvula1);
    this.updateSwitch('valv2-switch', this.sensorData.valvula2);
    this.updateSwitch('bomba1-switch', this.sensorData.bomba1);
    this.updateSwitch('bomba2-switch', this.sensorData.bomba2);

    document.getElementById('valv1-state').textContent = this.sensorData.valvula1 ? 'Abierta' : 'Cerrada';
    document.getElementById('valv2-state').textContent = this.sensorData.valvula2 ? 'Abierta' : 'Cerrada';
    document.getElementById('bomba1-state').textContent = this.sensorData.bomba1 ? 'Encendida' : 'Apagada';
    document.getElementById('bomba2-state').textContent = this.sensorData.bomba2 ? 'Encendida' : 'Apagada';

    // Actualizar estado del sistema
    const systemState = document.getElementById('system-state');
    systemState.textContent = this.getEstadoTexto(this.sensorData.estadoSistema);
    systemState.className = 'status-value ' + this.getEstadoClase(this.sensorData.estadoSistema);

    // Actualizar estado de emergencia
    const emergencyState = document.getElementById('emergency-state');
    emergencyState.textContent = this.sensorData.emergencia ? 'ACTIVO' : 'INACTIVO';
    emergencyState.className = 'status-value ' + (this.sensorData.emergencia ? 'status-emergencia' : 'status-apagado');

    // Actualizar bomba activa
    document.getElementById('active-pump').textContent = this.sensorData.bombaActiva;

    // Actualizar última actualización
    document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
  }

  updateSensorDisplay(elementId, value, unit, warningThreshold, isLevel = false) {
    const element = document.getElementById(elementId);
    if (!element) return;

    const valueElement = element.querySelector('.sensor-value');
    if (valueElement) {
      valueElement.innerHTML = `${value.toFixed(1)} <span class="sensor-unit">${unit}</span>`;
    }

    // Actualizar clase según el estado
    element.classList.remove('critical', 'warning', 'normal');

    if (isLevel) {
      // Para niveles, valores bajos son críticos
      if (value < warningThreshold) {
        element.classList.add('critical');
      } else {
        element.classList.add('normal');
      }
    } else {
      // Para otros sensores, valores altos son críticos
      if (value > warningThreshold) {
        element.classList.add('critical');
      } else if (value > warningThreshold * 0.8) {
        element.classList.add('warning');
      } else {
        element.classList.add('normal');
      }
    }
  }

  updateSwitch(switchId, isOn) {
    const switchElement = document.getElementById(switchId);
    if (switchElement) {
      switchElement.checked = isOn;
    }
  }

  getEstadoTexto(estado) {
    const estados = {
      'SISTEMA_APAGADO': 'APAGADO',
      'LLENADO_TANQUE': 'LLENANDO',
      'CALENTAMIENTO': 'CALENTANDO',
      'CIRCULACION': 'CIRCULANDO',
      'ENTREGA_AGUA': 'ENTREGANDO',
      'EMERGENCIA': 'EMERGENCIA',
      'MANTENIMIENTO': 'MANTENIMIENTO'
    };
    return estados[estado] || estado;
  }

  getEstadoClase(estado) {
    const clases = {
      'SISTEMA_APAGADO': 'status-apagado',
      'LLENADO_TANQUE': 'status-llenando',
      'CALENTAMIENTO': 'status-calentando',
      'CIRCULACION': 'status-circulando',
      'ENTREGA_AGUA': 'status-entregando',
      'EMERGENCIA': 'status-emergencia',
      'MANTENIMIENTO': 'status-apagado'
    };
    return clases[estado] || 'status-apagado';
  }

  updateConnectionStatus(connected) {
    const statusElement = document.getElementById('connection-status');
    if (!statusElement) return;

    this.isConnected = connected;

    if (connected) {
      statusElement.innerHTML = '<i class="fas fa-plug"></i> Conectado';
      statusElement.className = 'status-online';
    } else {
      statusElement.innerHTML = '<i class="fas fa-plug"></i> Desconectado';
      statusElement.className = 'status-offline';
    }
  }

  addMessage(message) {
    const logElement = document.getElementById('message-log');
    if (!logElement) return;

    const logEntry = document.createElement('div');
    logEntry.className = 'log-entry';
    logEntry.innerHTML = `<span class="log-time">${new Date().toLocaleTimeString()}</span> ${message}`;

    logElement.prepend(logEntry);

    // Limitar a 20 mensajes
    if (logElement.children.length > 20) {
      logElement.removeChild(logElement.lastChild);
    }
  }

  startAutoRefresh() {
    // Actualizar visualización cada segundo
    setInterval(() => {
      this.updateDisplay();
    }, 1000);
  }
}

// Funciones globales para interactuar con los controles
function toggleValve(valveNum) {
  if (window.hornoControl) {
    const switchElement = document.getElementById(`valv${valveNum}-switch`);
    if (!switchElement) return;

    const command = switchElement.checked ? `valv${valveNum}_on` : `valv${valveNum}_off`;
    window.hornoControl.sendCommand(command);
  }
}

function toggleBomba(bombaNum) {
  if (window.hornoControl) {
    const switchElement = document.getElementById(`bomba${bombaNum}-switch`);
    if (!switchElement) return;

    const command = switchElement.checked ? `bomba${bombaNum}_on` : `bomba${bombaNum}_off`;
    window.hornoControl.sendCommand(command);
  }
}

function sendCommand(command) {
  if (window.hornoControl) {
    window.hornoControl.sendCommand(command);
  }
}

// Inicializar la aplicación cuando el DOM esté listo
document.addEventListener('DOMContentLoaded', () => {
  console.log('Inicializando controlador Horno de Biomasa...');
  window.hornoControl = new HornoBiomasaControl();
});

// Manejar recarga de página
window.addEventListener('beforeunload', () => {
  if (window.hornoControl && window.hornoControl.eventSource) {
    window.hornoControl.eventSource.close();
  }
});