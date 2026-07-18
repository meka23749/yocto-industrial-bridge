#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

/*
 * Industrial Protocol Bridge
 * Reads Modbus TCP sensors → publishes via MQTT
 * Monitors thresholds → triggers alerts
 */

/* ===== CONFIGURATION ===== */
#define MAX_SENSORS        4
#define POLL_INTERVAL_SEC  2
#define LOG_FILE           "/var/log/bridge.log"

/* Thresholds */
#define TEMP_ALARM         80.0f
#define PRESSURE_ALARM     8.0f
#define HUMIDITY_ALARM     90.0f

/* ===== DATA STRUCTURES ===== */
typedef enum {
    SENSOR_TEMPERATURE = 0,
    SENSOR_PRESSURE    = 1,
    SENSOR_HUMIDITY    = 2,
    SENSOR_VOLTAGE     = 3
} sensor_type_t;

typedef struct {
    sensor_type_t type;
    uint16_t      modbus_address;
    float         value;
    float         alarm_threshold;
    bool          alarm_active;
    char          name[32];
    char          unit[8];
} sensor_t;

typedef enum {
    BRIDGE_IDLE    = 0,
    BRIDGE_POLLING = 1,
    BRIDGE_ALARM   = 2,
    BRIDGE_ERROR   = 3
} bridge_state_t;

typedef struct {
    bridge_state_t state;
    sensor_t       sensors[MAX_SENSORS];
    uint32_t       poll_count;
    uint32_t       alarm_count;
    uint32_t       mqtt_sent;
    time_t         start_time;
    bool           running;
} bridge_t;

/* ===== GLOBAL ===== */
static bridge_t bridge;

/* ===== SIGNAL HANDLER (clean shutdown) ===== */
void signal_handler(int sig) {
    printf("\n[BRIDGE] Signal %d received, shutting down...\n", sig);
    bridge.running = false;
}

/* ===== TIMESTAMP ===== */
void get_timestamp(char* buf, size_t len) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

/* ===== LOGGING ===== */
void log_message(const char* level, const char* msg) {
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    printf("[%s] [%s] %s\n", timestamp, level, msg);
}

/* ===== SIMULATED MODBUS TCP READ ===== */
/*
 * In production this would be:
 * modbus_t* ctx = modbus_new_tcp("192.168.1.100", 502);
 * modbus_read_registers(ctx, address, 1, &value);
 *
 * We simulate here for QEMU (no real Modbus device)
 */
float modbus_read_sensor(uint16_t address, sensor_type_t type) {
    float base, variation;

    switch (type) {
        case SENSOR_TEMPERATURE:
            base = 45.0f + (address * 5.0f);
            variation = (float)(rand() % 200 - 100) / 10.0f;
            return base + variation;

        case SENSOR_PRESSURE:
            base = 5.0f;
            variation = (float)(rand() % 40 - 20) / 10.0f;
            return base + variation;

        case SENSOR_HUMIDITY:
            base = 60.0f;
            variation = (float)(rand() % 300 - 150) / 10.0f;
            return base + variation;

        case SENSOR_VOLTAGE:
            base = 24.0f;
            variation = (float)(rand() % 20 - 10) / 10.0f;
            return base + variation;

        default:
            return 0.0f;
    }
}

/* ===== SIMULATED MQTT PUBLISH ===== */
/*
 * In production this would be:
 * mosquitto_publish(mosq, NULL, topic, len, payload, 0, false);
 */
void mqtt_publish(const char* topic, const char* payload) {
    printf("[MQTT] topic=%s | %s\n", topic, payload);
    bridge.mqtt_sent++;
}

/* ===== BRIDGE FUNCTIONS ===== */
void bridge_init(bridge_t* b) {
    memset(b, 0, sizeof(bridge_t));
    b->state = BRIDGE_IDLE;
    b->running = true;
    b->start_time = time(NULL);

    /* Configure sensors */
    b->sensors[0] = (sensor_t){
        SENSOR_TEMPERATURE, 100, 0, TEMP_ALARM, false,
        "Motortemperatur", "C"
    };
    b->sensors[1] = (sensor_t){
        SENSOR_PRESSURE, 200, 0, PRESSURE_ALARM, false,
        "Hydraulikdruck", "bar"
    };
    b->sensors[2] = (sensor_t){
        SENSOR_HUMIDITY, 300, 0, HUMIDITY_ALARM, false,
        "Luftfeuchtigkeit", "%"
    };
    b->sensors[3] = (sensor_t){
        SENSOR_VOLTAGE, 400, 0, 28.0f, false,
        "Versorgungsspannung", "V"
    };

    log_message("INFO", "Bridge initialized with 4 sensors");
}

void bridge_poll_sensors(bridge_t* b) {
    b->state = BRIDGE_POLLING;
    bool any_alarm = false;
    char msg[256];
    char topic[128];
    char payload[256];

    for (int i = 0; i < MAX_SENSORS; i++) {
        sensor_t* s = &b->sensors[i];

        /* Read from Modbus */
        s->value = modbus_read_sensor(s->modbus_address, s->type);

        /* Check threshold */
        bool was_alarm = s->alarm_active;
        s->alarm_active = (s->value > s->alarm_threshold);

        if (s->alarm_active) {
            any_alarm = true;
            if (!was_alarm) {
                snprintf(msg, sizeof(msg),
                         "ALARM: %s = %.1f %s (threshold: %.1f)",
                         s->name, s->value, s->unit,
                         s->alarm_threshold);
                log_message("ALARM", msg);
                b->alarm_count++;
            }
        } else if (was_alarm) {
            snprintf(msg, sizeof(msg),
                     "CLEARED: %s = %.1f %s (back to normal)",
                     s->name, s->value, s->unit);
            log_message("INFO", msg);
        }

        /* Publish to MQTT */
        snprintf(topic, sizeof(topic),
                 "factory/line1/sensor/%s", s->name);
        snprintf(payload, sizeof(payload),
                 "{\"value\":%.1f,\"unit\":\"%s\",\"alarm\":%s}",
                 s->value, s->unit,
                 s->alarm_active ? "true" : "false");
        mqtt_publish(topic, payload);
    }

    b->state = any_alarm ? BRIDGE_ALARM : BRIDGE_POLLING;
    b->poll_count++;
}

void bridge_print_status(bridge_t* b) {
    char msg[256];
    time_t uptime = time(NULL) - b->start_time;

    printf("\n--- Bridge Status ---\n");
    snprintf(msg, sizeof(msg),
             "State: %s | Polls: %u | Alarms: %u | "
             "MQTT sent: %u | Uptime: %lds",
             b->state == BRIDGE_ALARM ? "ALARM" :
             b->state == BRIDGE_POLLING ? "RUNNING" : "IDLE",
             b->poll_count, b->alarm_count,
             b->mqtt_sent, (long)uptime);
    log_message("STATUS", msg);

    for (int i = 0; i < MAX_SENSORS; i++) {
        sensor_t* s = &b->sensors[i];
        printf("  %-20s = %7.1f %-4s [max: %.1f] %s\n",
               s->name, s->value, s->unit,
               s->alarm_threshold,
               s->alarm_active ? "!! ALARM !!" : "OK");
    }
    printf("---------------------\n\n");
}

/* ===== MAIN ===== */
int main(int argc, char* argv[]) {
    int max_cycles = 5;  /* default: 5 poll cycles */

    if (argc > 1) {
        max_cycles = atoi(argv[1]);
    }

    printf("========================================\n");
    printf("  Industrial Protocol Bridge v1.0\n");
    printf("  Modbus TCP -> MQTT\n");
    printf("  Cycles: %d\n", max_cycles);
    printf("========================================\n\n");

    /* Register signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize */
    srand(time(NULL));
    bridge_init(&bridge);

    /* Main loop */
    int cycle = 0;
    while (bridge.running && cycle < max_cycles) {
        bridge_poll_sensors(&bridge);
        bridge_print_status(&bridge);
        cycle++;

        if (cycle < max_cycles && bridge.running) {
            sleep(POLL_INTERVAL_SEC);
        }
    }

    /* Final report */
    printf("\n========================================\n");
    printf("  Bridge Shutdown Report\n");
    printf("  Total polls:  %u\n", bridge.poll_count);
    printf("  Total alarms: %u\n", bridge.alarm_count);
    printf("  MQTT messages: %u\n", bridge.mqtt_sent);
    printf("========================================\n");

    return 0;
}
