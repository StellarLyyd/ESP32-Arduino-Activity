#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"

// =====================================================
// Wi-Fi settings
// =====================================================
const char *ssid = "Rev Member";
const char *password = "incubator";

// =====================================================
// XIAO ESP32-S3 Sense camera pins
// OV3660 camera
// =====================================================
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1

#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39

#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15

#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

// =====================================================
// MJPEG stream definitions
// =====================================================
#define PART_BOUNDARY "123456789000000000000987654321"

static const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;

static const char *STREAM_BOUNDARY =
    "\r\n--" PART_BOUNDARY "\r\n";

static const char *STREAM_PART =
    "Content-Type: image/jpeg\r\n"
    "Content-Length: %u\r\n"
    "\r\n";

// HTTP servers
httpd_handle_t camera_httpd = nullptr;
httpd_handle_t stream_httpd = nullptr;

// =====================================================
// Webpage
// =====================================================
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta
    name="viewport"
    content="width=device-width, initial-scale=1.0"
  >

  <title>Upright</title>

  <style>
    body {
      margin: 0;
      padding: 20px;
      background: #111;
      color: white;
      font-family: Arial, sans-serif;
      text-align: center;
    }

    h1 {
      font-size: 24px;
      margin-bottom: 15px;
    }

    .camera-container {
      width: 100%;
      max-width: 900px;
      margin: auto;
    }

    img {
      display: block;
      width: 100%;
      height: auto;
      background: black;
      border-radius: 10px;
    }

    .status {
      margin-top: 12px;
      color: #aaa;
      font-size: 14px;
    }

    button {
      margin-top: 15px;
      padding: 10px 18px;
      border: none;
      border-radius: 6px;
      font-size: 16px;
      cursor: pointer;
    }
  </style>
</head>

<body>
  <h1>UpRight!</h1>

  <div class="camera-container">
    <img id="stream" alt="Camera stream">
  </div>

  <button onclick="restartStream()">Restart Stream</button>

  <div class="status" id="status">
    Connecting...
  </div>

  <script>
    const stream = document.getElementById("stream");
    const statusText = document.getElementById("status");

    function startStream() {
      statusText.textContent = "Connecting...";

      // Stream runs on port 81.
      stream.src =
        "http://" + window.location.hostname + ":81/stream?t=" + Date.now();
    }

    function restartStream() {
      stream.src = "";
      setTimeout(startStream, 300);
    }

    stream.onload = function() {
      statusText.textContent = "Streaming";
    };

    stream.onerror = function() {
      statusText.textContent = "Stream disconnected. Retrying...";

      setTimeout(() => {
        restartStream();
      }, 2000);
    };

    startStream();
  </script>
</body>
</html>
)rawliteral";

// =====================================================
// Main webpage handler
// =====================================================
static esp_err_t indexHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  return httpd_resp_send(
      req,
      INDEX_HTML,
      HTTPD_RESP_USE_STRLEN
  );
}

// =====================================================
// Stream handler
// =====================================================
static esp_err_t streamHandler(httpd_req_t *req) {
  camera_fb_t *fb = nullptr;
  esp_err_t result = ESP_OK;

  uint8_t *jpegBuffer = nullptr;
  size_t jpegLength = 0;

  char partBuffer[128];

  result = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);

  if (result != ESP_OK) {
    return result;
  }

  httpd_resp_set_hdr(
      req,
      "Access-Control-Allow-Origin",
      "*"
  );

  httpd_resp_set_hdr(
      req,
      "Cache-Control",
      "no-store, no-cache, must-revalidate, max-age=0"
  );

  Serial.println("Stream client connected.");

  while (true) {
    // Get the newest available frame.
    fb = esp_camera_fb_get();

    if (fb == nullptr) {
      Serial.println("Camera frame capture failed.");
      result = ESP_FAIL;
      break;
    }

    // OV3660 is configured for JPEG, so conversion normally
    // should not be required.
    if (fb->format == PIXFORMAT_JPEG) {
      jpegBuffer = fb->buf;
      jpegLength = fb->len;
    } else {
      bool converted = frame2jpg(
          fb,
          80,
          &jpegBuffer,
          &jpegLength
      );

      if (!converted) {
        Serial.println("JPEG conversion failed.");

        esp_camera_fb_return(fb);
        fb = nullptr;

        result = ESP_FAIL;
        break;
      }
    }

    // Send multipart boundary.
    result = httpd_resp_send_chunk(
        req,
        STREAM_BOUNDARY,
        strlen(STREAM_BOUNDARY)
    );

    if (result != ESP_OK) {
      if (fb->format != PIXFORMAT_JPEG) {
        free(jpegBuffer);
        jpegBuffer = nullptr;
      }

      esp_camera_fb_return(fb);
      fb = nullptr;

      break;
    }

    // Build JPEG header.
    size_t headerLength = snprintf(
        partBuffer,
        sizeof(partBuffer),
        STREAM_PART,
        jpegLength
    );

    // Send JPEG header.
    result = httpd_resp_send_chunk(
        req,
        partBuffer,
        headerLength
    );

    if (result != ESP_OK) {
      if (fb->format != PIXFORMAT_JPEG) {
        free(jpegBuffer);
        jpegBuffer = nullptr;
      }

      esp_camera_fb_return(fb);
      fb = nullptr;

      break;
    }

    // Send JPEG image.
    result = httpd_resp_send_chunk(
        req,
        reinterpret_cast<const char *>(jpegBuffer),
        jpegLength
    );

    // Free converted JPEG buffer if conversion was needed.
    if (fb->format != PIXFORMAT_JPEG) {
      free(jpegBuffer);
      jpegBuffer = nullptr;
    }

    // Always return the frame buffer.
    esp_camera_fb_return(fb);
    fb = nullptr;

    if (result != ESP_OK) {
      break;
    }

    // Give Wi-Fi and system tasks a chance to run.
    delay(1);
  }

  Serial.println("Stream client disconnected.");

  return result;
}

// =====================================================
// Optional status endpoint
// =====================================================
static esp_err_t statusHandler(httpd_req_t *req) {
  char response[256];

  snprintf(
      response,
      sizeof(response),
      "{"
      "\"rssi\":%d,"
      "\"free_heap\":%u,"
      "\"free_psram\":%u"
      "}",
      WiFi.RSSI(),
      ESP.getFreeHeap(),
      ESP.getFreePsram()
  );

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  return httpd_resp_send(
      req,
      response,
      HTTPD_RESP_USE_STRLEN
  );
}

// =====================================================
// Start HTTP servers
// =====================================================
void startCameraServer() {
  // ---------------------------------------------------
  // Main webpage server on port 80
  // ---------------------------------------------------
  httpd_config_t mainConfig = HTTPD_DEFAULT_CONFIG();

  mainConfig.server_port = 80;
  mainConfig.ctrl_port = 32768;
  mainConfig.max_open_sockets = 4;
  mainConfig.lru_purge_enable = true;

  httpd_uri_t indexUri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = indexHandler,
      .user_ctx = nullptr
  };

  httpd_uri_t statusUri = {
      .uri = "/status",
      .method = HTTP_GET,
      .handler = statusHandler,
      .user_ctx = nullptr
  };

  if (httpd_start(&camera_httpd, &mainConfig) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &indexUri);
    httpd_register_uri_handler(camera_httpd, &statusUri);

    Serial.println("Web server started on port 80.");
  } else {
    Serial.println("Failed to start web server.");
  }

  // ---------------------------------------------------
  // Stream server on port 81
  // ---------------------------------------------------
  httpd_config_t streamConfig = HTTPD_DEFAULT_CONFIG();

  streamConfig.server_port = 81;
  streamConfig.ctrl_port = 32769;
  streamConfig.max_open_sockets = 2;
  streamConfig.lru_purge_enable = true;

  // Larger stack helps with continuous streaming.
  streamConfig.stack_size = 8192;

  httpd_uri_t streamUri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = streamHandler,
      .user_ctx = nullptr
  };

  if (httpd_start(&stream_httpd, &streamConfig) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &streamUri);

    Serial.println("Stream server started on port 81.");
  } else {
    Serial.println("Failed to start stream server.");
  }
}

// =====================================================
// Camera initialization
// =====================================================
bool initializeCamera() {
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    Serial.println("PSRAM detected.");

    // VGA is a good balance between image quality and latency.
    config.frame_size = FRAMESIZE_VGA;

    // JPEG quality:
    // Lower number = better image, larger network packet.
    // Higher number = lower image quality, less lag.
    config.jpeg_quality = 15;

    // Two buffers allow continuous capture.
    config.fb_count = 2;

    // Drop old frames and send the newest frame.
    config.grab_mode = CAMERA_GRAB_LATEST;

    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    Serial.println("WARNING: PSRAM not detected.");

    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 18;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t error = esp_camera_init(&config);

  if (error != ESP_OK) {
    Serial.printf(
        "Camera initialization failed: 0x%x\n",
        error
    );

    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();

  if (sensor == nullptr) {
    Serial.println("Failed to get camera sensor.");
    return false;
  }

  Serial.printf(
      "Camera sensor PID: 0x%04X\n",
      sensor->id.PID
  );

  // OV3660 commonly needs orientation adjustment on the
  // XIAO ESP32-S3 Sense.
  if (sensor->id.PID == OV3660_PID) {
    Serial.println("OV3660 detected.");

    sensor->set_vflip(sensor, 1);
    sensor->set_brightness(sensor, 1);
    sensor->set_saturation(sensor, -1);
  }

  // Explicitly enforce VGA after initialization.
  sensor->set_framesize(sensor, FRAMESIZE_VGA);

  return true;
}

// =====================================================
// Wi-Fi connection
// =====================================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);

  // Remove an old connection before reconnecting.
  WiFi.disconnect(true);
  delay(500);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (millis() - startTime > 30000) {
      Serial.println();
      Serial.println("Wi-Fi connection timed out.");
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println("Wi-Fi connected.");

  // Important for reducing periodic stream pauses.
  WiFi.setSleep(false);

  Serial.print("Camera webpage: http://");
  Serial.println(WiFi.localIP());

  Serial.print("Direct stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");

  Serial.print("Wi-Fi signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("XIAO ESP32-S3 OV3660 Camera");
  Serial.println("==============================");

  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  Serial.print("PSRAM size: ");
  Serial.println(ESP.getPsramSize());

  Serial.print("Free PSRAM: ");
  Serial.println(ESP.getFreePsram());

  if (!initializeCamera()) {
    Serial.println("Camera setup failed. Restarting...");
    delay(5000);
    ESP.restart();
  }

  connectWiFi();
  startCameraServer();

  Serial.println("Camera server is ready.");
}

// =====================================================
// Main loop
// =====================================================
void loop() {
  // Reconnect if Wi-Fi is temporarily lost.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Reconnecting...");

    WiFi.disconnect();
    WiFi.begin(ssid, password);

    unsigned long reconnectStart = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - reconnectStart < 15000
    ) {
      delay(500);
      Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      WiFi.setSleep(false);

      Serial.print("Wi-Fi reconnected: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Reconnect failed. Restarting...");
      ESP.restart();
    }
  }

  delay(1000);
}