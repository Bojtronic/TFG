const express = require('express');
const path = require('path');
const app = express();
const port = process.env.PORT || 4200;

// Configuración para coincidir exactamente con tu build
const staticPath = path.join(__dirname, 'dist/horno-control/browser');

// Middleware para servir archivos estáticos
app.use(express.static(staticPath, {
  dotfiles: 'ignore',
  etag: true,
  fallthrough: true,
  immutable: true,
  maxAge: '1y'
}));

// Configuración SPA (Single Page Application)
app.get('*', (req, res) => {
  res.sendFile(path.join(staticPath, 'index.html'), {
    headers: {
      'Cache-Control': 'no-cache, no-store, must-revalidate',
      'Pragma': 'no-cache',
      'Expires': '0'
    }
  });
});

// Manejo de errores mejorado
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).send('Error interno del servidor');
});

// Iniciar servidor
app.listen(port, () => {
  console.log(`Servidor escuchando en puerto ${port}`);
  console.log(`Ruta de archivos: ${staticPath}`);
});