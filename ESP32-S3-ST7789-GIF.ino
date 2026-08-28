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

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include "AnimatedGIF.h"

using namespace fs;

// WiFi credentials
const char* ssid = "FPT Bao Minh";
const char* password = "123456789";

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;
WebServer server(80);

bool gifLoaded = false;
int gifDelay = 20;

// Forward declarations
void handleRoot();
void handleUpload();
void handleFileList();
void handleDelete();
void displayWiFiInfo();

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
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>ESP32-S3 GIF Upload</title><style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh}";
  html += ".container{background:white;border-radius:10px;padding:30px;box-shadow:0 10px 30px rgba(0,0,0,0.3)}";
  html += "h1{color:#333;text-align:center;margin-bottom:10px}";
  html += ".info{text-align:center;color:#666;margin-bottom:30px;font-size:14px}";
  html += ".upload-area{border:3px dashed #667eea;border-radius:8px;padding:40px;text-align:center;cursor:pointer;background:#f8f9ff;transition:all 0.3s}";
  html += ".upload-area:hover{border-color:#764ba2;background:#f0f2ff}";
  html += ".upload-area.dragover{border-color:#764ba2;background:#e8ebff}";
  html += "input[type=\"file\"]{display:none}";
  html += ".upload-icon{font-size:48px;margin-bottom:15px}";
  html += ".upload-text{font-size:16px;color:#333;margin-bottom:5px}";
  html += ".upload-hint{font-size:12px;color:#999}";
  html += "button{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;padding:12px 30px;border-radius:5px;font-size:16px;cursor:pointer;margin-top:20px;width:100%;transition:transform 0.2s}";
  html += "button:hover{transform:translateY(-2px)}button:active{transform:translateY(0)}button:disabled{opacity:0.6;cursor:not-allowed}";
  html += ".progress{display:none;margin-top:20px;text-align:center}";
  html += ".progress-bar{width:100%;height:30px;background:#e0e0e0;border-radius:15px;overflow:hidden;margin-bottom:10px}";
  html += ".progress-fill{height:100%;background:linear-gradient(90deg,#667eea 0%,#764ba2 100%);width:0%;transition:width 0.3s;display:flex;align-items:center;justify-content:center;color:white;font-size:12px}";
  html += ".status{margin-top:20px;padding:15px;border-radius:5px;text-align:center;display:none}";
  html += ".status.success{background:#d4edda;color:#155724;border:1px solid #c3e6cb;display:block}";
  html += ".status.error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb;display:block}";
  html += ".file-list{margin-top:30px;padding-top:20px;border-top:1px solid #ddd}";
  html += ".file-list h3{color:#333;margin-bottom:15px}";
  html += ".file-item{background:#f8f9ff;padding:10px;margin:8px 0;border-radius:5px;display:flex;justify-content:space-between;align-items:center}";
  html += ".file-name{color:#667eea;font-weight:bold}";
  html += ".delete-btn{background:#dc3545;color:white;border:none;padding:5px 10px;border-radius:3px;cursor:pointer;font-size:12px}";
  html += ".delete-btn:hover{background:#c82333}";
  html += ".info-box{background:#e8f4f8;border-left:4px solid #0066cc;padding:15px;margin-bottom:20px;border-radius:3px}";
  html += ".info-box p{margin:5px 0;color:#333;font-size:14px}";
  html += "</style></head><body><div class=\"container\">";
  html += "<h1>GIF Player ESP32-S3</h1>";
  html += "<div class=\"info\">LCD: 240x240 | ST7789 | WiFi Enabled</div>";
  html += "<div class=\"info-box\"><p><strong>Instructions:</strong></p>";
  html += "<p>Select animated GIF file (max 2MB)</p>";
  html += "<p>Recommended size: 240x240 pixels</p></div>";
  html += "<div class=\"upload-area\" id=\"uploadArea\">";
  html += "<div class=\"upload-icon\">📁</div>";
  html += "<div class=\"upload-text\">Drag GIF file here</div>";
  html += "<div class=\"upload-hint\">or click to select</div>";
  html += "<input type=\"file\" id=\"fileInput\" accept=\".gif\">";
  html += "</div>";
  html += "<button id=\"uploadBtn\">Upload</button>";
  html += "<div class=\"progress\" id=\"progress\">";
  html += "<div class=\"progress-bar\"><div class=\"progress-fill\" id=\"progressFill\">0%</div></div>";
  html += "<span id=\"progressText\">Uploading...</span></div>";
  html += "<div class=\"status\" id=\"status\"></div>";
  html += "<div class=\"file-list\"><h3>Files on Device:</h3><div id=\"fileItems\"></div></div>";
  html += "</div>";
  html += "<script>";
  html += "const ua=document.getElementById('uploadArea');const fi=document.getElementById('fileInput');";
  html += "const ub=document.getElementById('uploadBtn');const p=document.getElementById('progress');";
  html += "const pf=document.getElementById('progressFill');const pt=document.getElementById('progressText');";
  html += "const st=document.getElementById('status');";
  html += "ua.addEventListener('click',()=>fi.click());";
  html += "ua.addEventListener('dragover',(e)=>{e.preventDefault();ua.classList.add('dragover');});";
  html += "ua.addEventListener('dragleave',()=>ua.classList.remove('dragover'));";
  html += "ua.addEventListener('drop',(e)=>{e.preventDefault();ua.classList.remove('dragover');fi.files=e.dataTransfer.files;if(fi.files.length>0)uploadFile();});";
  html += "fi.addEventListener('change',()=>{if(fi.files.length>0)uploadFile();});";
  html += "ub.addEventListener('click',uploadFile);";
  html += "function uploadFile(){const f=fi.files[0];if(!f)return;if(!f.name.endsWith('.gif')){showStatus('Please select GIF!','error');return;}";
  html += "const fd=new FormData();fd.append('file',f);p.style.display='block';st.style.display='none';ub.disabled=true;";
  html += "const xhr=new XMLHttpRequest();xhr.upload.addEventListener('progress',(e)=>{if(e.lengthComputable){const pc=(e.loaded/e.total)*100;";
  html += "pf.style.width=pc+'%';pf.textContent=Math.round(pc)+'%';pt.textContent='Uploading: '+Math.round(e.loaded/1024)+' KB';}});";
  html += "xhr.addEventListener('load',()=>{if(xhr.status===200){showStatus('Upload successful!','success');fi.value='';loadFileList();}else{showStatus('Upload failed!','error');}";
  html += "p.style.display='none';ub.disabled=false;});";
  html += "xhr.addEventListener('error',()=>{showStatus('Connection error!','error');p.style.display='none';ub.disabled=false;});";
  html += "xhr.open('POST','/upload',true);xhr.send(fd);}";
  html += "function showStatus(m,t){st.textContent=m;st.className='status '+t;}";
  html += "function loadFileList(){fetch('/files').then(r=>r.json()).then(d=>{const fi=document.getElementById('fileItems');fi.innerHTML='';";
  html += "if(d.files.length===0){fi.innerHTML='<p style=\"color:#999;\">No files</p>';return;}";
  html += "d.files.forEach(f=>{const item=document.createElement('div');item.className='file-item';";
  html += "item.innerHTML='<span class=\"file-name\">'+f.name+' ('+f.size+' bytes)</span><button class=\"delete-btn\" onclick=\"deleteFile('"'"'+f.name+'"'"')\">Delete</button>';";
  html += "fi.appendChild(item);});});}";
  html += "function deleteFile(fn){if(!confirm('Delete this file?'))return;";
  html += "fetch('/delete?name='+encodeURIComponent(fn),{method:'GET'}).then(r=>r.json()).then(d=>{";
  html += "showStatus(d.message,d.success?'success':'error');loadFileList();});}";
  html += "loadFileList();";
  html += "</script></body></html>";
  
  server.send(200, "text/html; charset=utf-8", html);
}

// Handle file upload
void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload Start: %s\n", upload.filename.c_str());
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf("Writing %d bytes\n", upload.currentSize);
    
  } else if (upload.status == UPLOAD_FILE_END) {
    fs::File file = SPIFFS.open("/animation.gif", "w");
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
      Serial.println("File saved successfully!");
      
      // Reload GIF
      gif.close();
      delay(100);
      
      // Try to open GIF
      if (gif.open((uint8_t *)"/animation.gif", 0, GIFDraw)) {
        gifLoaded = true;
        Serial.printf("GIF reloaded: %dx%d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
      } else {
        Serial.println("Failed to load GIF");
      }
    }
    server.send(200, "application/json", "{\"success\": true}");
  }
}

// Get file list
void handleFileList() {
  String json = "{\"files\": [";
  bool first = true;
  
  fs::File root = SPIFFS.open("/");
  fs::File file = root.openNextFile();
  
  while (file) {
    if (file.name()[0] != '.') {
      if (!first) json += ",";
      json += "{\"name\": \"" + String(file.name()) + "\", \"size\": " + String(file.size()) + "}";
      first = false;
    }
    file = root.openNextFile();
  }
  
  json += "]}";
  server.send(200, "application/json", json);
}

// Delete file
void handleDelete() {
  if (!server.hasArg("name")) {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"Missing filename\"}");
    return;
  }
  
  String filename = "/" + server.arg("name");
  if (SPIFFS.remove(filename)) {
    server.send(200, "application/json", "{\"success\": true, \"message\": \"File deleted\"}");
  } else {
    server.send(500, "application/json", "{\"success\": false, \"message\": \"Delete failed\"}");
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
  if (gif.open((uint8_t *)"/animation.gif", 0, GIFDraw)) {
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
    delay(gifDelay);
  } else if (gifLoaded) {
    gif.reset();  // Loop back
  } else {
    delay(100);  // Wait if no GIF loaded
  }
}
