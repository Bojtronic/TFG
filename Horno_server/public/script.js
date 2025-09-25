// Estado de la aplicación
let appState = {
  connected: false,
  systemData: {},
  eventSource: null
};

const ESTADOS = {
  0: 'APAGADO',
  1: 'DETENER',
  2: 'PROCESANDO',
  3: 'EMERGENCIA',
  4: 'MANUAL'
};

const MENSAJES = {
  0: 'SISTEMA APAGADO',
  1: 'Horno y camara calientes, Nivel del tanque mas de la mitad',
  2: 'Hay presion de agua, Horno y camara calientes, Nivel del tanque está entre vacio y mitad',
  3: 'Horno y camara calientes, Nivel del tanque menor a la mitad',
  4: 'Horno y camara calientes, Nivel del tanque mayor a la mitad',
  5: 'Horno y camara calientes, Nivel del tanque lleno',
  6: 'Horno y camara frios, Nivel del tanque menor a la mitad',
  7: 'Horno y camara frios, Nivel del tanque mayor a la mitad',
  8: 'GRAVE: No hay presion de agua, Horno y camara DEMASIADO calientes, Tanque vacio',
  9: 'No hay presion de agua, Nivel de tanque vacio, Horno caliente',
  10: 'No hay presion de agua, El tanque tiene algo de agua, Horno caliente',
  11: 'Tanque de agua esta muy caliente y no esta lleno, Horno y camara calientes',
  12: 'Tanque de agua esta muy caliente y no esta lleno, Horno y camara frios',
  13: 'Horno caliente, El tanque no esta lleno',
  14: 'Horno caliente, El tanque esta lleno',
  15: 'Camara de humo caliente, El tanque no esta lleno',
  16: 'Modo MANUAL activado',
  17: 'DESCONOCIDO'
};

// Variable para guardar el último mensaje mostrado (evitar duplicados)
let ultimoMensajeMostrado = null;


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
  nivelTanque: document.getElementById('nivel-tanque'),
  //nivelVacio: document.getElementById('nivel-vacio'),
  //nivelMitad: document.getElementById('nivel-mitad'),
  //nivelLleno: document.getElementById('nivel-lleno'),
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

  // Estado del sistema (SOLO LOS QUE QUEDAN)
  systemState: document.getElementById('system-state'),
  lastUpdate: document.getElementById('last-update')

};

// Inicialización
document.addEventListener('DOMContentLoaded', function () {
  setupEventListeners();
  initializeSSE();
  loadSystemData();
  addLogEntry('Sistema inicializado. Conectando al servidor...');
});

// Configurar EventSource para SSE
function initializeSSE() {
  try {
    appState.eventSource = new EventSource('/events');

    appState.eventSource.onopen = function () {
      console.log('✅ Conexión SSE establecida');
      appState.connected = true;
      updateConnectionStatus(true);
      addLogEntry('Conexión con el servidor establecida');
    };

    appState.eventSource.onmessage = function (event) {
      try {
        const data = JSON.parse(event.data);

        if (data.type === 'systemData') {
          updateSystemData(data.data);
        }
      } catch (error) {
        console.log('Mensaje recibido:', event.data);
      }
    };

    appState.eventSource.onerror = function (error) {
      console.error('Error en SSE:', error);
      appState.connected = false;
      updateConnectionStatus(false);

      // Intentar reconectar después de 5 segundos
      setTimeout(() => {
        if (!appState.connected) {
          console.log('🔄 Intentando reconectar SSE...');
          appState.eventSource.close();
          initializeSSE();
        }
      }, 5000);
    };
  } catch (error) {
    console.error('Error al inicializar SSE:', error);
    appState.connected = false;
    updateConnectionStatus(false);
  }
}

// Actualizar estado de conexión en la UI
function updateConnectionStatus(connected) {
  if (connected) {
    elements.connectionStatus.textContent = 'Conectado';
    elements.connectionStatus.className = 'status-online';
    elements.brokerStatus.textContent = 'Servidor: Conectado';
  } else {
    elements.connectionStatus.textContent = 'Desconectado';
    elements.connectionStatus.className = 'status-offline';
    elements.brokerStatus.textContent = 'Servidor: Desconectado';
  }
}

// Cargar datos del sistema
async function loadSystemData() {
  try {
    const response = await fetch('/api/system-data');
    const data = await response.json();

    if (data.success) {
      updateSystemData(data.data);
    }
  } catch (error) {
    console.error('Error cargando datos:', error);
    addLogEntry('Error al cargar datos del sistema');
  }
}

// Configurar event listeners
function setupEventListeners() {
  // Listeners para switches
  elements.valv1Switch.addEventListener('change', () => toggleValve(1));
  elements.valv2Switch.addEventListener('change', () => toggleValve(2));
  elements.bomba1Switch.addEventListener('change', () => toggleBomba(1));
  elements.bomba2Switch.addEventListener('change', () => toggleBomba(2));

  // Listeners para botones
  document.getElementById('btn-start').addEventListener('click', () => sendCommand('start'));
  document.getElementById('btn-stop').addEventListener('click', () => sendCommand('stop'));
  document.getElementById('btn-manual').addEventListener('click', () => sendCommand('manual'));
  //document.getElementById('btn-reset').addEventListener('click', () => sendCommand('reset'));
  //document.getElementById('btn-emergency').addEventListener('click', () => sendCommand('emergency'));
}

// Enviar comando al servidor
/*
async function sendCommand(command) {
  try {
    const response = await fetch('/api/message', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        topic: 'esp32/control',
        message: command
      })
    });
    
    // Verificar si la respuesta es JSON válido
    const contentType = response.headers.get('content-type');
    if (contentType && contentType.includes('application/json')) {
      const data = await response.json();
      
      if (data.success || data.status === 'success') {
        //addLogEntry(`Comando enviado: ${command}`);
        showCommandMessage(command);
        return true;
      } else {
        //addLogEntry(`Error en servidor: ${data.message || 'Error desconocido'}`);
        showSystemMessage(17); // Mensaje de error
        return false;
      }
    } else {
      // Si no es JSON, puede ser una respuesta simple
      const text = await response.text();
      if (response.ok) {
        //addLogEntry(`Comando enviado: ${command}`);
        showCommandMessage(command);
        return true;
      } else {
        //addLogEntry(`Error en servidor: ${text}`);
        showSystemMessage(17); // Mensaje de error
        return false;
      }
    }
  } catch (error) {
    console.error('Error al enviar comando:', error);
    //addLogEntry('Error de conexión al enviar comando');
    showSystemMessage(14); // Error de comunicación
    return false;
  }
}
*/

async function sendCommand(command) {
  try {
    const response = await fetch('/api/message', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        topic: 'esp32/control',
        message: command
      })
    });

    // Solo enviar el comando, NO mostrar log
    console.log(`Comando enviado: ${command}`);
    return true;

  } catch (error) {
    console.error('Error al enviar comando:', error);
    return false;
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
  console.log('Actualizando datos del sistema:', data);

  // Actualizar temperaturas
  if (data.temperaturas && Array.isArray(data.temperaturas)) {
    elements.tempTanque.querySelector('.sensor-value').textContent = `${data.temperaturas[0]} °C`;
    elements.tempHorno.querySelector('.sensor-value').textContent = `${data.temperaturas[1]} °C`;
    elements.tempCamara.querySelector('.sensor-value').textContent = `${data.temperaturas[2]} °C`;
    elements.tempSalida.querySelector('.sensor-value').textContent = `${data.temperaturas[3]} °C`;
  }

  // Actualizar niveles
  if (data.nivelTanque !== undefined) {
    elements.nivelTanque.querySelector('.sensor-value').textContent = `${data.nivelTanque} %`;
    //elements.nivelVacio.querySelector('.sensor-value').textContent = `${data.niveles[0]} %`;
    //elements.nivelMitad.querySelector('.sensor-value').textContent = `${data.niveles[1]} %`;
    //elements.nivelLleno.querySelector('.sensor-value').textContent = `${data.niveles[2]} %`;
  }

  // Actualizar presión
  if (data.presion !== undefined) {
    elements.presion.querySelector('.sensor-value').textContent = `${data.presion} bar`;
  }

  // Actualizar estados de actuadores (QUITAR event listeners temporalmente para evitar bucles)
  elements.valv1Switch.removeEventListener('change', () => toggleValve(1));
  elements.valv2Switch.removeEventListener('change', () => toggleValve(2));
  elements.bomba1Switch.removeEventListener('change', () => toggleBomba(1));
  elements.bomba2Switch.removeEventListener('change', () => toggleBomba(2));

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

  // RESTAURAR event listeners
  setTimeout(() => {
    elements.valv1Switch.addEventListener('change', () => toggleValve(1));
    elements.valv2Switch.addEventListener('change', () => toggleValve(2));
    elements.bomba1Switch.addEventListener('change', () => toggleBomba(1));
    elements.bomba2Switch.addEventListener('change', () => toggleBomba(2));
  }, 100);

  
  if (data.mensaje !== undefined && data.mensaje !== null) {
    mostrarMensajeEstado(data.mensaje);
  }

  if (data.estado !== undefined) {
    const estadoTexto = ESTADOS[data.estado] || 'DESCONOCIDO';
    elements.systemState.textContent = estadoTexto;
    elements.systemState.className = `status-value ${getStatusClass(data.estado)}`;
  }


  // NOTA: Se han eliminado las referencias a emergency-state y active-pump

  if (data.lastUpdate) {
    elements.lastUpdate.textContent = formatDateTime(data.lastUpdate);
  }

  // Guardar datos en el estado de la aplicación
  appState.systemData = { ...appState.systemData, ...data };
}

// Obtener clase CSS para el estado del sistema
function getStatusClass(status) {
  switch (status) {
    case 0: return 'status-off';        // APAGADO
    case 1: return 'status-stop';       // DETENER
    case 2: return 'status-on';         // PROCESANDO
    case 3: return 'status-emergency';  // EMERGENCIA
    case 4: return 'status-manual';     // MANUAL
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


// Función específica para mensajes de estado del sistema
function mostrarMensajeEstado(mensajeCode) {
  // Evitar mostrar el mismo mensaje repetidamente
  if (mensajeCode === ultimoMensajeMostrado) {
    return;
  }
  
  const textoMensaje = MENSAJES[mensajeCode] || 'Estado desconocido';
  const ahora = new Date();
  const horaString = ahora.toLocaleTimeString();
  
  // Crear elemento del mensaje
  const elementoMensaje = document.createElement('div');
  elementoMensaje.className = 'log-entry';
  
  // Determinar clase según el tipo de mensaje - ACTUALIZADO
  let claseMensaje = 'mensaje-normal';
  
  if (mensajeCode >= 8 && mensajeCode <= 15) { // EMERGENCIA_1 a EMERGENCIA_8
    claseMensaje = 'mensaje-emergencia-grave';
  } else if (mensajeCode >= 3 && mensajeCode <= 7) { // PROCESANDO_1 a PROCESANDO_5
    claseMensaje = 'mensaje-procesando';
  } else if (mensajeCode === 16) { // MANUAL_0
    claseMensaje = 'mensaje-manual';
  } else if (mensajeCode === 0) { // APAGADO_0
    claseMensaje = 'mensaje-apagado';
  } else if (mensajeCode === 1 || mensajeCode === 2) { // DETENER_1 y DETENER_2
    claseMensaje = 'mensaje-advertencia'; 
  }
  
  elementoMensaje.innerHTML = `
    <span class="log-time">${horaString}</span>
    <span class="${claseMensaje}">${textoMensaje}</span>
  `;
  
  // Agregar al inicio del log de mensajes
  elements.messageLog.prepend(elementoMensaje);
  
  // Limitar a 20 mensajes máximo
  if (elements.messageLog.children.length > 20) {
    elements.messageLog.removeChild(elements.messageLog.lastChild);
  }
  
  // Actualizar último mensaje mostrado
  ultimoMensajeMostrado = mensajeCode;
  
  console.log(`Mensaje de estado mostrado: ${mensajeCode} - ${textoMensaje}`);
}