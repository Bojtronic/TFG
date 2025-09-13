const aedes = require('aedes')();
const http = require('http');
const ws = require('websocket-stream');
const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');

const PORT = process.env.PORT || 3000;

let clients = []; // conexiones SSE (navegadores escuchando)

// ===== ALMACENAMIENTO DE COMANDOS PARA ESP32 =====
let pendingCommands = [];
const MAX_PENDING_COMMANDS = 10;

// Almacenamiento de datos del sistema de horno
let systemData = {
  temperaturas: [0, 0, 0, 0],      // [tanque, horno, cámara, salida]
  niveles: [0, 0, 0],              // [vacío, mitad, lleno]
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
  console.log(`📡 IP: ${req.socket.remoteAddress}`);
  
  // Habilitar CORS para todas las rutas
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS, DELETE');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  // Manejar preflight OPTIONS
  if (req.method === 'OPTIONS') {
    console.log('🛬 Preflight OPTIONS recibido');
    res.writeHead(200);
    res.end();
    return;
  }

  const publicPath = path.join(__dirname, 'public');

  // Manejar archivos estáticos PRIMERO
  if (req.url.match(/\.(css|js|png|jpg|jpeg|gif|svg|ico|html)$/i)) {
    console.log('📁 Sirviendo archivo estático:', req.url);
    const filePath = path.join(publicPath, req.url);
    const ext = path.extname(filePath);

    const contentTypes = {
      '.html': 'text/html',
      '.css': 'text/css',
      '.js': 'application/javascript',
      '.png': 'image/png',
      '.jpg': 'image/jpeg',
      '.jpeg': 'image/jpeg',
      '.gif': 'image/gif',
      '.svg': 'image/svg+xml',
      '.ico': 'image/x-icon'
    };

    fs.readFile(filePath, (err, data) => {
      if (err) {
        console.log('❌ Archivo no encontrado:', filePath);
        res.writeHead(404);
        res.end('Archivo no encontrado');
      } else {
        console.log('✅ Archivo servido:', filePath);
        res.writeHead(200, {
          'Content-Type': contentTypes[ext] || 'application/octet-stream'
        });
        res.end(data);
      }
    });
    return;
  }

  // ===== ENDPOINT PARA COMANDOS ESP32 =====
  if (req.url === '/api/esp32-commands' && req.method === 'GET') {
    console.log(`🎯 GET recibido en /api/esp32-commands`);
    console.log(`📡 Desde IP: ${req.socket.remoteAddress}`);
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
    return;
  }

  // ===== ENDPOINT PARA DATOS DEL SISTEMA =====
  if (req.url === '/api/system-data' && req.method === 'GET') {
    console.log('📊 Solicitud de datos del sistema recibida');
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      success: true,
      data: systemData,
      timestamp: new Date().toISOString()
    }));
    return;
  }

  // ===== ENDPOINT PARA MENSAJES =====
  if (req.url === '/api/messages' && req.method === 'GET') {
    console.log('📋 Solicitud de mensajes recibida');
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      success: true,
      messages: [], // Podrías implementar un sistema de mensajes si lo necesitas
      count: 0
    }));
    return;
  }

  if (req.url === '/api/messages' && req.method === 'DELETE') {
    console.log('🧹 Solicitud para limpiar mensajes recibida');
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      success: true,
      message: 'Historial de mensajes limpiado'
    }));
    return;
  }

  // Resto de endpoints
  if (req.url === '/') {
    console.log('🏠 Sirviendo página principal');
    const filePath = path.join(__dirname, 'public', 'index.html');
    fs.readFile(filePath, (err, data) => {
      if (err) {
        console.log('❌ Error cargando index.html:', err);
        res.writeHead(500);
        res.end("Error cargando index.html");
      } else {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(data);
      }
    });
  } else if (req.url === '/events') {
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
  } else if (req.url === '/status') {
    console.log('📊 Solicitud de status recibida');
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      status: 'online',
      clients: aedes.connectedClients,
      timestamp: new Date().toISOString(),
      environment: process.env.NODE_ENV || 'development',
      pendingCommands: pendingCommands.length,
      systemData: systemData
    }));
  } else if (req.url === '/api/message' && req.method === 'POST') {
    console.log('📨 POST recibido en /api/message');
    let body = '';
    req.on('data', chunk => {
      body += chunk.toString();
    });

    req.on('end', () => {
      try {
        console.log('📨 Mensaje recibido via API:', body);

        const data = JSON.parse(body);

        if (data.topic && data.message) {
          // ===== ALMACENAR COMANDOS PARA ESP32 =====
          if (data.topic === 'esp32/control') {
            // Comandos para el sistema de horno de biomasa
            const validCommands = [
              'start', 'stop', 'reset', 'emergency', 
              'valv1_on', 'valv1_off', 'valv2_on', 'valv2_off',
              'bomba1_on', 'bomba1_off', 'bomba2_on', 'bomba2_off'
            ];
            
            if (validCommands.includes(data.message)) {
              if (pendingCommands.length < MAX_PENDING_COMMANDS) {
                pendingCommands.push(data.message);
                console.log(`💾 Comando almacenado: ${data.message}`);
                console.log(`📊 Comandos pendientes: ${pendingCommands.length}`);
              } else {
                console.log('❌ Límite de comandos pendientes alcanzado');
              }
            }
          }
          // ===== FIN ALMACENAMIENTO =====

          // Procesar mensajes de datos del sistema
          if (data.topic === 'horno/data') {
            try {
              const systemUpdate = JSON.parse(data.message);
              updateSystemData(systemUpdate);
            } catch (e) {
              console.log('❌ Error parseando datos del sistema:', e.message);
            }
          }

          // Publicar en el broker MQTT (para otros clientes)
          aedes.publish({
            topic: data.topic,
            payload: data.message,
            qos: 0,
            retain: false
          });

          res.writeHead(200, {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*'
          });

          res.end(JSON.stringify({
            status: 'success',
            message: 'Mensaje publicado en MQTT',
            topic: data.topic,
            received: data.message,
            pendingCommands: pendingCommands.length
          }));

          console.log(`📤 Publicado en ${data.topic}: ${data.message}`);

        } else {
          console.log('❌ Formato inválido en /api/message');
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
        console.log('❌ Error procesando JSON en /api/message:', error.message);
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
  } else {
    console.log('❌ Endpoint no encontrado:', req.url);
    res.writeHead(404, {
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*'
    });
    res.end(JSON.stringify({
      status: 'error',
      message: 'Endpoint no encontrado',
      availableEndpoints: [
        '/', '/events', '/status', '/api/message', 
        '/api/esp32-commands', '/api/system-data', '/api/messages'
      ]
    }));
  }
});

// ===== SOPORTE DUAL =====

// 1. MQTT ESTÁNDAR sobre WebSocket (para clientes Node.js, Python, etc.)
ws.createServer({ server }, aedes.handle);

// 2. WEB SOCKET SIMPLE (para ESP32 y clientes básicos)
const wss = new WebSocket.Server({
  server,
  path: '/simple'  // Path especial para clientes simples
});

wss.on('connection', function connection(ws, req) {
  const clientId = `simple_${Math.random().toString(36).substr(2, 9)}`;
  const clientIp = req.socket.remoteAddress;
  const msg = `📡 Cliente WebSocket simple conectado: ${clientId} desde ${clientIp}`;
  console.log(msg);

  ws.on('message', function incoming(message) {
    try {
      const data = message.toString();
      const msg = `📩 Mensaje simple de ${clientId}: ${data}`;
      console.log(msg);

      // Procesar diferentes formatos de mensaje
      if (data.startsWith('{')) {
        // Formato JSON: {"topic": "mi/topico", "message": "mi mensaje"}
        try {
          const parsed = JSON.parse(data);
          if (parsed.topic && parsed.message) {
            // ===== ALMACENAR COMANDOS PARA ESP32 =====
            if (parsed.topic === 'esp32/control') {
              const validCommands = [
                'start', 'stop', 'reset', 'emergency', 
                'valv1_on', 'valv1_off', 'valv2_on', 'valv2_off',
                'bomba1_on', 'bomba1_off', 'bomba2_on', 'bomba2_off'
              ];
              
              if (validCommands.includes(parsed.message)) {
                if (pendingCommands.length < MAX_PENDING_COMMANDS) {
                  pendingCommands.push(parsed.message);
                  console.log(`💾 Comando almacenado desde WS: ${parsed.message}`);
                  console.log(`📊 Comandos pendientes: ${pendingCommands.length}`);
                } else {
                  console.log('❌ Límite de comandos pendientes alcanzado');
                }
              }
            }
            // ===== FIN ALMACENAMIENTO =====
            
            // Procesar datos del sistema
            if (parsed.topic === 'horno/data') {
              try {
                const systemUpdate = JSON.parse(parsed.message);
                updateSystemData(systemUpdate);
              } catch (e) {
                console.log('❌ Error parseando datos del sistema:', e.message);
              }
            }
            
            aedes.publish({
              topic: parsed.topic,
              payload: parsed.message,
              qos: 0,
              retain: false
            });
          }
        } catch (e) {
          console.log('❌ Error parseando JSON:', e.message);
        }
      } else if (data.includes('|')) {
        // Formato: "topico|mensaje"
        const [topic, payload] = data.split('|');
        
        // ===== ALMACENAR COMANDOS PARA ESP32 =====
        if (topic.trim() === 'esp32/control') {
          const validCommands = [
            'start', 'stop', 'reset', 'emergency', 
            'valv1_on', 'valv1_off', 'valv2_on', 'valv2_off',
            'bomba1_on', 'bomba1_off', 'bomba2_on', 'bomba2_off'
          ];
          
          if (validCommands.includes(payload.trim())) {
            if (pendingCommands.length < MAX_PENDING_COMMANDS) {
              pendingCommands.push(payload.trim());
              console.log(`💾 Comando almacenado desde WS: ${payload.trim()}`);
              console.log(`📊 Comandos pendientes: ${pendingCommands.length}`);
            } else {
              console.log('❌ Límite de comandos pendientes alcanzado');
            }
          }
        }
        // ===== FIN ALMACENAMIENTO =====
        
        aedes.publish({
          topic: topic.trim(),
          payload: payload.trim(),
          qos: 0,
          retain: false
        });
      } else {
        // Mensaje simple - publicar en tópico por defecto
        aedes.publish({
          topic: 'esp32/messages',
          payload: data,
          qos: 0,
          retain: false
        });
      }

      // Responder al cliente
      ws.send(JSON.stringify({
        status: 'received',
        clientId: clientId,
        message: data,
        timestamp: new Date().toISOString(),
        pendingCommands: pendingCommands.length
      }));

    } catch (error) {
      console.error('❌ Error procesando mensaje:', error);
    }
  });

  ws.on('close', function () {
    const msg = `❌ Cliente WebSocket desconectado: ${clientId}`;
    console.log(msg);
  });

  // Enviar mensaje de bienvenida
  ws.send(JSON.stringify({
    status: 'connected',
    clientId: clientId,
    message: 'Bienvenido al broker MQTT WebSocket',
    endpoints: {
      mqtt: `wss://${req.headers.host}`,
      simple: `wss://${req.headers.host}/simple`,
      api: `https://${req.headers.host}/api/message`,
      commands: `https://${req.headers.host}/api/esp32-commands`,
      systemData: `https://${req.headers.host}/api/system-data`
    },
    pendingCommands: pendingCommands.length
  }));
});

// ===== FUNCIONES DE LOGGING =====

function broadcastLog(message) {
  const timestamp = new Date().toISOString();
  const logMessage = `[${timestamp}] ${message}`;

  clients.forEach(client => {
    client.write(`data: ${JSON.stringify({message: logMessage})}\n\n`);
  });
}

function broadcastSystemData() {
  clients.forEach(client => {
    client.write(`data: ${JSON.stringify({type: 'systemData', data: systemData})}\n\n`);
  });
}

// Función para actualizar datos del sistema
function updateSystemData(newData) {
  // Actualizar solo las propiedades que existen en systemData
  Object.keys(newData).forEach(key => {
    if (systemData.hasOwnProperty(key)) {
      systemData[key] = newData[key];
    }
  });
  
  systemData.lastUpdate = new Date().toISOString();
  console.log('✅ Datos del sistema actualizados:', systemData);
  
  // Transmitir a clientes SSE
  broadcastSystemData();
}

// Logs de clientes MQTT conectados
aedes.on('client', (client) => {
  const msg = `📡 Cliente MQTT conectado: ${client ? client.id : 'Desconocido'}`;
  console.log(msg);
  broadcastLog(msg);
});

// Logs de mensajes publicados
aedes.on('publish', (packet, client) => {
  // ===== FILTRO: Ignorar mensajes del sistema $SYS/ =====
  if (packet.topic.startsWith('$SYS/')) {
    return; // No procesar mensajes del sistema
  }
  // ===== FIN DEL FILTRO =====

  if (client) {
    const msg = `📩 Mensaje MQTT en '${packet.topic}' por '${client.id}': ${packet.payload.toString()}`;
    console.log(msg);
    broadcastLog(msg);
  } else {
    const msg = `📩 Mensaje en '${packet.topic}': ${packet.payload.toString()}`;
    console.log(msg);
    broadcastLog(msg);
  }
});

// Logs de desconexiones
aedes.on('clientDisconnect', (client) => {
  const msg = `❌ Cliente MQTT desconectado: ${client ? client.id : 'Desconocido'}`;
  console.log(msg);
  broadcastLog(msg);
});

// ===== INICIAR SERVIDOR =====

server.listen(PORT, '0.0.0.0', () => {
  const domain = process.env.RENDER_EXTERNAL_HOSTNAME || `localhost:${PORT}`;

  console.log(`
🚀 Broker MQTT WebSocket ejecutándose en puerto ${PORT}
=======================================================

🔗 Endpoints disponibles:
- MQTT WebSocket estándar: wss://${domain}
- WebSocket simple: wss://${domain}/simple
- API HTTP POST: https://${domain}/api/message
- API Comandos ESP32: https://${domain}/api/esp32-commands
- API Datos del sistema: https://${domain}/api/system-data
- Página web: https://${domain}
- SSE Logs: https://${domain}/events
- Status: https://${domain}/status

📡 Protocolos soportados:
1. MQTT sobre WebSocket (clientes avanzados)
2. WebSocket simple (ESP32, mensajes de texto)
3. HTTP POST API (ESP32, HTTPS seguro)

🌐 Ambiente: ${process.env.NODE_ENV || 'development'}
🕐 Iniciado: ${new Date().toISOString()}

✅ Listo para recibir conexiones...
`);
});

// Manejo de errores no capturados
process.on('uncaughtException', (error) => {
  console.error('❌ Error no capturado:', error);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('❌ Promise rechazada:', reason);
});

// Manejo de cierre graceful
process.on('SIGINT', () => {
  console.log('\n🛑 Cerrando broker gracefulmente...');
  server.close(() => {
    console.log('✅ Broker cerrado correctamente');
    process.exit(0);
  });
});
