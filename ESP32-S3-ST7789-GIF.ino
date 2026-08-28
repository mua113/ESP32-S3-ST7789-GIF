/*
 * ESP32-S3 ST7789 GIF Animation Player with WiFi
 * 240x240 LCD Display
 * WiFi Upload Web Interface
 * 
 * Pins:
 * GPIO12 (MOSI) → SDA
 * GPIO13 (SCK)  → SCL
 * GPIO10        → DC
 * GPIO11        → RST
 * GPIO9         → CS
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include "AnimatedGIF.h"

// WiFi credentials
const char* ssid = "FPT Bao Minh";
const char* password = "123456789";

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;
WebServer server(80);

bool gifLoaded = false;
String currentGifFile = "/animation.gif";

// Callback function to draw GIF frame
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s = pDraw->pPixels;
  uint16_t *d = (uint16_t *)malloc(pDraw->iWidth * 2);
  
  if (d) {
    for (int x = 0; x < pDraw->iWidth; x++) {
      uint8_t ucPixel = s[x];
      d[x] = pDraw->pPalette[ucPixel];
    }
    tft.pushImage(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight, d);
    free(d);
  }
}

// ============= WEB SERVER HANDLERS =============

// HTML page for GIF upload
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-S3 GIF Upload</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
        }
        .container {
            background: white;
            border-radius: 10px;
            padding: 30px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 10px;
        }
        .info {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .upload-area {
            border: 3px dashed #667eea;
            border-radius: 8px;
            padding: 40px;
            text-align: center;
            cursor: pointer;
            background: #f8f9ff;
            transition: all 0.3s ease;
        }
        .upload-area:hover {
            border-color: #764ba2;
            background: #f0f2ff;
        }
        .upload-area.dragover {
            border-color: #764ba2;
            background: #e8ebff;
        }
        input[type="file"] {
            display: none;
        }
        .upload-icon {
            font-size: 48px;
            margin-bottom: 15px;
        }
        .upload-text {
            font-size: 16px;
            color: #333;
            margin-bottom: 5px;
        }
        .upload-hint {
            font-size: 12px;
            color: #999;
        }
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            margin-top: 20px;
            width: 100%;
            transition: transform 0.2s;
        }
        button:hover {
            transform: translateY(-2px);
        }
        button:active {
            transform: translateY(0);
        }
        .progress {
            display: none;
            margin-top: 20px;
            text-align: center;
        }
        .progress-bar {
            width: 100%;
            height: 30px;
            background: #e0e0e0;
            border-radius: 15px;
            overflow: hidden;
            margin-bottom: 10px;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
            width: 0%;
            transition: width 0.3s;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-size: 12px;
        }
        .status {
            margin-top: 20px;
            padding: 15px;
            border-radius: 5px;
            text-align: center;
            display: none;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
            display: block;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
            display: block;
        }
        .file-list {
            margin-top: 30px;
            padding-top: 20px;
            border-top: 1px solid #ddd;
        }
        .file-list h3 {
            color: #333;
            margin-bottom: 15px;
        }
        .file-item {
            background: #f8f9ff;
            padding: 10px;
            margin: 8px 0;
            border-radius: 5px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .file-name {
            color: #667eea;
            font-weight: bold;
        }
        .delete-btn {
            background: #dc3545;
            color: white;
            border: none;
            padding: 5px 10px;
            border-radius: 3px;
            cursor: pointer;
            font-size: 12px;
        }
        .delete-btn:hover {
            background: #c82333;
        }
        .info-box {
            background: #e8f4f8;
            border-left: 4px solid #0066cc;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 3px;
        }
        .info-box p {
            margin: 5px 0;
            color: #333;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎬 ESP32-S3 GIF Player</h1>
        <div class="info">LCD: 240x240 | ST7789 | WiFi Enabled</div>
        
        <div class="info-box">
            <p><strong>📋 Hướng dẫn:</strong></p>
            <p>✓ Chọn file GIF (dùng animated GIF tối ưu)</p>
            <p>✓ Kích thước GIF nên ≤ 240x240 pixels</p>
            <p>✓ Dung lượng file tối đa 2MB</p>
        </div>

        <div class="upload-area" id="uploadArea">
            <div class="upload-icon">📁</div>
            <div class="upload-text">Kéo thả file GIF tại đây</div>
            <div class="upload-hint">hoặc click để chọn</div>
            <input type="file" id="fileInput" accept=".gif" />
        </div>

        <button id="uploadBtn">Tải lên</button>

        <div class="progress" id="progress">
            <div class="progress-bar">
                <div class="progress-fill" id="progressFill">0%</div>
            </div>
            <span id="progressText">Đang tải...</span>
        </div>

        <div class="status" id="status"></div>

        <div class="file-list" id="fileList">
            <h3>📂 File trong thiết bị:</h3>
            <div id="fileItems"></div>
        </div>
    </div>

    <script>
        const uploadArea = document.getElementById('uploadArea');
        const fileInput = document.getElementById('fileInput');
        const uploadBtn = document.getElementById('uploadBtn');
        const progress = document.getElementById('progress');
        const progressFill = document.getElementById('progressFill');
        const progressText = document.getElementById('progressText');
        const status = document.getElementById('status');

        uploadArea.addEventListener('click', () => fileInput.click());
        
        uploadArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            uploadArea.classList.add('dragover');
        });
        
        uploadArea.addEventListener('dragleave', () => {
            uploadArea.classList.remove('dragover');
        });
        
        uploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            uploadArea.classList.remove('dragover');
            fileInput.files = e.dataTransfer.files;
            if (fileInput.files.length > 0) {
                uploadFile();
            }
        });

        fileInput.addEventListener('change', () => {
            if (fileInput.files.length > 0) {
                uploadFile();
            }
        });

        uploadBtn.addEventListener('click', uploadFile);

        function uploadFile() {
            const file = fileInput.files[0];
            if (!file) return;

            if (!file.name.endsWith('.gif')) {
                showStatus('Vui lòng chọn file GIF!', 'error');
                return;
            }

            const formData = new FormData();
            formData.append('file', file);

            progress.style.display = 'block';
            status.style.display = 'none';
            uploadBtn.disabled = true;

            const xhr = new XMLHttpRequest();

            xhr.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    progressFill.style.width = percentComplete + '%';
                    progressFill.textContent = Math.round(percentComplete) + '%';
                    progressText.textContent = `Đang tải: ${Math.round(e.loaded / 1024)} KB / ${Math.round(e.total / 1024)} KB`;
                }
            });

            xhr.addEventListener('load', () => {
                if (xhr.status === 200) {
                    showStatus('✓ Tải lên thành công! GIF đang chạy...', 'success');
                    fileInput.value = '';
                    loadFileList();
                } else {
                    showStatus('✗ Lỗi tải lên: ' + xhr.statusText, 'error');
                }
                progress.style.display = 'none';
                uploadBtn.disabled = false;
            });

            xhr.addEventListener('error', () => {
                showStatus('✗ Lỗi kết nối!', 'error');
                progress.style.display = 'none';
                uploadBtn.disabled = false;
            });

            xhr.open('POST', '/upload', true);
            xhr.send(formData);
        }

        function showStatus(message, type) {
            status.textContent = message;
            status.className = 'status ' + type;
        }

        function loadFileList() {
            fetch('/files')
                .then(r => r.json())
                .then(data => {
                    const fileItems = document.getElementById('fileItems');
                    fileItems.innerHTML = '';
                    
                    if (data.files.length === 0) {
                        fileItems.innerHTML = '<p style="color:#999;">Không có file</p>';
                        return;
                    }

                    data.files.forEach(file => {
                        const item = document.createElement('div');
                        item.className = 'file-item';
                        item.innerHTML = `
                            <span class="file-name">${file.name} (${file.size} bytes)</span>
                            <button class="delete-btn" onclick="deleteFile('${file.name}')">Xóa</button>
                        `;
                        fileItems.appendChild(item);
                    });
                });
        }

        function deleteFile(filename) {
            if (!confirm('Xóa file này?')) return;
            
            fetch('/delete?name=' + encodeURIComponent(filename), { method: 'GET' })
                .then(r => r.json())
                .then(data => {
                    showStatus(data.message, data.success ? 'success' : 'error');
                    loadFileList();
                });
        }

        loadFileList();
    </script>
</body>
</html>
  )";
  server.send(200, "text/html; charset=utf-8", html);
}

// Handle file upload
void handleUpload() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    Serial.printf("Upload Start: %s\n", filename.c_str());
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf("Writing %d bytes\n", upload.currentSize);
    
  } else if (upload.status == UPLOAD_FILE_END) {
    File file = SPIFFS.open("/animation.gif", "w");
    if (file) {
      file.close();
      Serial.println("File saved successfully!");
      
      // Reload GIF
      gif.close();
      delay(100);
      if (gif.open("/animation.gif", GIFDraw)) {
        gifLoaded = true;
        Serial.println("GIF reloaded!");
      }
    }
    server.send(200, "text/json", "{\"success\": true}");
  }
}

// Get file list
void handleFileList() {
  String json = "{\"files\": [";
  bool first = true;
  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  while (file) {
    if (file.name()[0] != '.') {
      if (!first) json += ",";
      json += "{\"name\": \"" + String(file.name()) + "\", \"size\": " + String(file.size()) + "}";
      first = false;
    }
    file = root.openNextFile();
  }
  
  json += "]}";
  server.send(200, "application/json; charset=utf-8", json);
}

// Delete file
void handleDelete() {
  if (!server.hasArg("name")) {
    server.send(400, "text/json", "{\"success\": false, \"message\": \"Missing filename\"}");
    return;
  }
  
  String filename = "/" + server.arg("name");
  if (SPIFFS.remove(filename)) {
    server.send(200, "text/json", "{\"success\": true, \"message\": \"File deleted\"}");
  } else {
    server.send(500, "text/json", "{\"success\": false, \"message\": \"Delete failed\"}");
  }
}

// Display WiFi info on LCD
void displayWiFiInfo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  
  tft.drawString("WiFi Connected!", 50, 50);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("SSID: " + String(ssid), 20, 80);
  tft.drawString("IP: " + WiFi.localIP().toString(), 20, 100);
  tft.drawString("Web: http://" + WiFi.localIP().toString(), 10, 130);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Ready!", 70, 170);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32-S3 ST7789 GIF Player with WiFi ===");
  Serial.printf("Display: 240x240\n");
  Serial.println("Initializing TFT...");
  
  // Initialize TFT
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Initializing...", 50, 100);
  
  delay(500);
  
  // Initialize SPIFFS
  Serial.println("Mounting SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: SPIFFS Mount Failed!");
    tft.fillScreen(TFT_RED);
    tft.drawString("SPIFFS Error!", 40, 100);
    delay(3000);
    return;
  }
  Serial.println("SPIFFS mounted successfully!");
  
  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Connecting to WiFi...", 30, 80);
  tft.drawString(ssid, 30, 100);
  
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    displayWiFiInfo();
    delay(3000);
  } else {
    Serial.println("WiFi connection failed!");
    tft.fillScreen(TFT_RED);
    tft.drawString("WiFi Failed!", 50, 100);
    delay(3000);
  }
  
  // Setup web server
  Serial.println("Starting Web Server...");
  server.on("/", handleRoot);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/files", handleFileList);
  server.on("/delete", handleDelete);
  server.begin();
  
  Serial.println("Web server started!");
  Serial.printf("Open http://%s in browser\n", WiFi.localIP().toString().c_str());
  
  // Try to load GIF
  Serial.println("\nLoading GIF...");
  if (gif.open("/animation.gif", GIFDraw)) {
    gifLoaded = true;
    Serial.printf("GIF loaded: %dx%d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    tft.fillScreen(TFT_BLACK);
  } else {
    Serial.println("No GIF found. Upload one using web interface.");
    tft.fillScreen(TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString("Upload GIF via Web", 30, 100);
    tft.drawString("http://" + WiFi.localIP().toString(), 20, 120);
  }
}

void loop() {
  server.handleClient();  // Handle web requests
  
  if (gifLoaded && gif.playFrame(true, NULL)) {
    delay(gif.getLastDelay());
  } else if (gifLoaded) {
    gif.reset();  // Loop back
  } else {
    delay(100);  // Wait if no GIF loaded
  }
}
