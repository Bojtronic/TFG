// script.js - Cliente Web para Control del Horno de Biomasa

// Configuración
const BROKER_URL = window.location.hostname;
const BROKER_PORT = window.location.port || (window.location.protocol === 'https:' ? 443 : 80);
const WS_PROTOCOL = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
const WS_URL = `${WS_PROTOCOL}//${BROKER_URL}:${BROKER_PORT}/simple`;
const API_BASE_URL = `${window.location.protocol}//${BROKER_URL}:${BROKER_PORT}`;

// Estado de la aplicación
let appState = {
  connected: false,
  ws: null,
  reconnectAttempts: 0,
  maxReconnectAttempts: 10,
  reconnectInterval: 3000,
  systemData: {
    temperaturas: [0, 0, 0, 0],
    niveles: [0, 0, 0],
    presion: 0,
    valvula1: false,
    valvula2: false,
    bomba1: false,
    bomba2: false,
    estado: 'SISTEMA_APAGADO',
    emergencia: false,
    bombaActiva: 'PRINCIPAL',
    lastUpdate: new Date().toISOString()
  }
};

// Elementos de la interfaz
const elements = {
  connectionStatus: document.getElementById('connection-status'),
  brokerStatus: document.getElementById('broker-status'),
  messageLog: document.getElementById('message-log'),
  
  // Temperaturas
  tempTanque: document.getElementById('temp-tanque'),
  tempHorno: document.getElementById('temp-horno'),
  tempCamara: document.getElementById('temp-camara'),
  tempSalida: document.getElementById('temp-salida'),
  
  // Niveles y presión
  nivelVacio: document.getElementById('nivel-vacio'),
  nivelMitad: document.getElementById('nivel-mitad'),
  nivelLleno: document.getElementById('nivel-lleno'),
  presion: document.getElementById('presion'),
  
  // Estados de actuadores
  valv1State: document.getElementById('valv1-state'),
  valv2State: document.getElementById('valv2-state'),
  bomba1State: document.getElementById('bomba1-state'),
  bomba2State: document.getElementById('bomba2-state'),
  
  // Switches de control
  valv1Switch: document.getElementById('valv1-switch'),
  valv2Switch: document.getElementById('valv2-switch'),
  bomba1Switch: document.getElementById('bomba1-switch'),
  bomba2Switch: document.getElementById('bomba2-switch'),
  
  // Estado del sistema
  systemState: document.getElementById('system-state'),
  emergencyState: document.getElementById('emergency-state'),
  activePump: document.getElementById('active-pump'),
  lastUpdate: document.getElementById('last-update')
};

// Inicialización
document.addEventListener('DOMContentLoaded', function() {
  initializeWebSocket();
  setupEventListeners();
  startDataPolling();
  addLogEntry('Sistema inicializado. Conectando al broker...');
});

// Configurar WebSocket
function initializeWebSocket() {
  try {
    appState.ws = new WebSocket(WS_URL);
    
    appState.ws.onopen = function() {
      handleConnectionSuccess();
    };
    
    appState.ws.onmessage = function(event) {
      handleIncomingMessage(event);
    };
    
    appState.ws.onclose = function() {
      handleConnectionClose();
    };
    
    appState.ws.onerror = function(error) {
      handleConnectionError(error);
    };
    
  } catch (error) {
    console.error('Error al inicializar WebSocket:', error);
    addLogEntry('Error de conexión: ' + error.message);
    attemptReconnection();
  }
}

// Manejar conexión exitosa
function handleConnectionSuccess() {
  appState.connected = true;
  appState.reconnectAttempts = 0;
  
  elements.connectionStatus.textContent = 'Conectado';
  elements.connectionStatus.className = 'status-online';
  elements.brokerStatus.textContent = 'Broker: Conectado';
  
  addLogEntry('Conexión WebSocket establecida correctamente');
  
  // Solicitar datos actuales al conectarse
  sendWebSocketMessage('horno/status', 'get_data');
}

// Manejar mensajes entrantes
function handleIncomingMessage(event) {
  try {
    const data = JSON.parse(event.data);
    
    if (data.type === 'systemData') {
      updateSystemData(data.data);
    } else if (data.message) {
      addLogEntry(data.message);
    }
    
  } catch (error) {
    console.log('Mensaje recibido (texto plano):', event.data);
    // Intentar procesar como datos del sistema en formato texto
    try {
      const systemData = JSON.parse(event.data);
      if (systemData.temperaturas) {
        updateSystemData(systemData);
      }
    } catch (e) {
      addLogEntry(event.data);
    }
  }
}

// Manejar cierre de conexión
function handleConnectionClose() {
  appState.connected = false;
  
  elements.connectionStatus.textContent = 'Desconectado';
  elements.connectionStatus.className = 'status-offline';
  elements.brokerStatus.textContent = 'Broker: Desconectado';
  
  addLogEntry('Conexión WebSocket cerrada. Intentando reconectar...');
  attemptReconnection();
}

// Manejar error de conexión
function handleConnectionError(error) {
  console.error('Error de WebSocket:', error);
  addLogEntry('Error de conexión WebSocket');
}

// Intentar reconexión
function attemptReconnection() {
  if (appState.reconnectAttempts < appState.maxReconnectAttempts) {
    appState.reconnectAttempts++;
    addLogEntry(`Intento de reconexión ${appState.reconnectAttempts}/${appState.maxReconnectAttempts}`);
    
    setTimeout(() => {
      initializeWebSocket();
    }, appState.reconnectInterval);
  } else {
    addLogEntry('No se pudo reconectar después de múltiples intentos');
  }
}

// Configurar event listeners
function setupEventListeners() {
  // Event listeners para los switches ya están configurados en el HTML
}

// Enviar mensaje a través de WebSocket
function sendWebSocketMessage(topic, message) {
  if (appState.ws && appState.ws.readyState === WebSocket.OPEN) {
    const payload = JSON.stringify({ topic, message });
    appState.ws.send(payload);
    return true;
  } else {
    addLogEntry('Error: No hay conexión WebSocket activa');
    return false;
  }
}

// Enviar comando al servidor (usando fetch como fallback)
async function sendCommand(command) {
  try {
    const response = await fetch(`${API_BASE_URL}/api/message`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        topic: 'esp32/control',
        message: command
      })
    });
    
    const data = await response.json();
    
    if (data.success) {
      addLogEntry(`Comando enviado: ${command}`);
    } else {
      addLogEntry(`Error al enviar comando: ${command}`);
    }
  } catch (error) {
    console.error('Error al enviar comando:', error);
    addLogEntry('Error de conexión al enviar comando');
    
    // Intentar enviar por WebSocket como fallback
    sendWebSocketMessage('esp32/control', command);
  }
}

// Alternar válvula
function toggleValve(valveNumber) {
  const isChecked = document.getElementById(`valv${valveNumber}-switch`).checked;
  const command = isChecked ? `valv${valveNumber}_on` : `valv${valveNumber}_off`;
  
  sendCommand(command);
  updateValveState(valveNumber, isChecked);
}

// Alternar bomba
function toggleBomba(bombaNumber) {
  const isChecked = document.getElementById(`bomba${bombaNumber}-switch`).checked;
  const command = isChecked ? `bomba${bombaNumber}_on` : `bomba${bombaNumber}_off`;
  
  sendCommand(command);
  updateBombaState(bombaNumber, isChecked);
}

// Actualizar estado de válvula en la interfaz
function updateValveState(valveNumber, state) {
  const stateElement = document.getElementById(`valv${valveNumber}-state`);
  stateElement.textContent = state ? 'Abierta' : 'Cerrada';
  stateElement.className = state ? 'control-state state-on' : 'control-state state-off';
}

// Actualizar estado de bomba en la interfaz
function updateBombaState(bombaNumber, state) {
  const stateElement = document.getElementById(`bomba${bombaNumber}-state`);
  stateElement.textContent = state ? 'Encendida' : 'Apagada';
  stateElement.className = state ? 'control-state state-on' : 'control-state state-off';
}

// Actualizar datos del sistema en la interfaz
function updateSystemData(data) {
  // Actualizar temperaturas
  if (data.temperaturas && Array.isArray(data.temperaturas)) {
    elements.tempTanque.querySelector('.sensor-value').textContent = `${data.temperaturas[0]} °C`;
    elements.tempHorno.querySelector('.sensor-value').textContent = `${data.temperaturas[1]} °C`;
    elements.tempCamara.querySelector('.sensor-value').textContent = `${data.temperaturas[2]} °C`;
    elements.tempSalida.querySelector('.sensor-value').textContent = `${data.temperaturas[3]} °C`;
  }
  
  // Actualizar niveles
  if (data.niveles && Array.isArray(data.niveles)) {
    elements.nivelVacio.querySelector('.sensor-value').textContent = `${data.niveles[0]} %`;
    elements.nivelMitad.querySelector('.sensor-value').textContent = `${data.niveles[1]} %`;
    elements.nivelLleno.querySelector('.sensor-value').textContent = `${data.niveles[2]} %`;
  }
  
  // Actualizar presión
  if (data.presion !== undefined) {
    elements.presion.querySelector('.sensor-value').textContent = `${data.presion} bar`;
  }
  
  // Actualizar estados de actuadores
  if (data.valvula1 !== undefined) {
    elements.valv1Switch.checked = data.valvula1;
    updateValveState(1, data.valvula1);
  }
  
  if (data.valvula2 !== undefined) {
    elements.valv2Switch.checked = data.valvula2;
    updateValveState(2, data.valvula2);
  }
  
  if (data.bomba1 !== undefined) {
    elements.bomba1Switch.checked = data.bomba1;
    updateBombaState(1, data.bomba1);
  }
  
  if (data.bomba2 !== undefined) {
    elements.bomba2Switch.checked = data.bomba2;
    updateBombaState(2, data.bomba2);
  }
  
  // Actualizar estado del sistema
  if (data.estado) {
    elements.systemState.textContent = data.estado;
    elements.systemState.className = `status-value ${getStatusClass(data.estado)}`;
  }
  
  if (data.emergencia !== undefined) {
    elements.emergencyState.textContent = data.emergencia ? 'ACTIVO' : 'INACTIVO';
    elements.emergencyState.className = `status-value ${data.emergencia ? 'status-emergency' : ''}`;
  }
  
  if (data.bombaActiva) {
    elements.activePump.textContent = data.bombaActiva;
  }
  
  if (data.lastUpdate) {
    elements.lastUpdate.textContent = formatDateTime(data.lastUpdate);
  }
  
  // Guardar datos en el estado de la aplicación
  appState.systemData = { ...appState.systemData, ...data };
}

// Obtener clase CSS para el estado del sistema
function getStatusClass(status) {
  switch (status) {
    case 'SISTEMA_APAGADO': return 'status-off';
    case 'SISTEMA_ENCENDIDO': return 'status-on';
    case 'MODO_EMERGENCIA': return 'status-emergency';
    default: return '';
  }
}

// Formatear fecha y hora
function formatDateTime(dateString) {
  const date = new Date(dateString);
  return date.toLocaleTimeString() + ' ' + date.toLocaleDateString();
}

// Añadir entrada al log de mensajes
function addLogEntry(message) {
  const now = new Date();
  const timeString = now.toLocaleTimeString();
  
  const logEntry = document.createElement('div');
  logEntry.className = 'log-entry';
  logEntry.innerHTML = `<span class="log-time">${timeString}</span> ${message}`;
  
  elements.messageLog.prepend(logEntry);
  
  // Limitar el número de entradas en el log
  if (elements.messageLog.children.length > 100) {
    elements.messageLog.removeChild(elements.messageLog.lastChild);
  }
}

// Polling periódico para datos del sistema
function startDataPolling() {
  // Polling cada 5 segundos para obtener datos actualizados
  setInterval(async () => {
    if (!appState.connected) return;
    
    try {
      const response = await fetch(`${API_BASE_URL}/api/system-data`);
      const data = await response.json();
      
      if (data.success) {
        updateSystemData(data.data);
      }
    } catch (error) {
      console.error('Error en polling de datos:', error);
    }
  }, 5000);
}

// Hacer funciones globales para que estén disponibles en los onclick del HTML
window.sendCommand = sendCommand;
window.toggleValve = toggleValve;
window.toggleBomba = toggleBomba;
