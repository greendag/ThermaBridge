#pragma once

// Initialize subsystems in small steps. Each function returns true on success
// or false when it has handed control over to provisioning or rebooted.
bool initSerial();
bool initPins();
bool initLedAndSystem();
bool initFileSystem();
// Returns true if config loaded successfully. If false, provisioning should start.
bool tryLoadConfig();
// Attempt to connect to WiFi using cfg; returns true if connected.
bool tryConnectWifi(unsigned long timeoutMs = 15000);
