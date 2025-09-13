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
        setInterval(() => this.loadSystemData(), 5000);
      }

      setupEventSource() {
        try {
          this.evtSource = new EventSource('/events');
          
          this.evtSource.onmessage = (event) => {
            try {
              const data = JSON.parse(event.data);
              
              if (data.type === 'systemData') {
                this.updateSystemDisplay(data.data);
              } else {
                this.addLog(data.message || event.data, 'message');
              }
              
              this.messageCount++;
              this.lastMessageTime = new Date();
              this.updateStats();
            } catch (e) {
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

      setupClearButton() {
        const clearBtn = document.getElementById('clearBtn');
        clearBtn.addEventListener('click', () => {
          this.clearLogs();
        });
      }

      async loadSystemData() {
        try {
          const response = await fetch('/api/system-data');
          const data = await response.json();
          
          if (data.success) {
            this.systemData = data.data;
            this.updateSystemDisplay(this.systemData);
            this.updateStats();
          }
        } catch (error) {
          console.error('Error loading system data:', error);
        }
        
        try {
          const response = await fetch('/status');
          const data = await response.json();
          
          if (data.clients !== undefined) {
            this.clientCountElement.textContent = data.clients;
          }
          
          if (data.pendingCommands !== undefined) {
            this.commandCountElement.textContent = data.pendingCommands;
          }
        } catch (error) {
          console.error('Error loading status:', error);
        }
      }

      updateSystemDisplay(data) {
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
          element.classList.add('normal');
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
        
        if (value === 'EMERGENCIA' || value === 'ACTIVADO') {
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
      window.monitor = new MQTTMonitor();
    });

    // Manejar recarga de página con confirmación
    window.addEventListener('beforeunload', (e) => {
      e.preventDefault();
      e.returnValue = '';
    });
