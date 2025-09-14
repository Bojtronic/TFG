const fs = require('fs');
const path = require('path');
const { systemData } = require('../data/systemData');

function handleWebRoutes(req, res) {
  // Servir la aplicación web
  if (req.url === '/' || req.url === '/index.html') {
    serveWebApp(req, res);
  } 
  // Servir archivos estáticos
  else if (req.url === '/styles.css') {
    serveStyles(req, res);
  } 
  else if (req.url === '/script.js') {
    serveScript(req, res);
  }
  else if (req.url === '/favicon.ico') {
    serveFavicon(req, res);
  }
  else if (req.url.match(/\.(png|jpg|jpeg|gif|svg)$/i)) {
    serveStaticFile(req, res);
  } 
  // Ruta no encontrada
  else {
    res.writeHead(404);
    res.end('Not found');
  }
}

// Función para servir la aplicación web
function serveWebApp(req, res) {
  fs.readFile(path.join(__dirname, '../public/index.html'), 'utf8', (err, html) => {
    if (err) {
      res.writeHead(500);
      res.end('Error loading page');
      return;
    }
    
    res.writeHead(200, { "Content-Type": "text/html" });
    res.end(html);
  });
}

// Función para servir estilos
function serveStyles(req, res) {
  fs.readFile(path.join(__dirname, '../public/styles.css'), 'utf8', (err, css) => {
    if (err) {
      res.writeHead(500);
      res.end('Error loading styles');
      return;
    }
    
    res.writeHead(200, { "Content-Type": "text/css" });
    res.end(css);
  });
}

// Función para servir script
function serveScript(req, res) {
  fs.readFile(path.join(__dirname, '../public/script.js'), 'utf8', (err, js) => {
    if (err) {
      res.writeHead(500);
      res.end('Error loading script');
      return;
    }
    
    res.writeHead(200, { "Content-Type": "application/javascript" });
    res.end(js);
  });
}

// Función para servir favicon
function serveFavicon(req, res) {
  const faviconPath = path.join(__dirname, '../public/favicon.ico');
  
  fs.readFile(faviconPath, (err, data) => {
    if (err) {
      // Si no hay favicon, servir uno por defecto (opcional)
      res.writeHead(204); // No Content
      res.end();
      return;
    }
    
    res.writeHead(200, {
      'Content-Type': 'image/x-icon',
      'Cache-Control': 'public, max-age=86400' // Cache por 24 horas
    });
    res.end(data);
  });
}

// Función para servir archivos estáticos
function serveStaticFile(req, res) {
  const filePath = path.join(__dirname, '../public', req.url);
  const ext = path.extname(filePath);

  const contentTypes = {
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.gif': 'image/gif',
    '.svg': 'image/svg+xml'
  };

  fs.readFile(filePath, (err, data) => {
    if (err) {
      res.writeHead(404);
      res.end('File not found');
    } else {
      res.writeHead(200, {
        'Content-Type': contentTypes[ext] || 'application/octet-stream'
      });
      res.end(data);
    }
  });
}

module.exports = handleWebRoutes;
