const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');

const PORT = process.env.PORT || 3000;

let clients = []; // conexiones SSE (navegadores escuchando)
let pendingCommands = [];
const MAX_PENDING_COMMANDS = 10;

// Almacenamiento de datos del sistema de horno
let systemData = {
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
};

// Servidor HTTP
const server = http.createServer((req, res) => {
  console.log(`🌐 Solicitud recibida: ${req.method} ${req.url}`);
  
  // Habilitar CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    res.writeHead(200);
    res.end();
    return;
  }

  // Servir la aplicación web
  if (req.url === '/' || req.url === '/index.html') {
    serveWebApp(req, res);
  } 
  // Endpoint para comandos ESP32
  else if (req.url === '/api/esp32-commands' && req.method === 'GET') {
    handleEsp32Commands(req, res);
  } 
  // Endpoint para datos del sistema
  else if (req.url === '/api/system-data' && req.method === 'GET') {
    handleSystemData(req, res);
  } 
  // Endpoint para recibir datos del ESP32
  else if (req.url === '/api/message' && req.method === 'POST') {
    handlePostMessage(req, res);
  } 
  // SSE para actualizaciones en tiempo real
  else if (req.url === '/events') {
    handleSSE(req, res);
  } 
  // Servir archivos estáticos
  else if (req.url.match(/\.(css|js|png|jpg|jpeg|gif|svg|ico)$/i)) {
    serveStaticFile(req, res);
  } 
  // Ruta no encontrada
  else {
    res.writeHead(404);
    res.end('Not found');
  }
});

// Función para servir la aplicación web
function serveWebApp(req, res) {
  const html = `<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Control Horno de Biomasa ECOVIEW</title>
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
  <style>
    ${getEmbeddedStyles()}
  </style>
</head>
<body>
  <div class="container">
    <header class="header">
      <h1><i class="fas fa-fire"></i> Control del Horno de Biomasa</h1>
      <p>ECOVIEW - Sistema de Gestión Térmica</p>
    </header>

    <div class="dashboard">
      <!-- Panel de Temperaturas -->
      <div class="panel">
        <h2><i class="fas fa-thermometer-half"></i> Temperaturas</h2>
        <div class="sensor-grid">
          <div class="sensor-item" id="temp-tanque">
            <span class="sensor-label">Tanque de Agua</span>
            <div class="sensor-value">-- <span class="sensor-unit">°C</span></div>
          </div>
          <div class="sensor-item" id="temp-horno">
            <span class="sensor-label">Horno</span>
            <div class="sensor-value">-- <span class="sensor-unit">°C</span></div>
          </div>
          <div class="sensor-item" id="temp-camara">
            <span class="sensor-label">Cámara de Humo</span>
            <div class="sensor-value">-- <span class="sensor-unit">°C</span></div>
          </div>
          <div class="sensor-item" id="temp-salida">
            <span class="sensor-label">Salida de Agua</span>
            <div class="sensor-value">-- <span class="sensor-unit">°C</span></div>
          </div>
        </div>
      </div>

      <!-- Panel de Niveles y Presión -->
      <div class="panel">
        <h2><i class="fas fa-tachometer-alt"></i> Niveles y Presión</h2>
        <div class="sensor-grid">
          <div class="sensor-item" id="nivel-vacio">
            <span class="sensor-label">Nivel Vacío</span>
            <div class="sensor-value">-- <span class="sensor-unit">%</span></div>
          </div>
          <div class="sensor-item" id="nivel-mitad">
            <span class="sensor-label">Nivel Mitad</span>
            <div class="sensor-value">-- <span class="sensor-unit">%</span></div>
          </div>
          <div class="sensor-item" id="nivel-lleno">
            <span class="sensor-label">Nivel Lleno</span>
            <div class="sensor-value">-- <span class="sensor-unit">%</span></div>
          </div>
          <div class="sensor-item" id="presion">
            <span class="sensor-label">Presión</span>
            <div class="sensor-value">-- <span class="sensor-unit">bar</span></div>
          </div>
        </div>
      </div>

      <!-- Panel de Control -->
      <div class="panel">
        <h2><i class="fas fa-cogs"></i> Control de Actuadores</h2>
        <div class="control-panel">
          <div class="control-item">
            <div class="control-info">
              <span class="control-label">Válvula Agua Caliente</span>
              <span class="control-state" id="valv1-state">Cerrada</span>
            </div>
            <label class="switch">
              <input type="checkbox" id="valv1-switch">
              <span class="slider"></span>
            </label>
          </div>
          <div class="control-item">
            <div class="control-info">
              <span class="control-label">Válvula Agua Fría</span>
              <span class="control-state" id="valv2-state">Cerrada</span>
            </div>
            <label class="switch">
              <input type="checkbox" id="valv2-switch">
              <span class="slider"></span>
            </label>
          </div>
          <div class="control-item">
            <div class="control-info">
              <span class="control-label">Bomba Principal</span>
              <span class="control-state" id="bomba1-state">Apagada</span>
            </div>
            <label class="switch">
              <input type="checkbox" id="bomba1-switch">
              <span class="slider"></span>
            </label>
          </div>
          <div class="control-item">
            <div class="control-info">
              <span class="control-label">Bomba Redundante</span>
              <span class="control-state" id="bomba2-state">Apagada</span>
            </div>
            <label class="switch">
              <input type="checkbox" id="bomba2-switch">
              <span class="slider"></span>
            </label>
          </div>
        </div>
      </div>

      <!-- Panel de Estado -->
      <div class="panel">
        <h2><i class="fas fa-info-circle"></i> Estado del Sistema</h2>
        <div class="status-panel">
          <div class="status-item">
            <span class="status-label">Estado Actual:</span>
            <span class="status-value status-apagado" id="system-state">APAGADO</span>
          </div>
          <div class="status-item">
            <span class="status-label">Modo Emergencia:</span>
            <span class="status-value" id="emergency-state">INACTIVO</span>
          </div>
          <div class="status-item">
            <span class="status-label">Bomba Activa:</span>
            <span class="status-value" id="active-pump">PRINCIPAL</span>
          </div>
          <div class="status-item">
            <span class="status-label">Última Actualización:</span>
            <span class="status-value" id="last-update">--</span>
          </div>
        </div>

        <div class="button-group">
          <button class="btn btn-start" id="btn-start"><i class="fas fa-play"></i> Iniciar</button>
          <button class="btn btn-stop" id="btn-stop"><i class="fas fa-stop"></i> Detener</button>
          <button class="btn btn-reset" id="btn-reset"><i class="fas fa-redo"></i> Reset</button>
          <button class="btn btn-emergency" id="btn-emergency"><i class="fas fa-exclamation-triangle"></i> Emergencia</button>
        </div>
      </div>
    </div>

    <!-- Mensajes del Sistema -->
    <div class="panel">
      <h2><i class="fas fa-comment-alt"></i> Mensajes del Sistema</h2>
      <div class="message-log" id="message-log">
        <div class="log-entry"><span class="log-time">00:00:00</span> Sistema inicializado</div>
      </div>
    </div>

    <!-- Estado de Conexión -->
    <div class="connection-status">
      <span id="connection-status" class="status-offline"><i class="fas fa-plug"></i> Desconectado</span>
      <span id="broker-status">Servidor: Conectado</span>
    </div>
  </div>

  <script>
    ${getEmbeddedScript()}
  </script>
</body>
</html>`;
  
  res.writeHead(200, { "Content-Type": "text/html" });
  res.end(html);
}

// Función para manejar comandos ESP32
function handleEsp32Commands(req, res) {
  console.log(`📡 Comandos solicitados por: ${req.socket.remoteAddress}`);
  console.log(`📊 Comandos pendientes: ${pendingCommands.length}`);
  
  res.setHeader('Content-Type', 'application/json');
  
  if (pendingCommands.length > 0) {
    const commands = pendingCommands.join(',');
    pendingCommands = [];
    console.log(`📤 Enviando comandos a ESP32: ${commands}`);
    res.end(JSON.stringify({
      success: true,
      commands: commands,
      message: 'Comandos enviados'
    }));
  } else {
    console.log('📭 No hay comandos pendientes para ESP32');
    res.end(JSON.stringify({
      success: true,
      commands: 'no_commands',
      message: 'No hay comandos pendientes'
    }));
  }
}

// Función para manejar datos del sistema
function handleSystemData(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    success: true,
    data: systemData,
    timestamp: new Date().toISOString()
  }));
}

// Función para manejar mensajes POST del ESP32
function handlePostMessage(req, res) {
  let body = '';
  req.on('data', chunk => {
    body += chunk.toString();
  });

  req.on('end', () => {
    try {
      console.log('📨 Mensaje recibido:', body);
      const data = JSON.parse(body);

      if (data.topic && data.message) {
        // Procesar mensajes de control
        if (data.topic === 'esp32/control') {
          const validCommands = [
            'start', 'stop', 'reset', 'emergency', 
            'valv1_on', 'valv1_off', 'valv2_on', 'valv2_off',
            'bomba1_on', 'bomba1_off', 'bomba2_on', 'bomba2_off'
          ];
          
          if (validCommands.includes(data.message)) {
            if (pendingCommands.length < MAX_PENDING_COMMANDS) {
              pendingCommands.push(data.message);
              console.log(`💾 Comando almacenado: ${data.message}`);
            }
          }
        }
        // Procesar datos del sistema
        else if (data.topic === 'horno/data') {
          try {
            // Parsear el formato de cadena de consulta
            const params = new URLSearchParams(data.message);
            
            // Extraer y convertir datos
            const temperaturas = params.get('temperaturas').match(/[\d.]+/g).map(Number);
            const niveles = params.get('niveles').match(/[\d.]+/g).map(Number);
            const presion = parseFloat(params.get('presion'));
            const valvula1 = params.get('valvula1') === 'true';
            const valvula2 = params.get('valvula2') === 'true';
            const bomba1 = params.get('bomba1') === 'true';
            const bomba2 = params.get('bomba2') === 'true';
            const estado = params.get('estado');
            const emergencia = params.get('emergencia') === 'true';
            const bombaActiva = params.get('bombaActiva');
            
            // Actualizar datos del sistema
            const systemUpdate = {
              temperaturas,
              niveles,
              presion,
              valvula1,
              valvula2,
              bomba1,
              bomba2,
              estado,
              emergencia,
              bombaActiva,
              lastUpdate: new Date().toISOString()
            };
            
            updateSystemData(systemUpdate);
          } catch (e) {
            console.log('❌ Error parseando datos del sistema:', e.message);
          }
        }

        res.writeHead(200, {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*'
        });

        res.end(JSON.stringify({
          status: 'success',
          message: 'Mensaje recibido',
          topic: data.topic,
          received: data.message,
          pendingCommands: pendingCommands.length
        }));

      } else {
        res.writeHead(400, {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*'
        });
        res.end(JSON.stringify({
          status: 'error',
          message: 'Formato inválido. Use: {"topic":"x","message":"y"}'
        }));
      }
    } catch (error) {
      res.writeHead(500, {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      });
      res.end(JSON.stringify({
          status: 'error',
          message: 'Error procesando JSON: ' + error.message
      }));
    }
  });
}

// Función para manejar SSE
function handleSSE(req, res) {
  console.log('📡 Nuevo cliente SSE conectado');
  res.writeHead(200, {
    'Content-Type': 'text/event-stream',
    'Cache-Control': 'no-cache',
    'Connection': 'keep-alive',
    'Access-Control-Allow-Origin': '*'
  });
  res.write('\n');
  clients.push(res);

  req.on('close', () => {
    console.log('❌ Cliente SSE desconectado');
    clients = clients.filter(client => client !== res);
  });
}

// Función para servir archivos estáticos
function serveStaticFile(req, res) {
  // En una implementación real, servirías archivos desde el sistema de archivos
  // Para simplificar, estamos sirviendo todo embebido
  res.writeHead(404);
  res.end('Not found');
}

// Función para actualizar datos del sistema
function updateSystemData(newData) {
  Object.keys(newData).forEach(key => {
    if (systemData.hasOwnProperty(key)) {
      systemData[key] = newData[key];
    }
  });
  
  systemData.lastUpdate = new Date().toISOString();
  console.log('✅ Datos del sistema actualizados');
  
  // Transmitir a clientes SSE
  broadcastSystemData();
}

// Transmitir datos a clientes SSE
function broadcastSystemData() {
  clients.forEach(client => {
    client.write(`data: ${JSON.stringify({type: 'systemData', data: systemData})}\n\n`);
  });
}

// Obtener estilos embebidos
function getEmbeddedStyles() {
  return `
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d); color: #333; min-height: 100vh; }
    .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
    .header { text-align: center; margin-bottom: 30px; color: white; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }
    .header h1 { font-size: 2.5rem; margin-bottom: 10px; }
    .header p { font-size: 1.2rem; opacity: 0.9; }
    .dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 30px; }
    .panel { background: rgba(255, 255, 255, 0.95); border-radius: 15px; padding: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); backdrop-filter: blur(10px); }
    .panel h2 { margin-bottom: 15px; color: #2c3e50; border-bottom: 2px solid #eee; padding-bottom: 10px; }
    .sensor-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
    .sensor-item { background: #f8f9fa; padding: 15px; border-radius: 10px; text-align: center; }
    .sensor-label { display: block; font-weight: bold; margin-bottom: 5px; color: #6c757d; }
    .sensor-value { font-size: 1.8rem; font-weight: bold; color: #2c3e50; }
    .sensor-unit { font-size: 1rem; color: #6c757d; }
    .control-panel { display: flex; flex-direction: column; gap: 15px; }
    .control-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; background: #f8f9fa; border-radius: 10px; }
    .control-info { display: flex; flex-direction: column; }
    .control-label { font-weight: bold; }
    .control-state { font-size: 0.9rem; color: #6c757d; }
    .state-on { color: #28a745; }
    .state-off { color: #dc3545; }
    .switch { position: relative; display: inline-block; width: 60px; height: 34px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
    .slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
    input:checked + .slider { background-color: #2196F3; }
    input:checked + .slider:before { transform: translateX(26px); }
    .status-panel { display: flex; flex-direction: column; gap: 10px; margin-bottom: 20px; }
    .status-item { display: flex; justify-content: space-between; padding: 10px; background: #f8f9fa; border-radius: 10px; }
    .status-label { font-weight: bold; }
    .status-value { font-weight: bold; }
    .status-on { color: #28a745; }
    .status-off { color: #dc3545; }
    .status-emergency { color: #ffc107; }
    .button-group { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .btn { padding: 12px; border: none; border-radius: 8px; color: white; font-weight: bold; cursor: pointer; transition: all 0.3s; display: flex; align-items: center; justify-content: center; gap: 8px; }
    .btn:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }
    .btn-start { background: linear-gradient(135deg, #28a745, #20c997); }
    .btn-stop { background: linear-gradient(135deg, #dc3545, #fd7e14); }
    .btn-reset { background: linear-gradient(135deg, #6c757d, #adb5bd); }
    .btn-emergency { background: linear-gradient(135deg, #ffc107, #fd7e14); }
    .message-log { max-height: 200px; overflow-y: auto; background: #f8f9fa; border-radius: 10px; padding: 15px; }
    .log-entry { margin-bottom: 8px; padding-bottom: 8px; border-bottom: 1px solid #dee2e6; }
    .log-time { color: #6c757d; font-weight: bold; margin-right: 10px; }
    .connection-status { display: flex; justify-content: space-between; background: rgba(255, 255, 255, 0.2); padding: 15px; border-radius: 10px; margin-top: 20px; color: white; }
    .status-online { color: #28a745; }
    .status-offline { color: #dc3545; }
    
    @media (max-width: 768px) {
      .dashboard { grid-template-columns: 1fr; }
      .sensor-grid { grid-template-columns: 1fr; }
      .button-group { grid-template-columns: 1fr; }
    }
  `;
}

// Obtener script embebido
function getEmbeddedScript() {
  return `
    // Estado de la aplicación
    let appState = {
      connected: true,
      systemData: {}
    };

    // Elementos de la interfaz
    const elements = {
      connectionStatus: document.getElementById('connection-status'),
      brokerStatus: document.getElementById('broker-status'),
      messageLog: document.getElementById('message-log'),
      tempTanque: document.getElementById('temp-tanque'),
      tempHorno: document.getElementById('temp-horno'),
      tempCamara: document.getElementById('temp-camara'),
      tempSalida: document.getElementById('temp-salida'),
      nivelVacio: document.getElementById('nivel-vacio'),
      nivelMitad: document.getElementById('nivel-mitad'),
      nivelLleno: document.getElementById('nivel-lleno'),
      presion: document.getElementById('presion'),
      valv1State: document.getElementById('valv1-state'),
      valv2State: document.getElementById('valv2-state'),
      bomba1State: document.getElementById('bomba1-state'),
      bomba2State: document.getElementById('bomba2-state'),
      valv1Switch: document.getElementById('valv1-switch'),
      valv2Switch: document.getElementById('valv2-switch'),
      bomba1Switch: document.getElementById('bomba1-switch'),
      bomba2Switch: document.getElementById('bomba2-switch'),
      systemState: document.getElementById('system-state'),
      emergencyState: document.getElementById('emergency-state'),
      activePump: document.getElementById('active-pump'),
      lastUpdate: document.getElementById('last-update')
    };

    // Inicialización
    document.addEventListener('DOMContentLoaded', function() {
      setupEventListeners();
      initializeSSE();
      loadSystemData();
      addLogEntry('Sistema inicializado. Conectado al servidor local.');
    });

    // Configurar EventSource para SSE
    function initializeSSE() {
      const eventSource = new EventSource('/events');
      
      eventSource.onmessage = function(event) {
        try {
          const data = JSON.parse(event.data);
          
          if (data.type === 'systemData') {
            updateSystemData(data.data);
          }
        } catch (error) {
          console.log('Mensaje recibido:', event.data);
        }
      };
      
      eventSource.onerror = function(error) {
        console.error('Error en SSE:', error);
        elements.connectionStatus.textContent = 'Error de conexión';
        elements.connectionStatus.className = 'status-offline';
      };
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
      document.getElementById('btn-reset').addEventListener('click', () => sendCommand('reset'));
      document.getElementById('btn-emergency').addEventListener('click', () => sendCommand('emergency'));
    }

    // Enviar comando al servidor
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
        
        const data = await response.json();
        
        if (data.success) {
          addLogEntry(\`Comando enviado: \${command}\`);
        } else {
          addLogEntry(\`Error al enviar comando: \${command}\`);
        }
      } catch (error) {
        console.error('Error al enviar comando:', error);
        addLogEntry('Error de conexión al enviar comando');
      }
    }

    // Alternar válvula
    function toggleValve(valveNumber) {
      const isChecked = document.getElementById(\`valv\${valveNumber}-switch\`).checked;
      const command = isChecked ? \`valv\${valveNumber}_on\` : \`valv\${valveNumber}_off\`;
      
      sendCommand(command);
      updateValveState(valveNumber, isChecked);
    }

    // Alternar bomba
    function toggleBomba(bombaNumber) {
      const isChecked = document.getElementById(\`bomba\${bombaNumber}-switch\`).checked;
      const command = isChecked ? \`bomba\${bombaNumber}_on\` : \`bomba\${bombaNumber}_off\`;
      
      sendCommand(command);
      updateBombaState(bombaNumber, isChecked);
    }

    // Actualizar estado de válvula en la interfaz
    function updateValveState(valveNumber, state) {
      const stateElement = document.getElementById(\`valv\${valveNumber}-state\`);
      stateElement.textContent = state ? 'Abierta' : 'Cerrada';
      stateElement.className = state ? 'control-state state-on' : 'control-state state-off';
    }

    // Actualizar estado de bomba en la interfaz
    function updateBombaState(bombaNumber, state) {
      const stateElement = document.getElementById(\`bomba\${bombaNumber}-state\`);
      stateElement.textContent = state ? 'Encendida' : 'Apagada';
      stateElement.className = state ? 'control-state state-on' : 'control-state state-off';
    }

    // Actualizar datos del sistema en la interfaz
    function updateSystemData(data) {
      // Actualizar temperaturas
      if (data.temperaturas && Array.isArray(data.temperaturas)) {
        elements.tempTanque.querySelector('.sensor-value').textContent = \`\${data.temperaturas[0]} °C\`;
        elements.tempHorno.querySelector('.sensor-value').textContent = \`\${data.temperaturas[1]} °C\`;
        elements.tempCamara.querySelector('.sensor-value').textContent = \`\${data.temperaturas[2]} °C\`;
        elements.tempSalida.querySelector('.sensor-value').textContent = \`\${data.temperaturas[3]} °C\`;
      }
      
      // Actualizar niveles
      if (data.niveles && Array.isArray(data.niveles)) {
        elements.nivelVacio.querySelector('.sensor-value').textContent = \`\${data.niveles[0]} %\`;
        elements.nivelMitad.querySelector('.sensor-value').textContent = \`\${data.niveles[1]} %\`;
        elements.nivelLleno.querySelector('.sensor-value').textContent = \`\${data.niveles[2]} %\`;
      }
      
      // Actualizar presión
      if (data.presion !== undefined) {
        elements.presion.querySelector('.sensor-value').textContent = \`\${data.presion} bar\`;
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
        elements.systemState.className = \`status-value \${getStatusClass(data.estado)}\`;
      }
      
      if (data.emergencia !== undefined) {
        elements.emergencyState.textContent = data.emergencia ? 'ACTIVO' : 'INACTIVO';
        elements.emergencyState.className = \`status-value \${data.emergencia ? 'status-emergency' : ''}\`;
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
      logEntry.innerHTML = \`<span class="log-time">\${timeString}</span> \${message}\`;
      
      elements.messageLog.prepend(logEntry);
      
      // Limitar el número de entradas en el log
      if (elements.messageLog.children.length > 100) {
        elements.messageLog.removeChild(elements.messageLog.lastChild);
      }
    }
  `;
}

// Iniciar servidor
server.listen(PORT, '0.0.0.0', () => {
  console.log(`
🚀 Servidor ECOVIEW ejecutándose en puerto ${PORT}
=================================================

🔗 Endpoints disponibles:
- Aplicación web: http://localhost:${PORT}
- API Comandos ESP32: http://localhost:${PORT}/api/esp32-commands
- API Datos del sistema: http://localhost:${PORT}/api/system-data
- API Mensajes: http://localhost:${PORT}/api/message
- SSE Actualizaciones: http://localhost:${PORT}/events

🌐 Ambiente: ${process.env.NODE_ENV || 'development'}
🕐 Iniciado: ${new Date().toISOString()}

✅ Listo para recibir conexiones...
`);
});

// Manejo de errores
process.on('uncaughtException', (error) => {
  console.error('❌ Error no capturado:', error);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('❌ Promise rechazada:', reason);
});

process.on('SIGINT', () => {
  console.log('\n🛑 Cerrando servidor...');
  server.close(() => {
    console.log('✅ Servidor cerrado correctamente');
    process.exit(0);
  });
});
