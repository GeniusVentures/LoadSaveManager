const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');
const https = require('https');
//const http = require('http');

const activeSockets = [];
const serverOptions = {
    key: fs.readFileSync('./certs/key.pem'),
    cert: fs.readFileSync('./certs/cert.pem')
};

/* const serverOptions_nonsec = {
}; */

// Non-secure WebSocket server
/* const nonSecureServer = new WebSocket.Server({
    noServer: true,
    verifyClient: (info, cb) => {
        console.log('WebSocket handshake received:');
        console.log('Request origin:', info.origin);
        console.log('Request secure:', info.secure);
        console.log('Request resource:', info.req.url);
        console.log('Request headers:', info.req.headers);

        // Validate and sanitize the resource request
        const sanitizedPath = sanitizeResourcePath(info.req.url);
        if (sanitizedPath === null) {
            // Invalid resource request, reject the connection
            cb(false, 400, 'Bad Request');
            return;
        }
		const requestedResource = info.req.url;
        // Store the WebSocket connection along with the associated file path
        const filePath = __dirname + sanitizedPath; // Adjust the base directory accordingly
        const socket = { connection: info.req.connection, filePath: filePath };
        activeSockets.push(socket);

        cb(true, 200, 'OK');
    }
}); */

// Secure WebSocket server
const secureServer = new WebSocket.Server({
    noServer: true,
    verifyClient: (info, cb) => {
        console.log('WebSocket handshake received:');
        console.log('Request origin:', info.origin);
        console.log('Request secure:', info.secure);
        console.log('Request resource:', info.req.url);
        console.log('Request headers:', info.req.headers);

        // Validate and sanitize the resource request
        const sanitizedPath = sanitizeResourcePath(info.req.url);
        if (sanitizedPath === null) {
            // Invalid resource request, reject the connection
            cb(false, 400, 'Bad Request');
            return;
        }
		const requestedResource = info.req.url;
        // Store the WebSocket connection along with the associated file path
        const filePath = __dirname + sanitizedPath; // Adjust the base directory accordingly
        const socket = { connection: info.req.connection, filePath: filePath };
        activeSockets.push(socket);

        cb(true, 200, 'OK');
    }
});

const httpsServer = https.createServer(serverOptions, (req, res) => {
    // Your regular HTTP server logic
    res.writeHead(200);
    res.end('Hello, this is a secure server!');
});

httpsServer.on('upgrade', (request, socket, head) => {
    const pathname = request.url;
	secureServer.handleUpgrade(request, socket, head, (ws) => {
		secureServer.emit('connection', ws, request);
	});
});

httpsServer.listen(8090, () => {
    console.log('Server listening on port 8090');
});


/* const httpServer = http.createServer(serverOptions_nonsec, (req, res) => {
    // Your regular HTTP server logic
    res.writeHead(200);
    res.end('Hello, this is a secure server!');
});

httpServer.on('upgrade', (request, socket, head) => {
    const pathname = request.url;
	console.log('Upgrade now?', pathname);
	nonSecureServer.handleUpgrade(request, socket, head, (ws) => {
		nonSecureServer.emit('connection', ws, request);
	});
});

httpServer.listen(8080, () => {
    console.log('Server listening on port 8080');
}); */


/* nonSecureServer.on('connection', (socket) => {
    // Non-secure WebSocket connection logic
    console.log('Client connected');

    socket.on('message', (message) => {
        console.log(`Received message: ${message}`);
        // Handle different types of messages
        if (String(message) === 'GET_FILE_SIZE') {
            // Find the associated file path for this WebSocket connection
            const socketInfo = activeSockets.find((s) => s.connection === socket._socket);
            if (socketInfo) {
                const filePath = socketInfo.filePath;
				// Log out socket information
				console.log(`Sending file size to socket: ${socket._socket.remoteAddress}:${socket._socket.remotePort}`);
                // Respond with the size of the file
                fs.stat(filePath, (err, stats) => {
                    if (err) {
                        console.error('Error getting file size:', err);
                        return;
                    }
                    const fileSize = stats.size.toString();
					const response = `${fileSize}\n`;
					console.log('send file size:', fileSize);
                    socket.send(response);
                });
            }
        }
		if(String(message) === 'GET_FILE') {
            // Find the associated file path for this WebSocket connection
            const socketInfo = activeSockets.find((s) => s.connection === socket._socket);
            if (socketInfo) {
                const filePath = socketInfo.filePath;
				// Log out socket information
				console.log(`Sending file size to socket: ${socket._socket.remoteAddress}:${socket._socket.remotePort}`);
                // Respond with the file
				fs.readFile(filePath, (err, data) => {
                    if (err) {
                        console.error('Error getting file:', err);
                        return;
                    }
					//const response = `${data}WSEOF`;
					const delimiterBuffer = Buffer.from('WSEOF');
					const response = Buffer.concat([data, delimiterBuffer]);
                    socket.send(response);
                });
            }			
		}else {
            // Handle other types of messages as needed
        }
	});
}); */

secureServer.on('connection', (socket) => {
    // Secure WebSocket connection logic
    console.log('Client connected');

    socket.on('message', (message) => {
        console.log(`Received message: ${message}`);
        // Handle different types of messages
        if (String(message) === 'GET_FILE_SIZE') {
            // Find the associated file path for this WebSocket connection
            const socketInfo = activeSockets.find((s) => s.connection === socket._socket);
            if (socketInfo) {
                const filePath = socketInfo.filePath;
				// Log out socket information
				console.log(`Sending file size to socket: ${socket._socket.remoteAddress}:${socket._socket.remotePort}`);
                // Respond with the size of the file
                fs.stat(filePath, (err, stats) => {
                    if (err) {
                        console.error('Error getting file size:', err);
                        return;
                    }
                    const fileSize = stats.size.toString();
					const response = `${fileSize}\n`;
					console.log('send file size:', fileSize);
                    socket.send(response);
                });
            }
        }
		if(String(message) === 'GET_FILE') {
            // Find the associated file path for this WebSocket connection
            const socketInfo = activeSockets.find((s) => s.connection === socket._socket);
            if (socketInfo) {
                const filePath = socketInfo.filePath;
				// Log out socket information
				console.log(`Sending file size to socket: ${socket._socket.remoteAddress}:${socket._socket.remotePort}`);
                // Respond with the file
				fs.readFile(filePath, (err, data) => {
                    if (err) {
                        console.error('Error getting file:', err);
                        return;
                    }
					//const response = `${data}WSEOF`;
					const delimiterBuffer = Buffer.from('WSEOF');
					const response = Buffer.concat([data, delimiterBuffer]);
                    socket.send(response);
                });
            }			
		}else {
            // Handle other types of messages as needed
        }
	});
});

function sanitizeResourcePath(resourcePath) {
    // Prevent traversal attacks by normalizing and making sure path starts with /resources/

    const normalizedPath = path.normalize(resourcePath);
    const isValidPath = normalizedPath.startsWith('\\resources\\') || normalizedPath.startsWith('/resources/');

    return isValidPath ? normalizedPath : null;
}