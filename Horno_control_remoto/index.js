const express = require('express');
const axios = require('axios');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3001;

// Configuración del broker principal
const BROKER_URL = 'https://tfg-server-ecoview.onrender.com';
const API_ENDPOINT = `${BROKER_URL}/api/message`;

// Almacenamiento de datos del sistema de horno de biomasa
let systemData = {
  // Temperaturas
  temperaturas: [0, 0, 0, 0], // [tanque, horno, cámara, salida]
  
  // Niveles
  niveles: [0, 0, 0], // [vacío, mitad, lleno]
  
  // Presión
  presion: 0,
  
  // Estados de actuadores
  valvula1: false,
  valvula2: false,
  bomba1: false,
  bomba2: false,
  
  // Estado del sistema
  estado: 'SISTEMA_APAGADO',
  emergencia: false,
  bombaActiva: 'PRINCIPAL',
  
  // Información general
  lastUpdate: '--',
  mensajes: []
};

// Middleware
app.use(cors());
app.use(express.json({ limit: '10mb' }));
app.use(express.static('public'));


app.get('/favicon.ico', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'favicon.ico'));
});

// Servir la interfaz web
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// API Proxy para enviar mensajes al broker principal
app.post('/api/send-message', async (req, res) => {
  try {
    const { topic, message } = req.body;

    console.log(`📤 Enviando mensaje al broker principal: ${topic} - ${message}`);

    const response = await axios.post(API_ENDPOINT, {
      topic: topic,
      message: message
    }, {
      headers: {
        'Content-Type': 'application/json'
      },
      timeout: 10000
    });

    // Añadir mensaje al log
    addSystemMessage(`Comando enviado: ${message}`, 'info');

    res.json({
      success: true,
      data: response.data,
      message: 'Mensaje enviado correctamente'
    });

  } catch (error) {
    console.error('Error enviando mensaje:', error.message);
    
    // Añadir mensaje de error al log
    addSystemMessage(`Error enviando comando: ${error.message}`, 'error');
    
    res.status(500).json({
      success: false,
      message: error.response?.data?.message || error.message
    });
  }
});

// Obtener datos del sistema
app.get('/api/system-data', (req, res) => {
  res.json({
    success: true,
    data: systemData,
    timestamp: new Date().toISOString()
  });
});

// Verificar estado del broker principal
app.get('/api/broker-status', async (req, res) => {
  try {
    const response = await axios.get(`${BROKER_URL}/status`, { timeout: 5000 });
    res.json({
      success: true,
      status: 'online',
      data: response.data
    });
  } catch (error) {
    res.json({
      success: false,
      status: 'offline',
      message: error.message
    });
  }
});

// Endpoint para logs (SSE proxy) - Conectar al broker principal
app.get('/api/events', async (req, res) => {
  try {
    console.log('🔗 Conectando a SSE del broker...');
    
    const response = await axios.get(`${BROKER_URL}/events`, {
      responseType: 'stream',
      timeout: 0
    });

    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('X-Accel-Buffering', 'no'); // Importante para Nginx

    response.data.on('data', (chunk) => {
      try {
        const data = chunk.toString();
        console.log('📩 Evento SSE recibido:', data); // DEBUG
        
        // Procesar datos del sistema de horno
        if (data.includes('horno/')) {
          processSystemData(data);
        }
        
        // Reenviar datos al cliente
        res.write(`data: ${JSON.stringify(parseEventData(data))}\n\n`);
      } catch (error) {
        console.error('Error procesando chunk SSE:', error);
      }
    });

    response.data.on('error', (error) => {
      console.error('Error en stream SSE:', error);
      res.end();
    });

    req.on('close', () => {
      console.log('❌ Cliente SSE desconectado');
      response.data.destroy();
      res.end();
    });

  } catch (error) {
    console.error('❌ Error conectando a SSE:', error);
    res.status(500).json({ 
      success: false,
      error: 'Error conectando al broker',
      message: error.message 
    });
  }
});

// Función para analizar y estructurar los datos del evento
function parseEventData(eventData) {
  try {
    // Intentar parsear como JSON primero
    if (eventData.trim().startsWith('{')) {
      return JSON.parse(eventData);
    }
    
    // Buscar datos estructurados en el mensaje
    const dataMatch = eventData.match(/{([^}]+)}/);
    if (dataMatch) {
      const jsonStr = `{${dataMatch[1]}}`;
      return JSON.parse(jsonStr);
    }
    
    // Si es un mensaje simple, devolverlo como está
    return { message: eventData };
  } catch (error) {
    // Si falla el parsing, devolver el mensaje original
    return { message: eventData };
  }
}

// Procesar datos del sistema de horno de biomasa
function processSystemData(eventData) {
  try {
    console.log('📨 Procesando datos del sistema:', eventData);
    
    // Actualizar timestamp
    systemData.lastUpdate = new Date().toLocaleTimeString();
    
    // Buscar datos en diferentes formatos
    if (eventData.includes('temperaturas')) {
      const tempMatch = eventData.match(/temperaturas[^:]*:\s*\[([^\]]+)\]/);
      if (tempMatch && tempMatch[1]) {
        const temps = tempMatch[1].split(',').map(Number);
        if (temps.length === 4) {
          systemData.temperaturas = temps;
          addSystemMessage(`Temperaturas actualizadas: ${temps.join(', ')}°C`, 'info');
        }
      }
    }
    
    if (eventData.includes('niveles')) {
      const nivelesMatch = eventData.match(/niveles[^:]*:\s*\[([^\]]+)\]/);
      if (nivelesMatch && nivelesMatch[1]) {
        const niveles = nivelesMatch[1].split(',').map(Number);
        if (niveles.length === 3) {
          systemData.niveles = niveles;
          addSystemMessage(`Niveles actualizados: ${niveles.join(', ')}%`, 'info');
        }
      }
    }
    
    if (eventData.includes('presion')) {
      const presionMatch = eventData.match(/presion[^:]*:\s*([0-9.]+)/);
      if (presionMatch && presionMatch[1]) {
        systemData.presion = parseFloat(presionMatch[1]);
        addSystemMessage(`Presión actualizada: ${systemData.presion} bar`, 'info');
      }
    }
    
    if (eventData.includes('estado')) {
      const estadoMatch = eventData.match(/estado[^:]*:\s*['"]?([^,'"]+)['"]?/);
      if (estadoMatch && estadoMatch[1]) {
        systemData.estado = estadoMatch[1];
        addSystemMessage(`Estado del sistema: ${estadoMatch[1]}`, 'info');
      }
    }
    
    if (eventData.includes('emergencia')) {
      const emergenciaMatch = eventData.match(/emergencia[^:]*:\s*(true|false)/);
      if (emergenciaMatch) {
        systemData.emergencia = emergenciaMatch[1] === 'true';
        const estado = systemData.emergencia ? 'ACTIVADO' : 'DESACTIVADO';
        addSystemMessage(`Modo emergencia: ${estado}`, 
                         systemData.emergencia ? 'warning' : 'info');
      }
    }
    
    // Estados de actuadores
    ['valvula1', 'valvula2', 'bomba1', 'bomba2'].forEach(actuador => {
      if (eventData.includes(actuador)) {
        const match = eventData.match(new RegExp(`${actuador}[^:]*:\\s*(true|false)`));
        if (match) {
          systemData[actuador] = match[1] === 'true';
          const estado = systemData[actuador] ? 'activado' : 'desactivado';
          addSystemMessage(`${actuador} ${estado}`, 'info');
        }
      }
    });
    
    console.log('✅ Datos del sistema actualizados:', systemData);
    
  } catch (error) {
    console.error('❌ Error procesando datos del sistema:', error);
    addSystemMessage(`Error procesando datos: ${error.message}`, 'error');
  }
}

// Función para añadir mensajes al log del sistema
function addSystemMessage(message, type = 'info') {
  const timestamp = new Date().toLocaleTimeString();
  const logEntry = {
    timestamp,
    message,
    type
  };
  
  systemData.mensajes.unshift(logEntry);
  
  // Mantener sólo los últimos 50 mensajes
  if (systemData.mensajes.length > 50) {
    systemData.mensajes = systemData.mensajes.slice(0, 50);
  }
  
  console.log(`📝 [${type.toUpperCase()}] ${timestamp}: ${message}`);
}

// Endpoint para obtener el historial de mensajes
app.get('/api/messages', (req, res) => {
  res.json({
    success: true,
    messages: systemData.mensajes,
    count: systemData.mensajes.length
  });
});

// Endpoint para limpiar el historial de mensajes
app.delete('/api/messages', (req, res) => {
  systemData.mensajes = [];
  res.json({
    success: true,
    message: 'Historial de mensajes limpiado'
  });
});

// Endpoint de salud del servidor
app.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    timestamp: new Date().toISOString(),
    systemData: {
      estado: systemData.estado,
      emergencia: systemData.emergencia,
      lastUpdate: systemData.lastUpdate
    }
  });
});

// Manejo de errores global
app.use((error, req, res, next) => {
  console.error('Error no manejado:', error);
  addSystemMessage(`Error del servidor: ${error.message}`, 'error');
  
  res.status(500).json({
    success: false,
    error: 'Error interno del servidor',
    message: process.env.NODE_ENV === 'development' ? error.message : 'Something went wrong'
  });
});

// Manejo de rutas no encontradas - CORREGIDO
app.use((req, res) => {
  res.status(404).json({
    success: false,
    error: 'Endpoint no encontrado',
    path: req.path
  });
});

// Iniciar servidor
app.listen(PORT, () => {
  console.log(`🚀 Cliente web ejecutándose en http://localhost:${PORT}`);
  console.log(`🔗 Conectado al broker principal: ${BROKER_URL}`);
  console.log('✅ Listo para recibir datos del sistema de horno de biomasa');
  
  // Mensaje inicial
  addSystemMessage('Servidor iniciado correctamente', 'info');
  addSystemMessage(`Conectado al broker: ${BROKER_URL}`, 'info');
});
