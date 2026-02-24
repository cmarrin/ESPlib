/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2026, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "HTTPClient.h"

#if defined ARDUINO
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif
#elif defined ESP_PLATFORM
#include "esp_http_client.h"
#else
#include <curl/curl.h>
#endif

using namespace mil;

#if defined ARDUINO
bool
HTTPClient::fetch(const char* url)
{
    bool success = true;
    
    WiFiClient client;
	HTTPClient http;

	http.begin(client, url);
	int httpCode = http.GET();

	if (httpCode > 0) {
		printf("    got response: %d\n", int32_t(httpCode));

		if (httpCode == HTTP_CODE_OK) {
			std::string payload = http.getString().c_str();
			printf("Got payload, parsing...\n");
            _handler(payload.c_str(), payload.length());
		} else {
            printf("[HTTP] GET code not ok\n");
            success = false;
        }
	} else {
		printf("[HTTP] GET... failed, error: %s (%d)\n", http.errorToString(httpCode), int32_t(httpCode));
		success = false;
	}

	http.end();
    return success;
}
#elif defined ESP_PLATFORM

#include "esp_log.h"

static const char* TAG = "HTTPClient";

static esp_err_t eventHandler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            reinterpret_cast<HTTPClient*>(evt->user_data)->_handler(reinterpret_cast<const char*>(evt->data), uint32_t(evt->data_len));
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT: // Added in later ESP-IDF versions
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_redirection(evt->client); // Follow the redirect
            break;
    }
    return ESP_OK;
}

bool
HTTPClient::fetch(const char* url)
{
    esp_http_client_config_t config = { };
    config.url = url;
    config.event_handler = eventHandler;
    config.user_data = this;

    ESP_LOGI(TAG, "HTTP request with url '%s'", url);

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", "ESP32 HTTP Client");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        return true;
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        return false;
    }
}
#else
// HTTP callback for Mac
static size_t httpCB(char* p, size_t size, size_t nmemb, void* data)
{
    size *= nmemb;
    reinterpret_cast<HTTPClient*>(data)->_handler(p, uint32_t(size));
    return size;
}

bool
HTTPClient::fetch(const char* url)
{
    bool success = true;

    CURL* curl = curl_easy_init();
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
 
        // Use HTTP/3 but fallback to earlier HTTP if necessary
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long) CURL_HTTP_VERSION_3);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpCB);
         
        // Perform the request, res gets the return code
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK) {
            printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        
        // always cleanup
        curl_easy_cleanup(curl);
    }
    return success;
}
#endif
