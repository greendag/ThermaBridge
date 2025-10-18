#include "provisioning.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Preferences.h>

static WebServer server(80);
static DNSServer dnsServer;
static Preferences prefs;
static bool provisioningActive = false;
static WebServer statusServer(80);
static bool statusServerActive = false;

const byte DNS_PORT = 53;

static String macSuffixHex()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[5];
    snprintf(buf, sizeof(buf), "%02X%02X", mac[4], mac[5]);
    return String(buf);
}

void handleNotFound()
{
    // redirect any unknown path to root
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
}

void handleSave()
{
    if (server.method() != HTTP_POST)
    {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }
    Config cfg;
    cfg.ssid = server.arg("ssid");
    cfg.psk = server.arg("psk");
    cfg.devname = server.arg("devname");
    if (cfg.ssid.length() == 0)
    {
        server.send(400, "text/plain", "SSID required");
        return;
    }

    if (!saveConfig(cfg))
    {
        server.send(500, "text/plain", "Failed to save config");
        return;
    }

    String rsp = "<html><body><h3>Saved. Attempting to connect to ";
    rsp += cfg.ssid;
    rsp += " ...</h3><p>If successful, the device will reboot.</p></body></html>";
    server.send(200, "text/html", rsp);

    // Attempt connection
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid.c_str(), cfg.psk.c_str());

    unsigned long start = millis();
    const unsigned long CONNECT_TIMEOUT = 15000; // 15s
    while (millis() - start < CONNECT_TIMEOUT)
    {
        if (WiFi.status() == WL_CONNECTED)
            break;
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        // connected successfully
        Serial.println("Provisioning: connected to WiFi, rebooting...");
        delay(1000);
        ESP.restart();
    }
    else
    {
        Serial.println("Provisioning: failed to connect");
    }
}

void handleStatus()
{
    // Log incoming request for diagnostics
    IPAddress remote = server.client().remoteIP();
    Serial.print("/status request from: ");
    Serial.println(remote.toString());
    Serial.print("Method: ");
    Serial.println(server.method() == HTTP_GET ? "GET" : "OTHER");
    Serial.print("URI: ");
    Serial.println(server.uri());
    // Print all headers
    int headers = server.headers();
    Serial.print("Headers (count=");
    Serial.print(headers);
    Serial.println(")");
    for (int i = 0; i < headers; ++i)
    {
        String name = server.headerName(i);
        String value = server.header(i);
        Serial.print("  ");
        Serial.print(name);
        Serial.print(": ");
        Serial.println(value);
    }
    // Print query args
    int args = server.args();
    Serial.print("Args (count=");
    Serial.print(args);
    Serial.println(")");
    for (int i = 0; i < args; ++i)
    {
        Serial.print("  ");
        Serial.print(server.argName(i));
        Serial.print(" = ");
        Serial.println(server.arg(i));
    }

    // Return JSON with masked SSID/PSK and connection status
    Config current;
    bool has = loadConfig(current);

    String json = "{";
    json += "\"configured\":" + String(has ? "true" : "false") + ",";
    if (has)
    {
        // mask PSK for safety
        String pskMasked = current.psk;
        if (pskMasked.length() > 0)
        {
            for (size_t i = 0; i < pskMasked.length(); ++i)
                pskMasked.setCharAt(i, '*');
        }
        json += "\"ssid\":\"" + current.ssid + "\",";
        json += "\"psk_masked\":\"" + pskMasked + "\",";
        json += "\"devname\":\"" + current.devname + "\",";
    }
    else
    {
        json += "\"ssid\":\"\",";
        json += "\"psk_masked\":\"\",";
        json += "\"devname\":\"\",";
    }

    int status = WiFi.status();
    json += "\"wifi_status\":" + String(status) + ",";
    if (status == WL_CONNECTED)
    {
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    }
    else
    {
        json += "\"ip\":\"\"";
    }
    json += "}";

    server.send(200, "application/json", json);
}

void handleHealth()
{
    unsigned long upSec = millis() / 1000UL;
    size_t freeHeap = ESP.getFreeHeap();
    String json = "{";
    json += "\"uptime_s\": " + String(upSec) + ",";
    json += "\"free_heap\": " + String(freeHeap);
    json += "}";
    Serial.print("/health requested from: ");
    Serial.println(server.client().remoteIP().toString());
    server.send(200, "application/json", json);
}

// Helper to send the raw config.json file from LittleFS via the provided server
static void sendConfigFile(WebServer &srv)
{
    if (!LittleFS.exists("/config.json"))
    {
        srv.send(404, "text/plain", "config.json not found");
        return;
    }
    File f = LittleFS.open("/config.json", "r");
    if (!f)
    {
        srv.send(500, "text/plain", "failed to open config.json");
        return;
    }
    String body;
    while (f.available())
        body += (char)f.read();
    f.close();
    srv.send(200, "application/json", body);
}

// AP-mode wrapper
static void handleDownloadConfigAP()
{
    sendConfigFile(server);
}

// STA-mode wrapper
static void handleDownloadConfigSTA()
{
    sendConfigFile(statusServer);
}

void startStatusServer()
{
    // Start a simple status server on port 80 that always responds when in STA mode.
    statusServer.on("/status", HTTP_GET, []()
                    {
        // Log incoming request on the STA-mode status server
        IPAddress remote = statusServer.client().remoteIP();
        Serial.print("/status (STA) request from: ");
        Serial.println(remote.toString());
        // Note: statusServer is in lambda context; print method/URI and enumerate headers/args
        Serial.println("Method: GET");
        Serial.println("URI: /status");
        int headers = statusServer.headers();
        Serial.print("Headers (count="); Serial.print(headers); Serial.println(")");
        for (int i = 0; i < headers; ++i) {
            String name = statusServer.headerName(i);
            String value = statusServer.header(i);
            Serial.print("  "); Serial.print(name); Serial.print(": "); Serial.println(value);
        }
        int args = statusServer.args();
        Serial.print("Args (count="); Serial.print(args); Serial.println(")");
        for (int i = 0; i < args; ++i) {
            Serial.print("  "); Serial.print(statusServer.argName(i)); Serial.print(" = "); Serial.println(statusServer.arg(i));
        }

        Config current;
        bool has = loadConfig(current);
        String json = "{";
        json += "\"configured\":" + String(has ? "true" : "false") + ",";
        if (has) {
            String pskMasked = current.psk;
            if (pskMasked.length() > 0) {
                for (size_t i = 0; i < pskMasked.length(); ++i) pskMasked.setCharAt(i, '*');
            }
            json += "\"ssid\":\"" + current.ssid + "\",";
            json += "\"psk_masked\":\"" + pskMasked + "\",";
            json += "\"devname\":\"" + current.devname + "\",";
        } else {
            json += "\"ssid\":\"\",";
            json += "\"psk_masked\":\"\",";
            json += "\"devname\":\"\",";
        }
        int status = WiFi.status();
        json += "\"wifi_status\":" + String(status) + ",";
        if (status == WL_CONNECTED) {
            json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
        } else {
            json += "\"ip\":\"\"";
        }
        json += "}";
        statusServer.send(200, "application/json", json); });
    // Expose the config file from STA-mode status server as well
    statusServer.on("/config", HTTP_GET, []()
                    {
        Serial.print("/config (STA) requested from: ");
        Serial.println(statusServer.client().remoteIP().toString());
        // Use the shared helper to send the LittleFS config file
        sendConfigFile(statusServer); });
    statusServer.on("/config.json", HTTP_GET, []()
                    {
        Serial.print("/config.json (STA) requested from: ");
        Serial.println(statusServer.client().remoteIP().toString());
        sendConfigFile(statusServer); });
    // Add /health to STA-mode status server
    statusServer.on("/health", HTTP_GET, []()
                    {
        unsigned long upSec = millis() / 1000UL;
        size_t freeHeap = ESP.getFreeHeap();
        String json = "{";
        json += "\"uptime_s\": " + String(upSec) + ",";
        json += "\"free_heap\": " + String(freeHeap);
        json += "}";
        Serial.print("/health (STA) requested from: ");
        Serial.println(statusServer.client().remoteIP().toString());
        statusServer.send(200, "application/json", json); });
    statusServer.begin();
    statusServerActive = true;
}

void loopStatusServer()
{
    if (!statusServerActive)
        return;
    statusServer.handleClient();
}

void startProvisioning()
{
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS mount failed");
        return;
    }

    String apSSID = "ThermaBridge-" + macSuffixHex();
    Serial.print("Starting AP: ");
    Serial.println(apSSID);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str());
    IPAddress apIP = WiFi.softAPIP();

    dnsServer.start(DNS_PORT, "*", apIP);

    server.onNotFound(handleNotFound);
    server.on("/status", HTTP_GET, handleStatus);
    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/app.js", LittleFS, "/app.js");
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.on("/save", HTTP_POST, handleSave);
    // Expose the config file under both /config and /config.json for compatibility
    server.on("/config", HTTP_GET, handleDownloadConfigAP);      // Route for AP mode
    server.on("/config.json", HTTP_GET, handleDownloadConfigAP); // Alias
    // Also register handlers on the STA-mode statusServer in case we're serving from STA
    statusServer.on("/config", HTTP_GET, handleDownloadConfigSTA);      // Route for STA mode
    statusServer.on("/config.json", HTTP_GET, handleDownloadConfigSTA); // Alias
    server.begin();

    provisioningActive = true;
}

void loopProvisioning()
{
    if (!provisioningActive)
        return;
    dnsServer.processNextRequest();
    server.handleClient();
}

bool isProvisioningActive() { return provisioningActive; }
