#include "WifiManager.h"

WifiManager::WifiManager() : server(80) { uploadTargetFolder = "/"; }

void WifiManager::begin() {
  server.on("/", HTTP_GET, std::bind(&WifiManager::handleRoot, this));
  server.on("/create", HTTP_POST, std::bind(&WifiManager::handleCreate, this));
  server.on("/delete", HTTP_GET, std::bind(&WifiManager::handleDelete, this));
  server.on("/upload", HTTP_POST, std::bind(&WifiManager::handleUpload, this),
            std::bind(&WifiManager::handleUploadLoop, this));
}

void WifiManager::startAP() {
  // 1. Stop Audio
  AudioCommand cmd;
  cmd.type = CMD_SET_VOLUME;
  cmd.value = 0;
  xQueueSend(audioQueue, &cmd, 0);
  cmd.type = CMD_STOP;
  xQueueSend(audioQueue, &cmd, 0);

  // 2. Safety Delay (SPI Race)
  delay(600);

  // 3. Start AP & Server
  WiFi.softAP("Padium-Manager", "12345678");
  server.begin();
}

void WifiManager::stopAP() {
  server.stop();
  WiFi.mode(WIFI_OFF);
  // Main loop will detect exit and handle UI/Scanning
}

void WifiManager::handleClient() { server.handleClient(); }

String WifiManager::getIP() { return WiFi.softAPIP().toString(); }

// --- Logic ---

void WifiManager::deleteRecursive(File dir) {
  if (!dir)
    return;
  dir.rewindDirectory();
  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;
    String path = entry.path();
    if (entry.isDirectory()) {
      deleteRecursive(entry);
      SD.rmdir(path.c_str());
    } else {
      SD.remove(path.c_str());
    }
    entry.close();
  }
}

// STREAMING ROOT HANDLER
void WifiManager::handleRoot() {
  // 1. Send Header via Chunked Transfer
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  String head = R"raw(
        <html><head><title>Padium Pro Manager</title>
        <meta name='viewport' content='width=device-width, initial-scale=1'>
        <style>body{font-family:sans-serif; margin:20px;} table{border-collapse:collapse;} th,td{text-align:left;}</style>
        <script>function setAction(form) { var folder = document.getElementById('targetFolder').value; form.action = '/upload?folder=' + folder; }</script>
        </head><body>
        <h1>Padium Pro File Manager</h1>
        
        <div style='background:#e0e0e0; padding:10px; margin-bottom:10px; border-radius:5px;'>
        <h4>Create New Bank</h4>
        <form method='POST' action='/create'>
        <input type='text' name='folderName' placeholder='Bank Name'>
        <input type='submit' value='Create'>
        </form></div>

        <div style='background:#f0f0f0; padding:15px; margin-bottom:20px; border-radius:5px;'>
        <h3>Upload File</h3>
        <form method='POST' action='/upload' enctype='multipart/form-data' onsubmit='setAction(this)'>
        Target Bank: <select id='targetFolder' name='targetFolder'>
    )raw";
  server.sendContent(head);

  // 2. Dynamic Folders
  if (xSemaphoreTake(sdCardMutex, portMAX_DELAY)) {
    File root = SD.open("/");
    if (root) {
      root.rewindDirectory();
      while (true) {
        File entry = root.openNextFile();
        if (!entry)
          break;
        if (entry.isDirectory() && !String(entry.name()).startsWith(".")) {
          String opt = "<option value='" + String(entry.name()) + "'>" +
                       String(entry.name()) + "</option>";
          server.sendContent(opt);
        }
        entry.close();
      }
      root.close();
    }
    xSemaphoreGive(sdCardMutex);
  }

  // 3. Middle HTML
  String middle = R"raw(
        <option value='/'>Root (Not recommended)</option>
        </select><br><br>
        <input type='file' name='upload'>
        <input type='submit' value='Upload'>
        </form></div>
        <h3>SD Card Contents</h3>
        <table border='1' cellspacing='0' cellpadding='5' width='100%'>
        <tr><th>Type</th><th>Name</th><th>Size</th><th>Action</th></tr>
    )raw";
  server.sendContent(middle);

  // 4. File List (Streaming Rows)
  if (xSemaphoreTake(sdCardMutex, portMAX_DELAY)) {
    File root = SD.open("/");
    if (root) {
      root.rewindDirectory();
      while (true) {
        File entry = root.openNextFile();
        if (!entry)
          break;

        String eName = entry.name();
        if (eName.startsWith(".")) {
          entry.close();
          continue;
        }

        String row = "<tr>";
        row += "<td>" + String(entry.isDirectory() ? "DIR" : "FILE") + "</td>";
        row += "<td>" + eName + "</td>";
        row += "<td>" +
               String(entry.isDirectory() ? "-" : String(entry.size()) + " B") +
               "</td>";
        row += "<td><a href='/delete?path=" + String(entry.path()) +
               "' onclick=\"return confirm('Delete " + eName +
               "?');\">[DELETE]</a></td>";
        row += "</tr>";

        server.sendContent(row);
        entry.close();
      }
      root.close();
    }
    xSemaphoreGive(sdCardMutex);
  }

  // 5. Footer
  String foot = "</table></body></html>";
  server.sendContent(foot);
  server.sendContent(""); // End Stream
}

void WifiManager::handleCreate() {
  if (!server.hasArg("folderName")) {
    server.send(400, "text/plain", "Missing name");
    return;
  }
  String name = server.arg("folderName");
  if (name.length() > 0) {
    String path = "/" + name;
    xSemaphoreTake(sdCardMutex, portMAX_DELAY);
    if (!SD.exists(path))
      SD.mkdir(path);
    xSemaphoreGive(sdCardMutex);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void WifiManager::handleDelete() {
  if (!server.hasArg("path"))
    return;
  String path = server.arg("path");

  xSemaphoreTake(sdCardMutex, portMAX_DELAY);
  if (SD.exists(path)) {
    File f = SD.open(path);
    if (f.isDirectory()) {
      deleteRecursive(f);
      SD.rmdir(path.c_str());
    } else {
      SD.remove(path.c_str());
    }
    f.close();
  }
  xSemaphoreGive(sdCardMutex);
  server.sendHeader("Location", "/");
  server.send(303);
}

void WifiManager::handleUpload() { server.send(200, "text/plain", ""); }

void WifiManager::handleUploadLoop() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (server.hasArg("folder")) {
      uploadTargetFolder = server.arg("folder");
      if (!uploadTargetFolder.startsWith("/"))
        uploadTargetFolder = "/" + uploadTargetFolder;
      if (!uploadTargetFolder.endsWith("/"))
        uploadTargetFolder += "/";
    } else {
      uploadTargetFolder = "/";
    }

    String filename = uploadTargetFolder + upload.filename;

    xSemaphoreTake(sdCardMutex, portMAX_DELAY);
    if (SD.exists(filename.c_str()))
      SD.remove(filename.c_str());
    uploadFile = SD.open(filename.c_str(), FILE_WRITE);
    xSemaphoreGive(sdCardMutex);

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      xSemaphoreTake(sdCardMutex, portMAX_DELAY);
      uploadFile.write(upload.buf, upload.currentSize);
      xSemaphoreGive(sdCardMutex);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      xSemaphoreTake(sdCardMutex, portMAX_DELAY);
      uploadFile.close();
      xSemaphoreGive(sdCardMutex);
    }
    server.sendHeader("Location", "/");
    server.send(303);
  }
}
