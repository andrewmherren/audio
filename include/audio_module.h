#ifndef AUDIO_MODULE_H
#define AUDIO_MODULE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <interface/auth_types.h>
#include <interface/utils/route_variant.h>
#include <interface/web_module_interface.h>
#include <web_platform_interface.h>

// Type aliases for request/response (support both native and ESP32)
#ifdef NATIVE_PLATFORM
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
using RequestT = WebRequestCore;
using ResponseT = WebResponseCore;
#else
using RequestT = WebRequest;
using ResponseT = WebResponse;
#endif

// Forward declarations
class PlaybackEngine;
class RecordingEngine;

/**
 * @brief Audio Module - Modular audio recording and playback system
 *
 * Provides audio recording (I2S microphone), playback (I2S amplifier),
 * and file management through a web interface. Designed with internal
 * modular architecture for easy extension.
 */
class AudioModule : public IWebModule {
public:
  // Constructor for dependency injection
  explicit AudioModule(IWebPlatformProvider *provider = nullptr);
  ~AudioModule() override;

  // IWebModule lifecycle
  using IWebModule::begin;
  void begin() override;
  void begin(const JsonVariant &config) override;
  void handle() override;

  // IWebModule interface
  std::vector<RouteVariant> getHttpRoutes() override;
  std::vector<RouteVariant> getHttpsRoutes() override;
  String getModuleName() const override { return "Audio Module"; }
  String getModuleVersion() const override { return "0.1.0"; }
  String getModuleDescription() const override {
    return "Audio recording and playback with I2S support";
  }

  // ========================================================================
  // Public API for programmatic control (used by parent applications)
  // ========================================================================

  /**
   * @brief Start audio playback from a source
   * @param source File path (e.g., "/recording.wav") or future: stream URL
   * @param volume Volume level 0-100 (optional, uses current volume if omitted)
   * @return true if playback started successfully
   */
  bool play(const String &source, int volume = -1);

  /**
   * @brief Stop current playback or recording
   */
  void stop();

  /**
   * @brief Pause current playback (can be resumed)
   */
  void pause();

  /**
   * @brief Resume paused playback
   */
  void resume();

  /**
   * @brief Set volume level
   * @param level Volume 0-100
   */
  void setVolume(int level);

  /**
   * @brief Get current volume level
   * @return Volume 0-100
   */
  int getVolume() const;

  /**
   * @brief Check if audio is currently playing
   */
  bool isPlaying() const;

  /**
   * @brief Check if audio is currently paused
   */
  bool isPaused() const;

  /**
   * @brief Start recording audio to a file
   * @param destination File path (e.g., "/recording.wav")
   * @param durationMs Duration in milliseconds (0 = until stop() called)
   * @return true if recording started successfully
   */
  bool record(const String &destination, unsigned long durationMs = 0);

  /**
   * @brief Check if currently recording
   */
  bool isRecording() const;

  // Route handler methods (public for testing)
  void statusPageHandler(RequestT &req, ResponseT &res);
  void statusApiHandler(RequestT &req, ResponseT &res);
  void playApiHandler(RequestT &req, ResponseT &res);
  void stopApiHandler(RequestT &req, ResponseT &res);
  void pauseApiHandler(RequestT &req, ResponseT &res);
  void volumeApiHandler(RequestT &req, ResponseT &res);
  void recordApiHandler(RequestT &req, ResponseT &res);
  void filesApiHandler(RequestT &req, ResponseT &res);

private:
  // Platform provider for dependency injection
  IWebPlatformProvider *platformProvider;

  // Engine components (modular internal architecture)
  PlaybackEngine *playbackEngine;
  RecordingEngine *recordingEngine;

  // Configuration
  struct AudioConfig {
    // I2S Output (speaker/amplifier)
    struct {
      int bck_pin;
      int ws_pin;
      int data_pin;
    } i2s_output;

    // I2S Input (microphone)
    struct {
      int bck_pin;
      int ws_pin;
      int data_pin;
    } i2s_input;

    int sample_rate;
    int bits_per_sample;
    int channels;
    int volume; // 0-100

    AudioConfig()
        : sample_rate(16000), bits_per_sample(16), channels(1), volume(75) {
      // Default pins (can be overridden via config)
      i2s_output = {26, 25, 22};
      i2s_input = {14, 15, 32};
    }
  } config;

  // Configuration management
  void parseConfig(const JsonVariant &configJson);
  void loadStoredConfig();
  void saveConfig();

  // Helper to access platform
  IWebPlatform &getPlatform() const { return platformProvider->getPlatform(); }

  // Helper for JSON responses
  template <typename Fn>
  inline void respondJson(ResponseT &res, Fn &&fn) const {
    IWebPlatformProvider::getPlatformInstance().createJsonResponse(
        res, std::forward<Fn>(fn));
  }
};

// Global instance for production builds
#if defined(ARDUINO) || defined(ESP_PLATFORM)
extern AudioModule audioModule;
#endif

#endif // AUDIO_MODULE_H
