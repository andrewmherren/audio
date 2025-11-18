#include "audio_module.h"
#include "../assets/audio_status_html.h"

// PlaybackEngine is only available in Arduino/ESP32 builds
#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include "playback_engine.h"
#endif

// Global instance for production builds
#if defined(ARDUINO) || defined(ESP_PLATFORM)
AudioModule audioModule;
#endif

AudioModule::AudioModule(IWebPlatformProvider *provider)
    : platformProvider(provider), playbackEngine(nullptr),
      recordingEngine(nullptr) {

#ifndef STANDALONE_TESTS
  // In production, use the global platform provider
  if (!platformProvider) {
    platformProvider = IWebPlatformProvider::instance;
  }
#endif
}

AudioModule::~AudioModule() {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  if (playbackEngine) {
    delete playbackEngine;
  }
  if (recordingEngine) {
    delete recordingEngine;
  }
#endif
}

void AudioModule::begin() { begin(JsonVariant()); }

void AudioModule::begin(const JsonVariant &configJson) {
  Serial.println("Audio Module: Initializing...");

  // Parse configuration
  parseConfig(configJson);

#if defined(ARDUINO) || defined(ESP_PLATFORM)
  // Create playback engine (only available in Arduino/ESP32 builds)
  playbackEngine = new PlaybackEngine();

  // Initialize I2S output
  bool success =
      playbackEngine->begin(config.i2s_output.bck_pin, config.i2s_output.ws_pin,
                            config.i2s_output.data_pin, config.sample_rate,
                            config.bits_per_sample, config.channels);

  if (!success) {
    Serial.println("Audio Module: Failed to initialize playback engine");
    return;
  }

  // Set initial volume
  playbackEngine->setVolume(config.volume);

  Serial.println("Audio Module: Initialized successfully");
#else
  // Native tests: Skip hardware initialization
  Serial.println("Audio Module: Initialized (native test mode)");
#endif
}

void AudioModule::handle() {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  if (playbackEngine) {
    playbackEngine->handle();
  }

  if (recordingEngine) {
    // TODO: Handle recording engine
  }
#endif
}

void AudioModule::parseConfig(const JsonVariant &configJson) {
  if (configJson.isNull()) {
    Serial.println("Audio Module: Using default configuration");
    return;
  }

  // Parse I2S output configuration
  if (configJson.containsKey("i2s_output")) {
    JsonVariantConst output = configJson["i2s_output"];
    if (output.containsKey("bck_pin"))
      config.i2s_output.bck_pin = output["bck_pin"];
    if (output.containsKey("ws_pin"))
      config.i2s_output.ws_pin = output["ws_pin"];
    if (output.containsKey("data_pin"))
      config.i2s_output.data_pin = output["data_pin"];
  }

  // Parse I2S input configuration
  if (configJson.containsKey("i2s_input")) {
    JsonVariantConst input = configJson["i2s_input"];
    if (input.containsKey("bck_pin"))
      config.i2s_input.bck_pin = input["bck_pin"];
    if (input.containsKey("ws_pin"))
      config.i2s_input.ws_pin = input["ws_pin"];
    if (input.containsKey("data_pin"))
      config.i2s_input.data_pin = input["data_pin"];
  }

  // Parse audio settings
  if (configJson.containsKey("sample_rate")) {
    config.sample_rate = configJson["sample_rate"];
  }
  if (configJson.containsKey("bits_per_sample")) {
    config.bits_per_sample = configJson["bits_per_sample"];
  }
  if (configJson.containsKey("channels")) {
    config.channels = configJson["channels"];
  }
  if (configJson.containsKey("volume")) {
    config.volume = configJson["volume"];
  }

  Serial.println("Audio Module: Configuration loaded");
}

void AudioModule::loadStoredConfig() {
  // TODO: Load from storage in future phase
}

void AudioModule::saveConfig() {
  // TODO: Save to storage in future phase
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool AudioModule::play(const String &source, int volume) {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  if (!playbackEngine) {
    Serial.println("Audio Module: Playback engine not initialized");
    return false;
  }

  // Set volume if specified
  if (volume >= 0) {
    setVolume(volume);
  }

  // Determine source type and play
  if (source.startsWith("/")) {
    // File path
    return playbackEngine->playFile(source);
  } else if (source.startsWith("tone:")) {
    // Test tone format: "tone:440" for 440Hz
    float frequency = source.substring(5).toFloat();
    if (frequency <= 0)
      frequency = 440.0;
    return playbackEngine->playSineWave(frequency);
  } else {
    Serial.println("Audio Module: Unknown source type");
    return false;
  }
#else
  // Native tests: No actual playback
  return false;
#endif
}

void AudioModule::stop() {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  if (playbackEngine) {
    playbackEngine->stop();
  }
#endif
}

void AudioModule::pause() {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  if (playbackEngine) {
    playbackEngine->pause();
  }
#endif
}

void AudioModule::resume() {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  if (playbackEngine) {
    playbackEngine->resume();
  }
#endif
}

void AudioModule::setVolume(int level) {
  config.volume = constrain(level, 0, 100);
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  if (playbackEngine) {
    playbackEngine->setVolume(config.volume);
  }
#endif
}

int AudioModule::getVolume() const { return config.volume; }

bool AudioModule::isPlaying() const {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  return playbackEngine && playbackEngine->isPlaying();
#else
  return false;
#endif
}

bool AudioModule::isPaused() const {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  return playbackEngine && playbackEngine->isPaused();
#else
  return false;
#endif
}

bool AudioModule::record(const String &destination, unsigned long durationMs) {
  // TODO: Implement in recording phase
  Serial.println("Audio Module: Recording not yet implemented");
  return false;
}

bool AudioModule::isRecording() const {
  // TODO: Implement in recording phase
  return false;
}

// ============================================================================
// Web Route Handlers
// ============================================================================

std::vector<RouteVariant> AudioModule::getHttpRoutes() {
  return {// Main status page
          WebRoute("/", WebModule::WM_GET,
                   [this](RequestT &req, ResponseT &res) {
                     statusPageHandler(req, res);
                   },
                   {AuthType::SESSION}),

          // API endpoints
          ApiRoute("/api/status", WebModule::WM_GET,
                   [this](RequestT &req, ResponseT &res) {
                     statusApiHandler(req, res);
                   },
                   {AuthType::SESSION, AuthType::TOKEN}),

          ApiRoute("/api/play", WebModule::WM_POST,
                   [this](RequestT &req, ResponseT &res) {
                     playApiHandler(req, res);
                   },
                   {AuthType::SESSION, AuthType::TOKEN}),

          ApiRoute("/api/stop", WebModule::WM_POST,
                   [this](RequestT &req, ResponseT &res) {
                     stopApiHandler(req, res);
                   },
                   {AuthType::SESSION, AuthType::TOKEN}),

          ApiRoute("/api/pause", WebModule::WM_POST,
                   [this](RequestT &req, ResponseT &res) {
                     pauseApiHandler(req, res);
                   },
                   {AuthType::SESSION, AuthType::TOKEN}),

          ApiRoute("/api/volume", WebModule::WM_POST,
                   [this](RequestT &req, ResponseT &res) {
                     volumeApiHandler(req, res);
                   },
                   {AuthType::SESSION, AuthType::TOKEN})};
}

std::vector<RouteVariant> AudioModule::getHttpsRoutes() {
  return getHttpRoutes();
}

void AudioModule::statusPageHandler(RequestT &req, ResponseT &res) {
  res.setProgmemContent(AUDIO_STATUS_HTML, "text/html");
}

void AudioModule::statusApiHandler(RequestT &req, ResponseT &res) {
  respondJson(res, [this](JsonObject &json) {
    json["playing"] = isPlaying();
    json["paused"] = isPaused();
    json["recording"] = isRecording();
    json["volume"] = getVolume();

#if defined(ARDUINO) || defined(ESP_PLATFORM)
    if (playbackEngine) {
      const char *stateStr = "stopped";
      if (playbackEngine->getState() == PlaybackEngine::State::PLAYING) {
        stateStr = "playing";
      } else if (playbackEngine->getState() == PlaybackEngine::State::PAUSED) {
        stateStr = "paused";
      }
      json["playback_state"] = stateStr;

      const char *sourceStr = "none";
      if (playbackEngine->getSourceType() ==
          PlaybackEngine::SourceType::SINE_WAVE) {
        sourceStr = "sine_wave";
      } else if (playbackEngine->getSourceType() ==
                 PlaybackEngine::SourceType::FILE) {
        sourceStr = "file";
      }
      json["source_type"] = sourceStr;
    } else
#endif
    {
      // Default values for native tests or when playbackEngine is null
      json["playback_state"] = "stopped";
      json["source_type"] = "none";
    }
  });
}

void AudioModule::playApiHandler(RequestT &req, ResponseT &res) {
#ifdef NATIVE_PLATFORM
  std::string body = req.getBody();
#else
  String body = req.getBody();
#endif

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, body.c_str());

  if (error) {
    res.setStatus(400);
    respondJson(res, [](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Invalid JSON";
    });
    return;
  }

  String source = doc["source"] | "tone:440";
  int volume = doc["volume"] | -1;

  bool success = play(source, volume);

  respondJson(res, [success](JsonObject &json) {
    json["success"] = success;
    if (success) {
      json["message"] = "Playback started";
    } else {
      json["error"] = "Failed to start playback";
    }
  });
}

void AudioModule::stopApiHandler(RequestT &req, ResponseT &res) {
  stop();

  respondJson(res, [](JsonObject &json) {
    json["success"] = true;
    json["message"] = "Playback stopped";
  });
}

void AudioModule::pauseApiHandler(RequestT &req, ResponseT &res) {
  if (isPlaying()) {
    pause();
  } else if (isPaused()) {
    resume();
  }

  respondJson(res, [this](JsonObject &json) {
    json["success"] = true;
    json["paused"] = isPaused();
  });
}

void AudioModule::volumeApiHandler(RequestT &req, ResponseT &res) {
#ifdef NATIVE_PLATFORM
  std::string body = req.getBody();
#else
  String body = req.getBody();
#endif

  DynamicJsonDocument doc(128);
  DeserializationError error = deserializeJson(doc, body.c_str());

  if (error || !doc.containsKey("level")) {
    res.setStatus(400);
    respondJson(res, [](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Missing 'level' parameter";
    });
    return;
  }

  int level = doc["level"];
  setVolume(level);

  respondJson(res, [this](JsonObject &json) {
    json["success"] = true;
    json["volume"] = getVolume();
  });
}

void AudioModule::recordApiHandler(RequestT &req, ResponseT &res) {
  // TODO: Implement in recording phase
  respondJson(res, [](JsonObject &json) {
    json["success"] = false;
    json["error"] = "Recording not yet implemented";
  });
}

void AudioModule::filesApiHandler(RequestT &req, ResponseT &res) {
  // TODO: Implement file listing in file playback phase
  respondJson(res, [](JsonObject &json) {
    json["success"] = false;
    json["error"] = "File listing not yet implemented";
  });
}
