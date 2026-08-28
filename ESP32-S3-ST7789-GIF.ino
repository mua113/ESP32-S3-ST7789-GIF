/*
 * ESP32-S3 ST7789 GIF Animation Player with WiFi
 * 240x240 LCD Display
 * Simple WiFi Upload Web Interface
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

// Simple HTML page for GIF upload
void handleRoot() {
  String html = "<html><head><title>GIF Player</title>";
  html += "<style>";
  html += "body{font-family:Arial;max-width:600px;margin:50px auto;padding:20px;background:#667eea;color:#333}";
  html += ".container{background:white;padding:30px;border-radius:10px}";
  html += "h1{text-align:center;color:#667eea}";
  html += ".upload-area{border:3px dashed #667eea;padding:40px;text-align:center;border-radius:8px;cursor:pointer}";
  html += ".upload-area:hover{background:#f0f2ff}";
  html += "input[type=file]{display:none}";
  html += "button{background:#667eea;color:white;padding:12px 30px;border:none;border-radius:5px;cursor:pointer;width:100%;margin-top:20px;font-size:16px}";
  html += "button:hover{background:#764ba2}";
  html += ".progress{display:none;margin:20px 0;background:#e0e0e0;height:30px;border-radius:5px;overflow:hidden}";
  html += ".progress-bar{background:#667eea;height:100%;width:0%;transition:width 0.3s;color:white;text-align:center;line-height:30px}";
  html += ".status{margin:20px 0;padding:15px;border-radius:5px;text-align:center}";
  html += ".success{background:#d4edda;color:#155724}";
  html += ".error{background:#f8d7da;color:#721c24}";
  html += ".file-list{margin-top:30px}";
  html += ".file-item{background:#f8f9ff;padding:10px;margin:8px 0;border-radius:5px;display:flex;justify-content:space-between}";
  html += ".delete-btn{background:#dc3545;color:white;border:none;padding:5px 10px;border-radius:3px;cursor:pointer;font-size:12px}";
  html += ".delete-btn:hover{background:#c82333}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32-S3 GIF Player</h1>";
  html += "<p style='text-align:center'>LCD 240x240 | ST7789 | WiFi Upload</p>";
  html += "<div class='upload-area' id='uploadArea'>";
  html += "<p>Click to select GIF file</p>";
  html += "<input type='file' id='fileInput' accept='.gif'>";
  html += "</div>";
  html += "<button onclick='uploadFile()'>Upload GIF</button>";
  html += "<div class='progress' id='progress'><div class='progress-bar' id='progressBar'>0%</div></div>";
  html += "<div id='status'></div>";
  html += "<div class='file-list'>";
  html += "<h3>Files:</h3>";
  html += "<div id='fileList'></div>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "document.getElementById('uploadArea').onclick=function(){document.getElementById('fileInput').click()};";
  html += "function uploadFile(){";
  html += "  const file=document.getElementById('fileInput').files[0];";
  html += "  if(!file){alert('Select GIF first');return}";
  html += "  if(!file.name.endsWith('.gif')){alert('Select .gif file only');return}";
  html += "  const fd=new FormData();fd.append('file',file);";
  html += "  const prog=document.getElementById('progress');";
  html += "  const pbar=document.getElementById('progressBar');";
  html += "  const stat=document.getElementById('status');";
  html += "  prog.style.display='block';stat.innerHTML='';";
  html += "  const xhr=new XMLHttpRequest();";
  html += "  xhr.upload.onprogress=function(e){";
  html += "    if(e.lengthComputable){";
  html += "      const pc=Math.round((e.loaded/e.total)*100);";
  html += "      pbar.style.width=pc+'%';";
  html += "      pbar.textContent=pc+'%';";
  html += "    }";
  html += "  };";
  html += "  xhr.onload=function(){";
  html += "    if(xhr.status===200){";
  html += "      stat.innerHTML='<div class=\"status success\">Upload OK</div>';";
  html += "      document.getElementById('fileInput').value='';";
  html += "      loadFiles();";
  html += "    }else{";
  html += "      stat.innerHTML='<div class=\"status error\">Upload failed</div>';";
  html += "    }";
  html += "  };";
  html += "  xhr.onerror=function(){stat.innerHTML='<div class=\"status error\">Error</div>';};";
  html += "  xhr.open('POST','/upload',true);";
  html += "  xhr.send(fd);";
  html += "}";
  html += "function loadFiles(){";
  html += "  fetch('/files').then(r=>r.json()).then(d=>{";
  html += "    const fl=document.getElementById('fileList');";
  html += "    fl.innerHTML='';";
  html += "    if(d.files.length===0){fl.innerHTML='<p>No files</p>';return}";
  html += "    d.files.forEach(f=>{";
  html += "      const div=document.createElement('div');";
  html += "      div.className='file-item';";
  html += "      div.innerHTML='<span>'+f.name+' ('+f.size+' B)</span>';";
  html += "      const btn=document.createElement('button');";
  html += "      btn.className='delete-btn';";
  html += "      btn.textContent='Delete';";
  html += "      btn.onclick=function(){delFile(f.name)};";
  html += "      div.appendChild(btn);";
  html += "      fl.appendChild(div);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function delFile(name){";
  html += "  if(!confirm('Delete '+name))return;";
  html += "  fetch('/delete?name='+encodeURIComponent(name)).then(r=>r.json()).then(d=>{";
  html += "    if(d.success){";
  html += "      document.getElementById('status').innerHTML='<div class=\"status success\">Deleted</div>';";
  html += "      loadFiles();";
  html += "    }";
  html += "  });";
  html += "}";
  html += "loadFiles();";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html; charset=utf-8", html);
}

// Handle file upload
void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload: %s\n", upload.filename.c_str());
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf("Writing %d bytes\n", upload.currentSize);
    
  } else if (upload.status == UPLOAD_FILE_END) {
    fs::File file = SPIFFS.open("/animation.gif", "w");
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
      Serial.println("File saved!");
      
      gif.close();
      delay(100);
      
      if (gif.open((uint8_t *)"/animation.gif", 0, GIFDraw)) {
        gifLoaded = true;
        Serial.printf("GIF: %dx%d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
      }
    }
    server.send(200, "application/json", "{\"success\":true}");
  }
}

// Get file list
void handleFileList() {
  String json = "{\"files\":[";
  bool first = true;
  
  fs::File root = SPIFFS.open("/");
  fs::File file = root.openNextFile();
  
  while (file) {
    if (file.name()[0] != '.') {
      if (!first) json += ",";
      json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
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
    server.send(400, "application/json", "{\"success\":false}");
    return;
  }
  
  String filename = "/" + server.arg("name");
  if (SPIFFS.remove(filename)) {
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(500, "application/json", "{\"success\":false}");
  }
}

// Display WiFi info on LCD
void displayWiFiInfo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  
  tft.drawString("WiFi OK!", 80, 50);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("SSID: " + String(ssid), 20, 80);
  tft.drawString("IP: " + WiFi.localIP().toString(), 20, 100);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Ready", 70, 150);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32-S3 GIF Player ===");
  
  // Initialize TFT
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Init...", 80, 100);
  
  delay(500);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS failed");
    tft.fillScreen(TFT_RED);
    delay(3000);
    return;
  }
  Serial.println("SPIFFS OK");
  
  // Connect WiFi
  Serial.printf("WiFi: %s\n", ssid);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("WiFi...", 80, 100);
  
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    displayWiFiInfo();
    delay(3000);
  } else {
    Serial.println("WiFi failed");
    tft.fillScreen(TFT_RED);
    delay(3000);
  }
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/files", handleFileList);
  server.on("/delete", handleDelete);
  server.begin();
  Serial.println("Server started");
  Serial.printf("Open: http://%s\n", WiFi.localIP().toString().c_str());
  
  // Try to load GIF
  if (gif.open((uint8_t *)"/animation.gif", 0, GIFDraw)) {
    gifLoaded = true;
    Serial.println("GIF loaded");
  }
}

void loop() {
  server.handleClient();
  
  if (gifLoaded && gif.playFrame(true, NULL)) {
    delay(gifDelay);
  } else if (gifLoaded) {
    gif.reset();
  } else {
    delay(100);
  }
}
