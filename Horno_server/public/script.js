class MQTTMonitor {
  constructor() {
    this.logDiv = document.getElementById('logs');
    this.statusElement = document.getElementById('status');
    this.lastUpdateElement = document.getElementById('lastUpdate');
    this.messageCountElement = document.getElementById('messageCount');
    this.clientCountElement = document.getElementById('clientCount');
    this.commandCountElement = document.getElementById('commandCount');
    this.messageCount = 0;
    this.lastMessageTime = null;
    this.systemData = {};
    
    this.init();
  }

  init() {
    this.setupEventSource();
    this.setupClearButton();
    this.updateStats();
    this.loadSystemData();
    
    // Actualizar datos del sistema periódicamente
    setInterval(() => this.loadSystemData(), 3000);
  }

  setupEventSource() {
    try {
      this.evtSource = new EventSource('/events');
      
      this.evtSource.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          
          if (data.type === 'systemData') {
            console.log('📊 Datos del sistema recibidos via SSE:', data.data);
            this.updateSystemDisplay(data.data);
          } else if (data.message) {
            this.addLog(data.message, 'message');
          } else {
            this.addLog(event.data, 'message');
          }
          
          this.messageCount++;
          this.lastMessageTime = new Date();
          this.updateStats();
        } catch (e) {
          // Si no es JSON, tratar como mensaje simple
          console.log('📨 Mensaje SSE recibido:', event.data);
          
          // Intentar extraer datos del sistema de mensajes de log
          if (event.data.includes('horno/data')) {
            this.parseSystemDataFromLog(event.data);
          }
          
          this.addLog(event.data, 'message');
          this.messageCount++;
          this.lastMessageTime = new Date();
          this.updateStats();
        }
      };

      this.evtSource.onerror = (error) => {
        this.addLog('❌ Error de conexión con el servidor', 'error');
        this.statusElement.textContent = 'Desconectado';
        this.statusElement.style.color = '#e74c3c';
      };

      this.addLog('✅ Conectado al servidor de logs', 'connected');
      this.statusElement.textContent = 'Conectado';
      this.statusElement.style.color = '#4ecca3';

    } catch (error) {
      this.addLog('❌ No se pudo conectar al servidor: ' + error.message, 'error');
    }
  }

  parseSystemDataFromLog(logMessage) {
    try {
      // Extraer el mensaje del log
      const messageMatch = logMessage.match(/horno\/data'[^']*'([^']+)'/);
      if (messageMatch && messageMatch[1]) {
        const message = messageMatch[1];
        console.log('📋 Mensaje extraído del log:', message);
        
        // Parsear los parámetros
        const params = new URLSearchParams(message);
        
        // Crear objeto de datos del sistema
        const systemData = {};
        
        // Procesar temperaturas
        if (params.get('temperaturas')) {
          const temps = params.get('temperaturas').replace('[', '').replace(']', '').split(',').map(Number);
          if (temps.length === 4) {
            systemData.temperaturas = temps;
          }
        }
        
        // Procesar niveles
        if (params.get('niveles')) {
          const niveles = params.get('niveles').replace('[', '').replace(']', '').split(',').map(Number);
          if (niveles.length === 3) {
            systemData.niveles = niveles;
          }
        }
        
        // Procesar otros parámetros
        if (params.get('presion')) systemData.presion = parseFloat(params.get('presion'));
        if (params.get('valvula1')) systemData.valvula1 = params.get('valvula1') === 'true';
        if (params.get('valvula2')) systemData.valvula2 = params.get('valvula2') === 'true';
        if (params.get('bomba1')) systemData.bomba1 = params.get('bomba1') === 'true';
        if (params.get('bomba2')) systemData.bomba2 = params.get('bomba2') === 'true';
        if (params.get('estado')) systemData.estado = params.get('estado');
        if (params.get('emergencia')) systemData.emergencia = params.get('emergencia') === 'true';
        if (params.get('bombaActiva')) systemData.bombaActiva = params.get('bombaActiva');
        
        console.log('📊 Datos parseados del log:', systemData);
        this.updateSystemDisplay(systemData);
      }
    } catch (error) {
      console.error('❌ Error parseando datos del log:', error);
    }
  }

  setupClearButton() {
    const clearBtn = document.getElementById('clearBtn');
    clearBtn.addEventListener('click', () => {
      this.clearLogs();
    });
  }

  async loadSystemData() {
    try {
      const response = await fetch('/api/system-data');
      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
      
      const data = await response.json();
      
      if (data.success) {
        console.log('📊 Datos del sistema cargados via API:', data.data);
        this.systemData = data.data;
        this.updateSystemDisplay(this.systemData);
        this.updateStats();
      }
    } catch (error) {
      console.error('❌ Error cargando datos del sistema:', error);
    }
    
    try {
      const response = await fetch('/status');
      if (response.ok) {
        const data = await response.json();
        
        if (data.clients !== undefined) {
          this.clientCountElement.textContent = data.clients;
        }
        
        if (data.pendingCommands !== undefined) {
          this.commandCountElement.textContent = data.pendingCommands;
        }
      }
    } catch (error) {
      console.error('❌ Error cargando estado:', error);
    }
  }

  updateSystemDisplay(data) {
    console.log('🔄 Actualizando dashboard con:', data);
    
    // Actualizar temperaturas
    this.updateSystemItem('temp-tanque', data.temperaturas?.[0], '°C');
    this.updateSystemItem('temp-horno', data.temperaturas?.[1], '°C');
    this.updateSystemItem('temp-camara', data.temperaturas?.[2], '°C');
    this.updateSystemItem('temp-salida', data.temperaturas?.[3], '°C');
    
    // Actualizar niveles
    this.updateSystemItem('nivel-vacio', data.niveles?.[0], '%');
    this.updateSystemItem('nivel-mitad', data.niveles?.[1], '%');
    this.updateSystemItem('nivel-lleno', data.niveles?.[2], '%');
    
    // Actualizar presión
    this.updateSystemItem('presion', data.presion, 'bar');
    
    // Actualizar estados
    this.updateSystemState('estado-sistema', data.estado);
    this.updateSystemState('estado-emergencia', data.emergencia ? 'ACTIVADO' : 'DESACTIVADO');
    this.updateSystemState('valvula1-state', data.valvula1 ? 'ABIERTA' : 'CERRADA');
    this.updateSystemState('valvula2-state', data.valvula2 ? 'ABIERTA' : 'CERRADA');
    this.updateSystemState('bomba1-state', data.bomba1 ? 'ACTIVA' : 'INACTIVA');
    this.updateSystemState('bomba2-state', data.bomba2 ? 'ACTIVA' : 'INACTIVA');
    this.updateSystemState('bomba-activa', data.bombaActiva);
  }

  updateSystemItem(elementId, value, unit) {
    const element = document.getElementById(elementId);
    if (!element) return;
    
    const valueElement = element.querySelector('.system-value');
    if (valueElement) {
      valueElement.innerHTML = `${value !== undefined ? value.toFixed(1) : '--'} <span class="system-unit">${unit}</span>`;
    }
    
    // Actualizar clase según el valor
    element.classList.remove('critical', 'warning', 'normal');
    
    if (value === undefined || value === null) {
      element.classList.add('warning');
    } else {
      // Aplicar colores según valores críticos
      if (elementId.includes('temp-') && value > 80) {
        element.classList.add('critical');
      } else if (elementId.includes('nivel-') && value < 10) {
        element.classList.add('critical');
      } else if (elementId === 'presion' && value > 3.5) {
        element.classList.add('critical');
      } else {
        element.classList.add('normal');
      }
    }
  }

  updateSystemState(elementId, value) {
    const element = document.getElementById(elementId);
    if (!element) return;
    
    const valueElement = element.querySelector('.system-value');
    if (valueElement) {
      valueElement.textContent = value || '--';
    }
    
    // Actualizar clase según el estado
    element.classList.remove('critical', 'warning', 'normal');
    
    if (value === 'EMERGENCIA' || value === 'ACTIVADO' || value === 'ACTIVA') {
      element.classList.add('critical');
    } else if (!value || value === '--') {
      element.classList.add('warning');
    } else {
      element.classList.add('normal');
    }
  }

  addLog(message, type = 'message') {
    const timestamp = new Date().toLocaleTimeString();
    const logEntry = document.createElement('div');
    logEntry.className = `log-message ${type}`;
    logEntry.innerHTML = `
      <span class="timestamp" style="color: #888; margin-right: 10px;">[${timestamp}]</span>
      <span class="message">${this.formatMessage(message)}</span>
    `;
    
    this.logDiv.appendChild(logEntry);
    this.logDiv.scrollTop = this.logDiv.scrollHeight;
  }

  formatMessage(message) {
    // Añadir emojis y formato según el tipo de mensaje
    if (message.includes('conectado')) return `✅ ${message}`;
    if (message.includes('desconectado')) return `❌ ${message}`;
    if (message.includes('Mensaje')) return `📩 ${message}`;
    if (message.includes('Error')) return `⚠️ ${message}`;
    if (message.includes('Publicado')) return `📤 ${message}`;
    if (message.includes('Temperatura')) return `🌡️ ${message}`;
    if (message.includes('Presión')) return `📊 ${message}`;
    if (message.includes('Nivel')) return `💧 ${message}`;
    if (message.includes('EMERGENCIA')) return `🚨 ${message}`;
    return message;
  }

  clearLogs() {
    this.logDiv.innerHTML = '';
    this.messageCount = 0;
    this.updateStats();
    this.addLog('🧹 Logs limpiados', 'message');
  }

  updateStats() {
    this.messageCountElement.textContent = this.messageCount;
    
    if (this.lastMessageTime) {
      this.lastUpdateElement.textContent = this.lastMessageTime.toLocaleTimeString();
    }
  }

  refreshData() {
    this.addLog('🔄 Actualizando datos del sistema...', 'system');
    this.loadSystemData();
  }

  clearAll() {
    this.clearLogs();
    fetch('/api/messages', { method: 'DELETE' })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          this.addLog('🧹 Historial de mensajes limpiado', 'system');
        }
      })
      .catch(error => {
        this.addLog('❌ Error al limpiar mensajes: ' + error.message, 'error');
      });
  }
}

// Inicializar la aplicación cuando el DOM esté listo
document.addEventListener('DOMContentLoaded', () => {
  console.log('🚀 Inicializando monitor del sistema...');
  window.monitor = new MQTTMonitor();
});

// Manejar recarga de página con confirmación
window.addEventListener('beforeunload', (e) => {
  e.preventDefault();
  e.returnValue = '';
});
