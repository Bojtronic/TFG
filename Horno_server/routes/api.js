const { URLSearchParams } = require('url');
const { systemData, updateSystemData, broadcastSystemData } = require('../data/systemData');

let pendingCommands = [];
const MAX_PENDING_COMMANDS = 10;

function handleApiRoutes(req, res, clients) {
  // SSE para actualizaciones en tiempo real
  if (req.url === '/events') {
    handleSSE(req, res, clients);
    return;
  }

  // Endpoint para comandos ESP32
  if (req.url === '/api/esp32-commands' && req.method === 'GET') {
    handleEsp32Commands(req, res);
    return;
  }
  // Endpoint para datos del sistema
  else if (req.url === '/api/system-data' && req.method === 'GET') {
    handleSystemData(req, res);
    return;
  }
  // Endpoint para recibir datos del ESP32
  else if (req.url === '/api/message' && req.method === 'POST') {
    handlePostMessage(req, res, clients);
    return;
  }
  // Ruta no encontrada
  else {
    res.writeHead(404);
    res.end('Not found');
    return;
  }
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
function handlePostMessage(req, res, clients) {
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
            'start', 'stop', 'manual', 'emergency',
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
            const nivelTanque = parseInt(params.get('nivelTanque'));
            //const niveles = params.get('niveles').match(/[\d.]+/g).map(Number);
            const presion = parseFloat(params.get('presion'));
            const valvula1 = params.get('valvula1') === 'true';
            const valvula2 = params.get('valvula2') === 'true';
            const bomba1 = params.get('bomba1') === 'true';
            const bomba2 = params.get('bomba2') === 'true';
            const estado = parseInt(params.get('estado'));
            const mensaje = parseInt(params.get('menasaje'));
            //const emergencia = params.get('emergencia') === 'true';
            //const bombaActiva = params.get('bombaActiva');

            // Actualizar datos del sistema
            const systemUpdate = {
              temperaturas,
              nivelTanque,
              //niveles,
              presion,
              valvula1,
              valvula2,
              bomba1,
              bomba2,
              estado,
              mensaje,
              //emergencia,
              //bombaActiva,
              lastUpdate: new Date().toISOString()
            };


            updateSystemData(systemUpdate);
            broadcastSystemData(clients);
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
function handleSSE(req, res, clients) {
  console.log('📡 Nuevo cliente SSE conectado');
  res.writeHead(200, {
    'Content-Type': 'text/event-stream',
    'Cache-Control': 'no-cache',
    'Connection': 'keep-alive',
    'Access-Control-Allow-Origin': '*'
  });
  res.write('\n');

  // Guardar referencia a la respuesta
  clients.push(res);

  // Enviar datos iniciales
  res.write(`data: ${JSON.stringify({ type: 'systemData', data: systemData })}\n\n`);

  req.on('close', () => {
    console.log('❌ Cliente SSE desconectado');
    const index = clients.indexOf(res);
    if (index !== -1) {
      clients.splice(index, 1);
    }
  });
}

module.exports = handleApiRoutes;
