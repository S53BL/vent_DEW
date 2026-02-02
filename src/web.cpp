// web.cpp - Web server setup for vent_DEW
#include "web.h"
#include "html.h"
#include "web_handlers.h"
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

// Setup web server endpoints
void setupWebEndpoints() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/sd-list", HTTP_GET, handleSDList);
}
