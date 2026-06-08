#include "services/usgs_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <cstdlib>
#include <cstring>

#include "config.h"

namespace services::usgs {

namespace {

constexpr char kApiBase[] =
    "https://waterservices.usgs.gov/nwis/iv/"
    "?sites=%s&parameterCd=00060,00065,00010&period=PT26H&format=json";

constexpr char kParamCfs[]   = "00060";
constexpr char kParamStage[] = "00065";
constexpr char kParamTemp[]  = "00010";

GaugeReading s_latest;
GaugeReading s_history[config::kGaugeHistoryCount];
size_t s_history_count = 0;

float parseValue(const char* s) {
  if (s == nullptr || s[0] == '\0') {
    return -1.0f;
  }
  char* end = nullptr;
  const float v = strtof(s, &end);
  if (end == s) {
    return -1.0f;
  }
  return v;
}

}  // namespace

bool fetchUpdate() {
  char url[192];
  snprintf(url, sizeof(url), kApiBase, config::kUsgsGaugeId);

  // Filter: keep only the variable code and value strings — reduces the
  // parsed document from ~79 KB to a few KB by discarding dateTime, qualifiers, etc.
  JsonDocument filter;
  JsonObject ts_item = filter["value"]["timeSeries"].add<JsonObject>();
  ts_item["variable"]["variableCode"][0]["value"] = true;
  ts_item["values"][0]["value"][0]["value"] = true;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.println("usgs: http.begin failed");
    return false;
  }
  http.setTimeout(15000);

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("usgs: HTTP %d\n", code);
    http.end();
    return false;
  }

  JsonDocument doc;
  WiFiClient& stream = http.getStream();
  const DeserializationError err =
      deserializeJson(doc, stream, DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("usgs: JSON error: %s\n", err.c_str());
    return false;
  }

  JsonArray time_series = doc["value"]["timeSeries"].as<JsonArray>();
  if (time_series.isNull()) {
    Serial.println("usgs: no timeSeries in response");
    return false;
  }

  // Scratch arrays: collect all raw CFS values for history, plus latest of each param.
  static float cfs_scratch[config::kGaugeHistoryCount * 4];
  size_t cfs_scratch_n = 0;
  float latest_cfs   = -1.0f;
  float latest_stage = -1.0f;
  float latest_temp  = -1.0f;

  for (JsonObject ts : time_series) {
    const char* param = ts["variable"]["variableCode"][0]["value"].as<const char*>();
    if (param == nullptr) {
      continue;
    }

    JsonArray vals = ts["values"][0]["value"].as<JsonArray>();
    if (vals.isNull() || vals.size() == 0) {
      continue;
    }

    // Latest reading is the last element (USGS returns oldest-first).
    const char* last_val =
        vals[vals.size() - 1]["value"].as<const char*>();

    if (strcmp(param, kParamCfs) == 0) {
      latest_cfs = parseValue(last_val);

      // Populate scratch array for sparkline history.
      cfs_scratch_n = 0;
      for (JsonObject entry : vals) {
        if (cfs_scratch_n >= sizeof(cfs_scratch) / sizeof(cfs_scratch[0])) {
          break;
        }
        cfs_scratch[cfs_scratch_n++] = parseValue(entry["value"].as<const char*>());
      }
    } else if (strcmp(param, kParamStage) == 0) {
      latest_stage = parseValue(last_val);
    } else if (strcmp(param, kParamTemp) == 0) {
      latest_temp = parseValue(last_val);
    }
  }

  s_latest.cfs      = latest_cfs;
  s_latest.stage_ft = latest_stage;
  s_latest.temp_c   = latest_temp;

  // Keep the most recent kGaugeHistoryCount CFS readings.
  const size_t skip = cfs_scratch_n > config::kGaugeHistoryCount
                          ? cfs_scratch_n - config::kGaugeHistoryCount
                          : 0;
  s_history_count = cfs_scratch_n - skip;
  for (size_t i = 0; i < s_history_count; ++i) {
    s_history[i].cfs      = cfs_scratch[skip + i];
    s_history[i].stage_ft = -1.0f;
    s_history[i].temp_c   = -1.0f;
  }

  Serial.printf("usgs: CFS=%.0f stage=%.2fft temp=%.1fC history=%u readings\n",
                latest_cfs, latest_stage, latest_temp,
                static_cast<unsigned>(s_history_count));
  return true;
}

const GaugeReading& latest() { return s_latest; }

const GaugeReading* history() { return s_history; }

size_t historyCount() { return s_history_count; }

}  // namespace services::usgs
