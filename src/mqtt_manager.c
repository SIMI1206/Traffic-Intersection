#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>

#include "mqtt_manager.h"
#include "emergency.h"
#include "telemetry.h"
#include "hazard.h"

bool is_mqtt_connected = false;

LOG_MODULE_REGISTER(mqtt_manager, LOG_LEVEL_INF);

#define BROKER_ADDR "broker.hivemq.com"
#define BROKER_PORT 1883
#define MQTT_CLIENTID "pico_simi"
#define TOPIC_STATUS "simi/intersectie/status"
#define TOPIC_COMMANDS "simi/intersectie/comenzi"

static uint8_t rx_buffer[128];
static uint8_t tx_buffer[128];
static struct mqtt_client client_ctx;
static struct sockaddr_in broker;
static struct zsock_pollfd fds[1];

/* Funcție pentru a trimite mesaje la broker */
static int publish(struct mqtt_client *client, const char *topic, const char *payload) {
    struct mqtt_publish_param param;
    param.message.topic.topic.utf8 = (uint8_t *)topic;
    param.message.topic.topic.size = strlen(topic);
    param.message.payload.data = (uint8_t *)payload;
    param.message.payload.len = strlen(payload);
    param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
    param.message_id = 1;
    return mqtt_publish(client, &param);
}

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt) {
    switch (evt->type) {
        case MQTT_EVT_CONNACK:
            if (evt->result == 0) {
                LOG_INF("Conectat la Broker!");
                is_mqtt_connected = true;
                /* 1. Abonare la comenzi */
                struct mqtt_topic topic = { .topic.utf8 = (uint8_t *)TOPIC_COMMANDS, .topic.size = strlen(TOPIC_COMMANDS) };
                struct mqtt_subscription_list sub_list = { .list = &topic, .list_count = 1, .message_id = 1 };
                mqtt_subscribe(client, &sub_list);
                /* 2. Anuntam ca suntem online */
                publish(client, TOPIC_STATUS, "Intersectie: Online");
            }
            break;
        case MQTT_EVT_PUBLISH: {
            const struct mqtt_publish_param *pub = &evt->param.publish;
            char payload[32] = {0}; 
            int len = pub->message.payload.len;

            /* Ne asiguram ca textul primit nu e prea lung */
            if (len < sizeof(payload)) {
                mqtt_read_publish_payload(client, payload, len);
                LOG_INF("Comanda primita: %s", payload);

                /* Comparam textul si trimitem directia catre emergency.c */
                if (strncmp(payload, "URGENTA_NS", 10) == 0) {
                    LOG_INF(">>> ACTIVARE VERDE NORD-SUD! <<<");
                    trigger_emergency(EMERG_NS);
                } 
                else if (strncmp(payload, "URGENTA_EW", 10) == 0) {
                    LOG_INF(">>> ACTIVARE VERDE EST-VEST! <<<");
                    trigger_emergency(EMERG_EW);
                }
                if (strncmp(payload, "URGENTA_NS", 10) == 0) {
                    LOG_INF(">>> ACTIVARE VERDE NORD-SUD! <<<");
                    trigger_emergency(EMERG_NS);
                } 
                else if (strncmp(payload, "URGENTA_EW", 10) == 0) {
                    LOG_INF(">>> ACTIVARE VERDE EST-VEST! <<<");
                    trigger_emergency(EMERG_EW);
                }
                /* ADAUGA ACEST NOU BLOC PENTRU HAZARD: */
                else if (strncmp(payload, "HAZARD", 6) == 0) {
                    hazard_mode_active = !hazard_mode_active;
                    LOG_INF("Mod Hazard: %s", hazard_mode_active ? "ACTIVAT" : "DEZACTIVAT");

                    if (!hazard_mode_active) {
                        /* Daca am dezactivat Hazard, forțăm resetarea traficului */
                        force_auto_red(); // Punem pe rosu sa plece de la zero
                        restart_car_traffic_thread(); // Restartam thread-ul sa reia ciclul
                    }
                }
            } else {
                mqtt_read_publish_payload(client, payload, 0); 
            }
            break;
        
        }
    }
}

/* Configurarea Firului de Executie (Thread) pentru MQTT */
#define MQTT_STACK_SIZE 4096
#define MQTT_PRIORITY 7
K_THREAD_STACK_DEFINE(mqtt_stack_area, MQTT_STACK_SIZE);
struct k_thread mqtt_thread_data;

/* Functia principala a Thread-ului MQTT */
static void mqtt_thread_fn(void *p1, void *p2, void *p3) {
    int rc;
    struct zsock_addrinfo *res;
    struct zsock_addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    LOG_INF("Cautam IP-ul serverului %s...", BROKER_ADDR);
    rc = zsock_getaddrinfo(BROKER_ADDR, "1883", &hints, &res);
    if (rc != 0) {
        LOG_ERR("Eroare DNS (Nu pot gasi serverul): %d", rc);
        return;
    }

    broker.sin_family = AF_INET;
    broker.sin_port = htons(BROKER_PORT);
    broker.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    zsock_freeaddrinfo(res);

    /* Initializare Structura Client MQTT */
    mqtt_client_init(&client_ctx);
    client_ctx.broker = &broker;
    client_ctx.evt_cb = mqtt_evt_handler;
    client_ctx.client_id.utf8 = (uint8_t *)MQTT_CLIENTID;
    client_ctx.client_id.size = strlen(MQTT_CLIENTID);
    client_ctx.password = NULL;
    client_ctx.user_name = NULL;
    client_ctx.protocol_version = MQTT_VERSION_3_1_1;
    client_ctx.rx_buf = rx_buffer;
    client_ctx.rx_buf_size = sizeof(rx_buffer);
    client_ctx.tx_buf = tx_buffer;
    client_ctx.tx_buf_size = sizeof(tx_buffer);

    LOG_INF("Incercam sa ne logam la serverul MQTT...");
    rc = mqtt_connect(&client_ctx);
    if (rc != 0) {
        LOG_ERR("Eroare conectare: %d", rc);
        return;
    }

    fds[0].fd = client_ctx.transport.tcp.sock;
    fds[0].events = ZSOCK_POLLIN;

    /* Bucla infinita care asculta mesaje si tine conexiunea treaza (Ping) */
    while (1) {
        rc = zsock_poll(fds, 1, 1000);
        if (rc > 0) {
            mqtt_input(&client_ctx);
        }
        mqtt_live(&client_ctx);
        k_msleep(100);
    }
}

void init_mqtt(void) {
    /* Cream si pornim Thread-ul MQTT in fundal */
    k_thread_create(&mqtt_thread_data, mqtt_stack_area,
                    K_THREAD_STACK_SIZEOF(mqtt_stack_area),
                    mqtt_thread_fn, NULL, NULL, NULL,
                    MQTT_PRIORITY, 0, K_NO_WAIT);
}

int mqtt_publish_to_topic(const char *topic, const char *payload) {
    /* Apelam functia interna de publish folosind contextul global client_ctx */
    return publish(&client_ctx, topic, payload);
}