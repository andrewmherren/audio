# Audio Module

⚠️ **EARLY DEVELOPMENT - INTERNAL NOTES**

## Multiple Instance Pattern (IMPORTANT)

This module is designed to support **multiple independent instances** in the same application. This is a key feature for audio pipeline architectures.

**Good news**: All web_platform modules already support this pattern! The public constructors allow application-side instantiation, while the default global instance provides convenience for simple cases.

### Usage Patterns

**Simple Single Instance (use default global):**
```cpp
#include <audio_module.h>

void setup() {
    JsonDocument config;
    config["mode"] = "speaker";
    
    // Use the provided global instance
    webPlatform.registerModule("/audio", &audioModule, config);
    webPlatform.finalizeRoutes();
}
```

**Multiple Instances (create your own):**
```cpp
#include <audio_module.h>

// Create multiple instances - each maintains independent state
AudioModule audioSource;
AudioModule audioTransform;
AudioModule audioSink;

void setup() {
    // Configure each instance with different roles
    JsonDocument sourceConfig;
    sourceConfig["mode"] = "source";
    sourceConfig["input"] = "microphone";
    
    JsonDocument transformConfig;
    transformConfig["mode"] = "transform";
    transformConfig["effect"] = "reverb";
    
    JsonDocument sinkConfig;
    sinkConfig["mode"] = "sink";
    sinkConfig["output"] = "speaker";
    
    // Register each at different paths
    webPlatform.registerModule("/audio-source", &audioSource, sourceConfig);
    webPlatform.registerModule("/audio-transform", &audioTransform, transformConfig);
    webPlatform.registerModule("/audio-sink", &audioSink, sinkConfig);
    
    webPlatform.finalizeRoutes();
}
```

**Hybrid (mix global + custom instances):**
```cpp
AudioModule mic, mixer;  // Create two custom

void setup() {
    webPlatform.registerModule("/mic", &mic, micConfig);
    webPlatform.registerModule("/mixer", &mixer, mixerConfig);
    webPlatform.registerModule("/output", &audioModule, outConfig);  // Use global
}
```

### Key Implementation Notes

1. **Default Global Instance**: The library provides a default `audioModule` global instance for simple single-instance use cases. This maintains consistency with other modules.

2. **Multiple Instances Supported**: The public constructor allows applications to create additional instances as needed. No module changes required!

3. **Flexible Usage**:
   - Simple case: Use the provided `audioModule` global
   - Multiple instances: Create your own via `AudioModule source, transform, sink;`
   - Mixed: Use the global for one role, create additional instances for others

4. **Independent State**: Each instance (including the global) maintains completely independent state (config, buffers, connections, etc.).

4. **Pipeline Architecture**: Common pattern is source → transform → sink:
   - **Source**: Microphone, streaming input, file playback
   - **Transform**: Effects, filters, mixing, routing
   - **Sink**: Speaker output, file recording, streaming output

5. **Inter-Instance Communication**: Instances can communicate via:
   - Direct method calls (if references are shared)
   - Internal HTTP requests through platform
   - Shared storage/state if needed

### Example Pipeline Configurations

**Microphone → Reverb → Speaker:**
```cpp
AudioModule mic, reverb, speaker;
// Configure: mic as source, reverb as transform, speaker as sink
```

**Dual Recording with Mixing:**
```cpp
AudioModule mic1, mic2, mixer, recorder;
// mic1 + mic2 → mixer → recorder
```

**Streaming with Effects:**
```cpp
AudioModule stream, equalizer, compressor, output;
// stream → equalizer → compressor → output
```

---

## TODO
- [ ] Define audio pipeline interface (source/transform/sink)
- [ ] Implement buffer management per instance
- [ ] Add inter-instance connection API
- [ ] Test with 3+ simultaneous instances
- [ ] Memory profiling with multiple instances