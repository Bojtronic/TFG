// Almacenamiento de datos del sistema de horno
let systemData = {
  temperaturas: [0, 0, 0, 0],
  nivelTanque: 0,
  //niveles: [0, 0, 0],
  presion: 0,
  valvula1: false,
  valvula2: false,
  bomba1: false,
  bomba2: false,
  estado: 0,
  emergencia: false,
  bombaActiva: 'PRINCIPAL',
  lastUpdate: new Date().toISOString()
};

// Función para actualizar datos del sistema
function updateSystemData(newData) {
  Object.keys(newData).forEach(key => {
    if (systemData.hasOwnProperty(key)) {
      systemData[key] = newData[key];
    }
  });
  
  systemData.lastUpdate = new Date().toISOString();
  console.log('✅ Datos del sistema actualizados');
}

// Transmitir datos a clientes SSE
function broadcastSystemData(clients) {
  clients.forEach(client => {
    client.write(`data: ${JSON.stringify({type: 'systemData', data: systemData})}\n\n`);
  });
}

module.exports = {
  systemData,
  updateSystemData,
  broadcastSystemData
};
