#include "esp_camera.h"
#include "WiFi.h"
#include "esp_http_server.h"

// Pin definitions
const int8_t PWDN_GPIO_NUM = 32;
const int8_t RESET_GPIO_NUM = -1;
const int8_t XCLK_GPIO_NUM = 0;
const int8_t SIOD_GPIO_NUM = 26;
const int8_t SIOC_GPIO_NUM = 27;
const int8_t Y9_GPIO_NUM = 35;
const int8_t Y8_GPIO_NUM = 34;
const int8_t Y7_GPIO_NUM = 39;
const int8_t Y6_GPIO_NUM = 36;
const int8_t Y5_GPIO_NUM = 21;
const int8_t Y4_GPIO_NUM = 19;
const int8_t Y3_GPIO_NUM = 18;
const int8_t Y2_GPIO_NUM = 5;
const int8_t VSYNC_GPIO_NUM = 25;
const int8_t HREF_GPIO_NUM = 23;
const int8_t PCLK_GPIO_NUM = 22;

const char *ssid = "ESP32-CAM";
const char *password = "12345678";

httpd_handle_t httpd = NULL;

static esp_err_t index_handler(httpd_req_t *req) {
  const char *html = "<html><body><img src=\"/stream\" style=\"width:640px; height:480px;\"></body></html>";
  httpd_resp_send(req, html, strlen(html));
  return ESP_OK;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  char *part_buf[64];
  static int64_t last_frame = 0;

  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=123456789000000000000987654321");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
      break;
    }

    if (fb->format != PIXFORMAT_JPEG) {
      Serial.println("Non-JPEG data not implemented");
      res = ESP_FAIL;
      break;
    }

    httpd_resp_sendstr_chunk(req, "\r\n--123456789000000000000987654321\r\n");
    httpd_resp_sendstr_chunk(req, "Content-Type: image/jpeg\r\nContent-Length: ");
    char len_str[16];
    sprintf(len_str, "%u\r\n\r\n", fb->len);
    httpd_resp_sendstr_chunk(req, len_str);
    httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    
    esp_camera_fb_return(fb);
    fb = NULL;
  }
  return res;
}

void setup() {
  Serial.begin(115200);

  camera_config_t config;
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  WiFi.softAP(ssid, password);

  httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
  http_config.server_port = 80;

  if (httpd_start(&httpd, &http_config) == ESP_OK) {
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(httpd, &index_uri);

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(httpd, &stream_uri);
  }

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("' to connect");
}

void loop() {
  delay(1);
}