#include "cerveaux_poll.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome {
namespace cerveaux_poll {

static const char *const TAG = "cerveaux_poll";

std::atomic<float> g_sechoir_humidite{0.0f};
std::atomic<float> g_sechoir_temp{0.0f};
std::atomic<float> g_sechoir_chauffage{0.0f};
std::atomic<float> g_sechoir_ventilo{0.0f};
std::atomic<float> g_elevage_humidite{0.0f};
std::atomic<float> g_elevage_temp{0.0f};
std::atomic<float> g_elevage_chauffage{0.0f};

struct Endpoint {
  const char *url;
  std::atomic<float> *target;
};

static Endpoint ENDPOINTS[] = {
    {"http://cerveau-sechoir.local/sensor/Humidite%20case", &g_sechoir_humidite},
    {"http://cerveau-sechoir.local/sensor/Temperature%20case", &g_sechoir_temp},
    {"http://cerveau-sechoir.local/sensor/Chauffage%20pourcentage", &g_sechoir_chauffage},
    {"http://cerveau-sechoir.local/sensor/Ventilo%20pourcentage", &g_sechoir_ventilo},
    {"http://cerveau-elevage.local/sensor/Humidite%20case%20elevage", &g_elevage_humidite},
    {"http://cerveau-elevage.local/sensor/Temperature%20case%20elevage", &g_elevage_temp},
    {"http://cerveau-elevage.local/sensor/Chauffage%20elevage%20pourcentage", &g_elevage_chauffage},
};

// Va chercher UNE valeur JSON {"id":"...","value":12.3,"state":"..."} et la
// range dans out_value. Ne bloque jamais l'écran : cette fonction tourne
// uniquement dans la tâche de fond, sur l'autre coeur.
static bool fetch_value(const char *url, float *out_value) {
  esp_http_client_config_t config = {};
  config.url = url;
  config.timeout_ms = 3000;
  config.method = HTTP_METHOD_GET;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) {
    return false;
  }

  if (esp_http_client_open(client, 0) != ESP_OK) {
    esp_http_client_cleanup(client);
    return false;
  }

  int content_length = esp_http_client_fetch_headers(client);
  int status = esp_http_client_get_status_code(client);
  if (status != 200 || content_length <= 0 || content_length > 511) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return false;
  }

  char buffer[512] = {0};
  int total_read = 0;
  while (total_read < content_length && total_read < (int) sizeof(buffer) - 1) {
    int r = esp_http_client_read(client, buffer + total_read, sizeof(buffer) - 1 - total_read);
    if (r <= 0)
      break;
    total_read += r;
  }
  buffer[total_read] = '\0';

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  cJSON *root = cJSON_Parse(buffer);
  if (root == nullptr) {
    return false;
  }
  cJSON *value = cJSON_GetObjectItem(root, "value");
  bool ok = false;
  if (cJSON_IsNumber(value)) {
    *out_value = (float) value->valuedouble;
    ok = true;
  }
  cJSON_Delete(root);
  return ok;
}

static void poll_task(void *pv) {
  const int nb = sizeof(ENDPOINTS) / sizeof(ENDPOINTS[0]);
  while (true) {
    for (int i = 0; i < nb; i++) {
      float v;
      if (fetch_value(ENDPOINTS[i].url, &v)) {
        ENDPOINTS[i].target->store(v);
      } else {
        ESP_LOGW(TAG, "Lecture échouée : %s", ENDPOINTS[i].url);
      }
      vTaskDelay(pdMS_TO_TICKS(300));  // petite pause entre deux requêtes
    }
    vTaskDelay(pdMS_TO_TICKS(5000));  // pause avant le prochain tour complet
  }
}

void CerveauxPollComponent::setup() {
  // coeur 0 = celui qui gère aussi le wifi ; le coeur 1 reste entièrement
  // dédié à l'écran et au tactile, plus jamais interrompu par le réseau.
  xTaskCreatePinnedToCore(poll_task, "cerveaux_poll", 8192, nullptr, 4, nullptr, 0);
}

}  // namespace cerveaux_poll
}  // namespace esphome
