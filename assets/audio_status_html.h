#ifndef AUDIO_STATUS_HTML_H
#define AUDIO_STATUS_HTML_H

#include <Arduino.h>

const char AUDIO_STATUS_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="/assets/style.css" type="text/css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
    <script src="/assets/web-platform-utils.js"></script>
    <title>Audio Test - {{DEVICE_NAME}}</title>
</head>
<body>
    {{NAV_MENU}}
    
    <div class="container">
        <h1>Audio Test Interface</h1>
        
        <!-- Status Card -->
        <div class="status-card">
            <h2>Playback Status</h2>
            <div class="status-grid">
                <div class="status-item">
                    <label>State:</label>
                    <span id="playback-state">Stopped</span>
                </div>
                <div class="status-item">
                    <label>Source:</label>
                    <span id="source-type">None</span>
                </div>
                <div class="status-item">
                    <label>Volume:</label>
                    <span id="current-volume">75%</span>
                </div>
            </div>
        </div>

        <!-- Tone Generator Card -->
        <div class="card">
            <h2>Tone Generator</h2>
            <div class="form-group">
                <label for="frequency">Frequency (Hz):</label>
                <input type="number" id="frequency" value="440" min="20" max="20000" step="1">
                <small>Range: 20 Hz - 20 kHz (A4 = 440 Hz)</small>
            </div>
            
            <div class="form-group">
                <label for="duration">Duration (seconds):</label>
                <input type="number" id="duration" value="5" min="1" max="60" step="1">
                <small>Range: 1 - 60 seconds (0 = continuous)</small>
            </div>

            <div class="form-group">
                <label for="volume">Volume:</label>
                <input type="range" id="volume" min="0" max="100" value="75" step="1">
                <span id="volume-display">75%</span>
            </div>

            <div class="button-group">
                <button id="play-btn" class="btn btn-primary">Play Tone</button>
                <button id="stop-btn" class="btn btn-secondary" disabled>Stop</button>
            </div>
        </div>

        <!-- Quick Test Buttons -->
        <div class="card">
            <h2>Quick Tests</h2>
            <div class="button-group">
                <button class="btn btn-secondary btn-sm" onclick="playNote('C4', 261.63)">C4 (261 Hz)</button>
                <button class="btn btn-secondary btn-sm" onclick="playNote('A4', 440.00)">A4 (440 Hz)</button>
                <button class="btn btn-secondary btn-sm" onclick="playNote('C5', 523.25)">C5 (523 Hz)</button>
                <button class="btn btn-secondary btn-sm" onclick="playNote('1kHz', 1000)">1 kHz</button>
            </div>
        </div>
    </div>

    <script>
        let statusInterval = null;
        let stopTimer = null;

        // Update status periodically
        async function updateStatus() {
            try {
                const response = await AuthUtils.fetch('/audio/api/status');
                const data = await response.json();
                
                document.getElementById('playback-state').textContent = 
                    data.playback_state.charAt(0).toUpperCase() + data.playback_state.slice(1);
                document.getElementById('source-type').textContent = 
                    data.source_type.replace('_', ' ').charAt(0).toUpperCase() + 
                    data.source_type.replace('_', ' ').slice(1);
                document.getElementById('current-volume').textContent = data.volume + '%';

                // Update button states
                const isPlaying = data.playing;
                document.getElementById('play-btn').disabled = isPlaying;
                document.getElementById('stop-btn').disabled = !isPlaying;

            } catch (error) {
                console.error('Failed to get status:', error);
            }
        }

        // Play tone
        async function playTone() {
            const frequency = parseFloat(document.getElementById('frequency').value);
            const duration = parseInt(document.getElementById('duration').value);
            const volume = parseInt(document.getElementById('volume').value);

            if (frequency < 20 || frequency > 20000) {
                UIUtils.showAlert('Frequency must be between 20 and 20000 Hz', 'error');
                return;
            }

            try {
                const response = await AuthUtils.fetch('/audio/api/play', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        source: `tone:${frequency}`,
                        volume: volume
                    })
                });

                const result = await response.json();
                
                if (result.success) {
                    updateStatus();
                    
                    // Set auto-stop timer if duration specified
                    if (duration > 0) {
                        stopTimer = setTimeout(() => {
                            stopTone();
                        }, duration * 1000);
                    }
                } else {
                    UIUtils.showAlert('Failed to play tone: ' + (result.error || 'Unknown error'), 'error');
                }

            } catch (error) {
                console.error('Failed to play tone:', error);
                UIUtils.showAlert('Failed to play tone', 'error');
            }
        }

        // Stop tone
        async function stopTone() {
            if (stopTimer) {
                clearTimeout(stopTimer);
                stopTimer = null;
            }

            try {
                const response = await AuthUtils.fetch('/audio/api/stop', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'}
                });

                const result = await response.json();
                updateStatus();

            } catch (error) {
                console.error('Failed to stop tone:', error);
            }
        }

        // Set volume
        async function setVolume(level) {
            try {
                const response = await AuthUtils.fetch('/audio/api/volume', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({level: level})
                });

                const result = await response.json();
                if (result.success) {
                    document.getElementById('current-volume').textContent = result.volume + '%';
                }

            } catch (error) {
                console.error('Failed to set volume:', error);
            }
        }

        // Quick test helper
        function playNote(name, freq) {
            document.getElementById('frequency').value = freq;
            document.getElementById('duration').value = 2; // 2 second test
            playTone();
        }

        // Event listeners
        document.getElementById('play-btn').addEventListener('click', playTone);
        document.getElementById('stop-btn').addEventListener('click', stopTone);
        
        document.getElementById('volume').addEventListener('input', function() {
            const value = this.value;
            document.getElementById('volume-display').textContent = value + '%';
        });

        document.getElementById('volume').addEventListener('change', function() {
            setVolume(parseInt(this.value));
        });

        // Initialize
        updateStatus();
        statusInterval = setInterval(updateStatus, 1000);

        // Cleanup on page unload
        window.addEventListener('beforeunload', () => {
            if (statusInterval) clearInterval(statusInterval);
            if (stopTimer) clearTimeout(stopTimer);
        });
    </script>

    <style>
        .form-group {
            margin-bottom: 20px;
        }

        .form-group label {
            display: block;
            margin-bottom: 5px;
            font-weight: 500;
        }

        .form-group input[type="number"],
        .form-group input[type="range"] {
            width: 100%;
            padding: 8px;
            font-size: 1rem;
            border: 1px solid rgba(255, 255, 255, 0.2);
            background: rgba(255, 255, 255, 0.05);
            color: #fff;
            border-radius: 4px;
        }

        .form-group input[type="range"] {
            padding: 0;
        }

        .form-group small {
            display: block;
            margin-top: 5px;
            color: rgba(255, 255, 255, 0.6);
            font-size: 0.875rem;
        }

        .button-group {
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
        }

        .button-group button {
            flex: 1;
            min-width: 100px;
        }

        #volume-display {
            margin-left: 10px;
            font-weight: 500;
        }

        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }

        .status-item {
            padding: 10px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 4px;
        }

        .status-item label {
            display: block;
            font-size: 0.875rem;
            color: rgba(255, 255, 255, 0.6);
            margin-bottom: 5px;
        }

        .status-item span {
            font-size: 1.1rem;
            font-weight: 500;
        }
    </style>
</body>
</html>)rawliteral";

#endif // AUDIO_STATUS_HTML_H
