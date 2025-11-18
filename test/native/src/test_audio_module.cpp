#include <unity.h>

#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <audio_module.h>
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
#include <testing/testing_platform_provider.h>

using namespace fakeit;

// ===========================================================================
// Module Metadata Tests
// ===========================================================================

static void test_module_metadata() {
  // Use default constructor relying on global provider set in setUp
  AudioModule module;
  TEST_ASSERT_EQUAL_STRING("Audio Module", module.getModuleName().c_str());
  TEST_ASSERT_EQUAL_STRING("0.1.0", module.getModuleVersion().c_str());
  TEST_ASSERT_TRUE_MESSAGE(module.getModuleDescription().length() > 0,
                           "Description should be non-empty");
}

// ===========================================================================
// Configuration Tests
// ===========================================================================

static void test_default_configuration() {
  AudioModule module;
  // Module should initialize with default configuration
  // Default volume is 75%
  TEST_ASSERT_EQUAL(75, module.getVolume());
}

static void test_parse_i2s_output_config() {
  AudioModule module;

  StaticJsonDocument<256> doc;
  doc["i2s_output"]["bck_pin"] = 10;
  doc["i2s_output"]["ws_pin"] = 11;
  doc["i2s_output"]["data_pin"] = 12;
  doc["sample_rate"] = 44100;
  doc["bits_per_sample"] = 24;
  doc["channels"] = 2;
  doc["volume"] = 50;

  // This should parse config successfully
  // Note: begin() will fail because we can't create PlaybackEngine in native
  // tests, but parseConfig should work
  module.begin(doc.as<JsonVariant>());

  // Volume should be updated from config
  TEST_ASSERT_EQUAL(50, module.getVolume());
}

static void test_parse_empty_config() {
  AudioModule module;
  JsonVariant emptyConfig;

  // Should not crash with empty config
  module.begin(emptyConfig);

  // Should use default volume
  TEST_ASSERT_EQUAL(75, module.getVolume());
}

// ===========================================================================
// State Management Tests
// ===========================================================================

static void test_initial_state() {
  AudioModule module;

  // Initial state should be stopped, not playing, not recording
  TEST_ASSERT_FALSE(module.isPlaying());
  TEST_ASSERT_FALSE(module.isPaused());
  TEST_ASSERT_FALSE(module.isRecording());
}

static void test_volume_range() {
  AudioModule module;

  // Volume should accept valid range
  module.setVolume(0);
  TEST_ASSERT_EQUAL(0, module.getVolume());

  module.setVolume(100);
  TEST_ASSERT_EQUAL(100, module.getVolume());

  module.setVolume(50);
  TEST_ASSERT_EQUAL(50, module.getVolume());
}

// ===========================================================================
// Source Type Detection Tests (Business Logic)
// ===========================================================================

static void test_detect_sine_wave_source() {
  AudioModule module;

  // These sources should be detected as sine waves
  // Format: "tone:frequency"
  String tone440 = "tone:440";
  String tone1000 = "tone:1000";
  String toneFloat = "tone:440.5";

  // We can't actually play in native tests, but we can verify
  // the source format is correct by checking the play() doesn't crash
  // and returns false (because PlaybackEngine is null in native tests)
  TEST_ASSERT_FALSE(module.play(tone440));
  TEST_ASSERT_FALSE(module.play(tone1000));
  TEST_ASSERT_FALSE(module.play(toneFloat));
}

static void test_detect_file_source() {
  AudioModule module;

  // These sources should be detected as files
  // Format: starts with "/"
  String recording = "/recording.wav";
  String music = "/music/song.wav";

  // Should return false because PlaybackEngine is null in native tests
  TEST_ASSERT_FALSE(module.play(recording));
  TEST_ASSERT_FALSE(module.play(music));
}

// ===========================================================================
// Route Registration Tests
// ===========================================================================

static void test_routes_built_and_sizes() {
  AudioModule module;
  auto http = module.getHttpRoutes();
  auto https = module.getHttpsRoutes();

  // Should have routes for:
  // - Status page (/)
  // - Status API (/api/status)
  // - Play API (/api/play)
  // - Stop API (/api/stop)
  // - Pause API (/api/pause)
  // - Volume API (/api/volume)
  TEST_ASSERT_GREATER_OR_EQUAL(6, http.size());
  TEST_ASSERT_EQUAL(http.size(), https.size());
}

static void test_http_routes_structure() {
  AudioModule module;
  auto routes = module.getHttpRoutes();

  for (const auto &route : routes) {
    TEST_ASSERT_TRUE(route.isWebRoute() || route.isApiRoute());
  }
}

// ===========================================================================
// API Handler Tests
// ===========================================================================

static void test_statusApiHandler_returns_json() {
  AudioModule module;
  WebRequestCore req;
  WebResponseCore res;

  module.statusApiHandler(req, res);

  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");

  // Should contain state information
  TEST_ASSERT_TRUE(doc.containsKey("playing"));
  TEST_ASSERT_TRUE(doc.containsKey("paused"));
  TEST_ASSERT_TRUE(doc.containsKey("recording"));
  TEST_ASSERT_TRUE(doc.containsKey("volume"));
  TEST_ASSERT_TRUE(doc.containsKey("playback_state"));
  TEST_ASSERT_TRUE(doc.containsKey("source_type"));

  // Initial state should be stopped, not playing
  TEST_ASSERT_FALSE(doc["playing"].as<bool>());
  TEST_ASSERT_FALSE(doc["paused"].as<bool>());
  TEST_ASSERT_FALSE(doc["recording"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("stopped", doc["playback_state"]);
  TEST_ASSERT_EQUAL_STRING("none", doc["source_type"]);
}

static void test_stopApiHandler_returns_json() {
  AudioModule module;
  WebRequestCore req;
  WebResponseCore res;

  module.stopApiHandler(req, res);

  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");

  TEST_ASSERT_TRUE(doc.containsKey("success"));
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
}

static void test_volumeApiHandler_with_valid_level() {
  AudioModule module;
  WebRequestCore req;
  WebResponseCore res;

  // Create request body with volume level
  StaticJsonDocument<128> reqDoc;
  reqDoc["level"] = 80;
  std::string reqBody;
  serializeJson(reqDoc, reqBody);
  req.setBody(reqBody);

  module.volumeApiHandler(req, res);

  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());

  StaticJsonDocument<256> resDoc;
  DeserializationError err = deserializeJson(resDoc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");

  TEST_ASSERT_TRUE(resDoc.containsKey("success"));
  TEST_ASSERT_TRUE(resDoc["success"].as<bool>());
  TEST_ASSERT_EQUAL(80, resDoc["volume"].as<int>());

  // Verify volume was actually set
  TEST_ASSERT_EQUAL(80, module.getVolume());
}

static void test_volumeApiHandler_missing_level() {
  AudioModule module;
  WebRequestCore req;
  WebResponseCore res;

  // Create request body WITHOUT level parameter
  StaticJsonDocument<128> reqDoc;
  std::string reqBody;
  serializeJson(reqDoc, reqBody);
  req.setBody(reqBody);

  module.volumeApiHandler(req, res);

  TEST_ASSERT_EQUAL(400, res.getStatus());

  StaticJsonDocument<256> resDoc;
  DeserializationError err = deserializeJson(resDoc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");

  TEST_ASSERT_FALSE(resDoc["success"].as<bool>());
  TEST_ASSERT_TRUE(resDoc.containsKey("error"));
}

static void test_playApiHandler_with_tone() {
  AudioModule module;
  WebRequestCore req;
  WebResponseCore res;

  // Create request body with tone source
  StaticJsonDocument<128> reqDoc;
  reqDoc["source"] = "tone:440";
  reqDoc["volume"] = 60;
  std::string reqBody;
  serializeJson(reqDoc, reqBody);
  req.setBody(reqBody);

  module.playApiHandler(req, res);

  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());

  StaticJsonDocument<256> resDoc;
  DeserializationError err = deserializeJson(resDoc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");

  // Will fail because PlaybackEngine is null, but should handle gracefully
  TEST_ASSERT_FALSE(resDoc["success"].as<bool>());
  TEST_ASSERT_TRUE(resDoc.containsKey("error"));
}

// ===========================================================================
// Lifecycle Tests
// ===========================================================================

static void test_handle_completes() {
  AudioModule module;
  // handle() should not crash even if nothing is initialized
  module.handle();
  TEST_ASSERT_TRUE(true);
}

// ===========================================================================
// Test Registration
// ===========================================================================

void register_audio_module_tests() {
  // Metadata
  RUN_TEST(test_module_metadata);

  // Configuration
  RUN_TEST(test_default_configuration);
  RUN_TEST(test_parse_i2s_output_config);
  RUN_TEST(test_parse_empty_config);

  // State Management
  RUN_TEST(test_initial_state);
  RUN_TEST(test_volume_range);

  // Source Detection (Business Logic)
  RUN_TEST(test_detect_sine_wave_source);
  RUN_TEST(test_detect_file_source);

  // Routes
  RUN_TEST(test_routes_built_and_sizes);
  RUN_TEST(test_http_routes_structure);

  // API Handlers
  RUN_TEST(test_statusApiHandler_returns_json);
  RUN_TEST(test_stopApiHandler_returns_json);
  RUN_TEST(test_volumeApiHandler_with_valid_level);
  RUN_TEST(test_volumeApiHandler_missing_level);
  RUN_TEST(test_playApiHandler_with_tone);

  // Lifecycle
  RUN_TEST(test_handle_completes);
}

#endif // NATIVE_PLATFORM
