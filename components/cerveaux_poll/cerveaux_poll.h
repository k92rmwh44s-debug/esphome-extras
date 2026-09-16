#pragma once

#include "esphome/core/component.h"
#include <atomic>

namespace esphome {
namespace cerveaux_poll {

// Valeurs partagées : écrites par la tâche de fond (coeur 0),
// lues par l'écran (coeur 1). std::atomic = pas besoin de verrou,
// une lecture ne peut jamais tomber sur une valeur à moitié écrite.
extern std::atomic<float> g_sechoir_humidite;
extern std::atomic<float> g_sechoir_temp;
extern std::atomic<float> g_sechoir_chauffage;
extern std::atomic<float> g_sechoir_ventilo;
extern std::atomic<float> g_elevage_humidite;
extern std::atomic<float> g_elevage_temp;
extern std::atomic<float> g_elevage_chauffage;

class CerveauxPollComponent : public Component {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
};

}  // namespace cerveaux_poll
}  // namespace esphome
