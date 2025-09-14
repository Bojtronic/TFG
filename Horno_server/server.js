const http = require('http');
const fs = require('fs');
const path = require('path');

// Importar módulos
const apiRoutes = require('./routes/api');
const webRoutes = require('./routes/web');
const { systemData, updateSystemData, broadcastSystemData } = require('./data/systemData');

const PORT = process.env.PORT || 3000;

let clients = []; // conexiones SSE

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

  // Enrutamiento
  if (req.url.startsWith('/api/') || req.url === '/events') {
    apiRoutes(req, res, clients);
  } else {
    webRoutes(req, res);
  }
});

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

// Exportar clients para que otros módulos puedan usarlo si es necesario
module.exports = { clients };
