#pragma once

#include <cstddef>
#include <cstdint>

namespace services::usgs {

struct GaugeReading {
  float cfs       = -1.0f;  // discharge (ft³/s); -1 = no data
  float stage_ft  = -1.0f;  // gauge height (ft);  -1 = no data
  float temp_c    = -1.0f;  // water temp (°C);    -1 = no data
};

/** Fetch latest reading from USGS NWIS for the gauge configured in config.h. */
bool fetchUpdate();

/** Most recent reading (invalid until fetchUpdate() returns true once). */
const GaugeReading& latest();

/** Ring buffer of up to kGaugeHistoryCount past readings, oldest first. */
const GaugeReading* history();
size_t historyCount();

}  // namespace services::usgs
