#include "tool_get_time.h"
#include "mimi_config.h"
#include "proxy/http_proxy.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "tool_time";

static const char *MONTHS[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

/* Parse "Sat, 01 Feb 2025 10:25:00 GMT" → set system clock, return formatted string */
static bool parse_and_set_time(const char *date_str, char *out, size_t out_size)
{
    int day, year, hour, min, sec;
    char mon_str[4] = {0};

    if (sscanf(date_str, "%*[^,], %d %3s %d %d:%d:%d",
               &day, mon_str, &year, &hour, &min, &sec) != 6) {
        return false;
    }

    int mon = -1;
    for (int i = 0; i < 12; i++) {
        if (strcmp(mon_str, MONTHS[i]) == 0) { mon = i; break; }
    }
    if (mon < 0) return false;

    struct tm tm = {
        .tm_sec = sec, .tm_min = min, .tm_hour = hour,
        .tm_mday = day, .tm_mon = mon, .tm_year = year - 1900,
    };

    /* Convert UTC to epoch — mktime expects local, so temporarily set UTC */
    setenv("TZ", "UTC0", 1);
    tzset();
    time_t t = mktime(&tm);

    /* Restore timezone */
    setenv("TZ", MIMI_TIMEZONE, 1);
    tzset();

    if (t < 0) return false;

    struct timeval tv = { .tv_sec = t };
    settimeofday(&tv, NULL);

    /* Format in local time */
    struct tm local;
    localtime_r(&t, &local);
    strftime(out, out_size, "%Y-%m-%d %H:%M:%S %Z (%A)", &local);

    return true;
}

typedef struct {
    char date_val[64];
} time_header_ctx_t;

static esp_err_t time_http_event_handler(esp_http_client_event_t *evt)
{
    time_header_ctx_t *ctx = evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_HEADER) {
        if (strcasecmp(evt->header_key, "Date") == 0 && ctx) {
            strncpy(ctx->date_val, evt->header_value, sizeof(ctx->date_val) - 1);
            ctx->date_val[sizeof(ctx->date_val) - 1] = '\0';
        }
    }
    return ESP_OK;
}

static esp_err_t fetch_time_baidu(char *out, size_t out_size)
{
    time_header_ctx_t ctx = {0};

    esp_http_client_config_t config = {
        .url = "http://www.baidu.com/",
        .method = HTTP_METHOD_HEAD,
        .timeout_ms = 5000,
        .buffer_size = 1024,
        .buffer_size_tx = 512,
        .event_handler = time_http_event_handler,
        .user_data = &ctx,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_FAIL;

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) return err;
    if (ctx.date_val[0] == '\0') return ESP_ERR_NOT_FOUND;

    if (!parse_and_set_time(ctx.date_val, out, out_size)) return ESP_FAIL;
    return ESP_OK;
}

esp_err_t tool_get_time_execute(const char *input_json, char *output, size_t output_size)
{
    ESP_LOGI(TAG, "Fetching current time...");

    esp_err_t err = fetch_time_baidu(output, output_size);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Time: %s", output);
    } else {
        snprintf(output, output_size, "Error: failed to fetch time (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", output);
    }

    return err;
}
