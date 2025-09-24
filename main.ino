#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Configuración WiFi
const char* ssid = ""; // Nombre de RED
const char* password = "*"; // Password de RED

WebServer server(80);

// Variables del juego
bool gameActive = false;
int score = 0;
int level = 1;
unsigned long lastDropTime = 0;
int dropInterval = 1000; // ms

// Matriz del tablero (10x20)
const int boardWidth = 10;
const int boardHeight = 20;
int gameBoard[boardWidth][boardHeight] = {0};

// Piezas del Tetris
const int tetrominos[7][4][4] = {
  // I
  {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
  // J
  {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // L
  {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // O
  {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // S
  {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
  // T
  {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // Z
  {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}
};

// Pieza actual
int currentPiece[4][4] = {0};
int currentX = 3;
int currentY = 0;
int currentType = 0;

void setup() {
  Serial.begin(115200);
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // No continuar si falla
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Conectado a WiFi");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();

  
  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/rotate", handleRotate);
  server.on("/down", handleDown);
  server.on("/start", handleStart);
  server.on("/status", handleStatus);
  server.on("/ip", handleIP);
  server.begin();
  
  // Inicializar juego
  randomSeed(analogRead(0));
  newGame();
}

void loop() {
  server.handleClient();
  
  if (gameActive) {
    unsigned long currentTime = millis();
    if (currentTime - lastDropTime > dropInterval) {
      moveDown();
      lastDropTime = currentTime;
    }
  }
}

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tetris ESP32</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Arial', sans-serif;
            background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d);
            color: white;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            max-width: 800px;
            width: 100%;
            background-color: rgba(0, 0, 0, 0.7);
            border-radius: 15px;
            padding: 20px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
        }
        
        header {
            text-align: center;
            margin-bottom: 20px;
        }
        
        h1 {
            font-size: 2.5rem;
            text-shadow: 3px 3px 5px rgba(0, 0, 0, 0.5);
            margin-bottom: 10px;
            color: #ffcc00;
        }
        
        .game-container {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 20px;
        }
        
        .game-info {
            flex: 1;
            min-width: 200px;
            background-color: rgba(0, 0, 0, 0.5);
            padding: 15px;
            border-radius: 10px;
        }
        
        .info-item {
            margin-bottom: 15px;
        }
        
        .info-item p {
            font-size: 1.2rem;
            margin-bottom: 5px;
        }
        
        .info-value {
            font-weight: bold;
            font-size: 1.4rem;
            color: #ffcc00;
        }
        
        .game-board {
            width: 250px;
            height: 500px;
            background-color: #111;
            display: grid;
            grid-template-columns: repeat(10, 1fr);
            grid-template-rows: repeat(20, 1fr);
            gap: 1px;
            border: 3px solid #444;
            border-radius: 5px;
            box-shadow: 0 0 20px rgba(0, 0, 0, 0.8);
        }
        
        .cell {
            background-color: #222;
            border-radius: 2px;
        }
        
        .filled {
            border: 2px solid rgba(255, 255, 255, 0.3);
        }
        
        .current {
            opacity: 0.9;
        }
        
        .controls {
            display: flex;
            flex-direction: column;
            align-items: center;
            margin-top: 20px;
            gap: 10px;
        }
        
        .control-row {
            display: flex;
            gap: 10px;
        }
        
        .btn {
            width: 70px;
            height: 70px;
            font-size: 1.8rem;
            background: linear-gradient(145deg, #2a2a2a, #1a1a1a);
            color: white;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            display: flex;
            justify-content: center;
            align-items: center;
            box-shadow: 3px 3px 5px rgba(0, 0, 0, 0.5);
            transition: all 0.2s;
        }
        
        .btn:hover {
            transform: translateY(-3px);
            box-shadow: 5px 5px 8px rgba(0, 0, 0, 0.5);
        }
        
        .btn:active {
            transform: translateY(1px);
            box-shadow: 1px 1px 3px rgba(0, 0, 0, 0.5);
        }
        
        .start-btn {
            width: 200px;
            height: 60px;
            font-size: 1.5rem;
            background: linear-gradient(145deg, #d35400, #e67e22);
            margin-top: 10px;
        }
        
        .instructions {
            margin-top: 20px;
            background-color: rgba(0, 0, 0, 0.5);
            padding: 15px;
            border-radius: 10px;
            text-align: center;
        }
        
        .instructions h2 {
            color: #ffcc00;
            margin-bottom: 10px;
        }
        
        /* Colores para las piezas de Tetris */
        .piece-1 { background-color: #00ffff; } /* I - Cyan */
        .piece-2 { background-color: #0000ff; } /* J - Azul */
        .piece-3 { background-color: #ff7f00; } /* L - Naranja */
        .piece-4 { background-color: #ffff00; } /* O - Amarillo */
        .piece-5 { background-color: #00ff00; } /* S - Verde */
        .piece-6 { background-color: #800080; } /* T - Púrpura */
        .piece-7 { background-color: #ff0000; } /* Z - Rojo */
        
        @media (max-width: 600px) {
            .game-container {
                flex-direction: column;
            }
            
            .btn {
                width: 60px;
                height: 60px;
                font-size: 1.5rem;
            }
            
            .game-board {
                width: 200px;
                height: 400px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Tetris ESP32</h1>
        </header>
        
        <div class="game-container">
            <div class="game-info">
                <div class="info-item">
                    <p>Estado:</p>
                    <p class="info-value" id="status">En espera</p>
                </div>
                <div class="info-item">
                    <p>Puntuación:</p>
                    <p class="info-value" id="score">0</p>
                </div>
                <div class="info-item">
                    <p>Nivel:</p>
                    <p class="info-value" id="level">1</p>
                </div>
                <div class="info-item">
                    <p>IP ESP32:</p>
                    <p class="info-value" id="ip-address">)=====";
  
  html += WiFi.localIP().toString();
  html += R"=====(</p>
                </div>
            </div>
            
            <div class="game-board" id="board">
                <!-- El tablero se genera mediante JavaScript -->
            </div>
        </div>
        
        <div class="controls">
            <button class="btn start-btn" id="startBtn">Iniciar Juego</button>
            
            <div class="control-row">
                <button class="btn" id="leftBtn">←</button>
                <button class="btn" id="rotateBtn">↻</button>
                <button class="btn" id="rightBtn">→</button>
            </div>
            <div class="control-row">
                <button class="btn" id="downBtn">↓</button>
            </div>
        </div>
        
        <div class="instructions">
            <h2>Controles</h2>
            <p>Usa las flechas del teclado o los botones en pantalla para mover las piezas</p>
            <p>← → : Mover izquierda/derecha | ↻ : Rotar | ↓ : Bajar más rápido</p>
        </div>
    </div>

    <script>
        const boardWidth = 10;
        const boardHeight = 20;
        let gameActive = false;
        
        const boardElement = document.getElementById('board');
        const statusElement = document.getElementById('status');
        const scoreElement = document.getElementById('score');
        const levelElement = document.getElementById('level');
        const startBtn = document.getElementById('startBtn');
        const leftBtn = document.getElementById('leftBtn');
        const rightBtn = document.getElementById('rightBtn');
        const rotateBtn = document.getElementById('rotateBtn');
        const downBtn = document.getElementById('downBtn');

        // Inicializar tablero
        function initializeBoard() {
            boardElement.innerHTML = '';
            for (let y = 0; y < boardHeight; y++) {
                for (let x = 0; x < boardWidth; x++) {
                    const cell = document.createElement('div');
                    cell.className = 'cell';
                    cell.id = 'cell-' + x + '-' + y;
                    boardElement.appendChild(cell);
                }
            }
        }

        // --- CORREGIDO ---
        function updateGameUI(data) {
            statusElement.textContent = data.gameActive ? 'Jugando' : 'En espera';
            scoreElement.textContent = data.score;
            levelElement.textContent = data.level;

            for (let y = 0; y < boardHeight; y++) {
                for (let x = 0; x < boardWidth; x++) {
                    const cell = document.getElementById('cell-' + x + '-' + y);
                    if (cell) {
                        cell.className = 'cell';
                        if (data.board[y] && data.board[y][x] > 0) {
                            cell.classList.add('filled');
                            cell.classList.add('piece-' + data.board[y][x]);
                        }
                    }
                }
            }

            if (data.gameActive) {
                for (let i = 0; i < 4; i++) {
                    for (let j = 0; j < 4; j++) {
                        if (data.currentPiece[i][j] > 0) {
                            const x = data.currentX + i;
                            const y = data.currentY + j;
                            if (y >= 0 && y < boardHeight && x >= 0 && x < boardWidth) {
                                const cell = document.getElementById('cell-' + x + '-' + y);
                                if (cell) {
                                    cell.classList.add('filled');
                                    cell.classList.add('current');
                                    cell.classList.add('piece-' + (data.currentType + 1));
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- CORREGIDO ---
        async function fetchGameState() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                updateGameUI(data);
            } catch (error) {
                console.error('Error obteniendo estado:', error);
                statusElement.textContent = 'Error de conexión';
            } finally {
                setTimeout(fetchGameState, 100); // siempre seguimos consultando
            }
        }

        async function sendCommand(command) {
            try {
                await fetch('/' + command);
            } catch (error) {
                console.error('Error enviando comando ' + command, error);
            }
        }

        async function startGame() {
            await sendCommand('start');
            startBtn.textContent = 'Reiniciar Juego';
            gameActive = true;
        }

        startBtn.addEventListener('click', startGame);
        leftBtn.addEventListener('click', () => sendCommand('left'));
        rightBtn.addEventListener('click', () => sendCommand('right'));
        rotateBtn.addEventListener('click', () => sendCommand('rotate'));
        downBtn.addEventListener('click', () => sendCommand('down'));

        // --- CORREGIDO: ignorar autorepeat ---
        document.addEventListener('keydown', (e) => {
            if (e.repeat) return;
            if (!gameActive) return;

            switch(e.key) {
                case 'ArrowLeft': sendCommand('left'); break;
                case 'ArrowRight': sendCommand('right'); break;
                case 'ArrowUp': sendCommand('rotate'); break;
                case 'ArrowDown': sendCommand('down'); break;
            }
        });

        initializeBoard();
        fetchGameState();
    </script>
</body>
</html>
)=====";
  
  server.send(200, "text/html", html);
}

void handleIP() {
  server.send(200, "text/plain", WiFi.localIP().toString());
}

void handleStatus() {
  String json = "{";
  json += "\"gameActive\":" + String(gameActive ? "true" : "false") + ",";
  json += "\"score\":" + String(score) + ",";
  json += "\"level\":" + String(level) + ",";
  json += "\"currentType\":" + String(currentType) + ",";
  json += "\"currentX\":" + String(currentX) + ",";
  json += "\"currentY\":" + String(currentY) + ",";
  json += "\"board\":[";
  for (int y = 0; y < boardHeight; y++) {
    json += "[";
    for (int x = 0; x < boardWidth; x++) {
      json += String(gameBoard[x][y]);
      if (x < boardWidth - 1) json += ",";
    }
    json += "]";
    if (y < boardHeight - 1) json += ",";
  }
  json += "],\"currentPiece\":[";
  for (int i = 0; i < 4; i++) {
    json += "[";
    for (int j = 0; j < 4; j++) {
      json += String(currentPiece[i][j]);
      if (j < 3) json += ",";
    }
    json += "]";
    if (i < 3) json += ",";
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}

void handleLeft() {
  if (gameActive) {
    moveLeft();
  }
  server.send(200, "text/plain", "OK");
}

void handleRight() {
  if (gameActive) {
    moveRight();
  }
  server.send(200, "text/plain", "OK");
}

void handleRotate() {
  if (gameActive) {
    rotatePiece();
  }
  server.send(200, "text/plain", "OK");
}

void handleDown() {
  if (gameActive) {
    moveDown();
  }
  server.send(200, "text/plain", "OK");
}

void handleStart() {
  newGame();
  gameActive = true;
  server.send(200, "text/plain", "OK");
}

void newGame() {
  // Reiniciar tablero
  for (int x = 0; x < boardWidth; x++) {
    for (int y = 0; y < boardHeight; y++) {
      gameBoard[x][y] = 0;
    }
  }
  
  score = 0;
  level = 1;
  dropInterval = 1000;
  generateNewPiece();
  lastDropTime = millis();
}

void generateNewPiece() {
  currentType = random(7);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      currentPiece[i][j] = tetrominos[currentType][i][j];
    }
  }
  currentX = 3;
  currentY = 0;
}

void moveLeft() {
  if (checkCollision(currentX - 1, currentY, currentPiece)) return;
  currentX--;
}

void moveRight() {
  if (checkCollision(currentX + 1, currentY, currentPiece)) return;
  currentX++;
}

void moveDown() {
  if (checkCollision(currentX, currentY + 1, currentPiece)) {
    mergePiece();       // fusionar pieza actual
    checkLines();       // limpiar líneas completas
    generateNewPiece(); // nueva pieza
    if (checkCollision(currentX, currentY, currentPiece)) {
      gameActive = false; // game over
    }
  } else {
    currentY++; // solo bajar si no hay colisión
  }
}

void rotatePiece() {
  // No rotar la pieza O (índice 3 en tetrominos)
  if (currentType == 3) {
    return;
  }

  int rotated[4][4];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      // rotación en sentido horario
      rotated[i][j] = currentPiece[j][3 - i];
    }
  }

  // Solo aplicar si no hay colisión
  if (!checkCollision(currentX, currentY, rotated)) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        currentPiece[i][j] = rotated[i][j];
      }
    }
  }
}


bool checkCollision(int x, int y, int piece[4][4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (piece[i][j] != 0) {
        int boardX = x + i;
        int boardY = y + j;
        if (boardX < 0 || boardX >= boardWidth || boardY >= boardHeight) {
          return true;
        }
        if (boardY >= 0 && gameBoard[boardX][boardY] != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

void mergePiece() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (currentPiece[i][j] != 0) {
        int boardX = currentX + i;
        int boardY = currentY + j;
        if (boardY >= 0) {
          gameBoard[boardX][boardY] = currentType + 1;
        }
      }
    }
  }
}

void checkLines() {
  for (int y = boardHeight - 1; y >= 0; y--) {
    bool lineComplete = true;
    for (int x = 0; x < boardWidth; x++) {
      if (gameBoard[x][y] == 0) {
        lineComplete = false;
        break;
      }
    }
    if (lineComplete) {
      // Eliminar línea
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < boardWidth; x++) {
          gameBoard[x][yy] = gameBoard[x][yy-1];
        }
      }
      // Limpiar línea superior
      for (int x = 0; x < boardWidth; x++) {
        gameBoard[x][0] = 0;
      }
      score += 100;
      // Aumentar velocidad cada 500 puntos
      if (score % 500 == 0 && dropInterval > 100) {
        level++;
        dropInterval -= 100;
      }
      y++; // Revisar la misma posición otra vez
    }
  }
}
