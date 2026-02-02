// web_handlers.h - Web request handlers header for vent_DEW
#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <ESPAsyncWebServer.h>

// Handler function declarations
void handleRoot(AsyncWebServerRequest *request);
void handleSDList(AsyncWebServerRequest *request);

#endif // WEB_HANDLERS_H
