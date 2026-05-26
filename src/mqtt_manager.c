#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "mqtt_manager.h"
#include "emergency.h"
#include "telemetry.h"
#include "hazard.h"
#include "wifi_manager.h"

bool is_mqtt_connected = false;
static bool mqtt_needs_reconnect = true; 

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
                LOG_INF("Conectat la Broker cu succes!");
                is_mqtt_connected = true;
                
                struct mqtt_topic topic = { .topic.utf8 = (uint8_t *)TOPIC_COMMANDS, .topic.size = strlen(TOPIC_COMMANDS) };
                struct mqtt_subscription_list sub_list = { .list = &topic, .list_count = 1, .message_id = 1 };
                mqtt_subscribe(client, &sub_list);
                
                publish(client, TOPIC_STATUS, "Intersectie: Online");
            } else {
                LOG_ERR("Conexiune refuzata de broker (cod: %d)", evt->result);
                mqtt_needs_reconnect = true;
            }
            break;
            
        case MQTT_EVT_DISCONNECT:
            LOG_ERR("Deconectat de la broker!");
            is_mqtt_connected = false;
            mqtt_needs_reconnect = true;
            break;

        case MQTT_EVT_PUBLISH: {
            const struct mqtt_publish_param *pub = &evt->param.publish;
            char payload[32] = {0}; 
            int len = pub->message.payload.len;

            if (len < sizeof(payload)) {
                mqtt_read_publish_payload(client, payload, len);
                
                if (pub->retain_flag == 1) {
                    LOG_WRN("Comandă fantomă ștearsă (Mesaj vechi ignorat): %s", payload);
                    break; 
                }

                LOG_INF("Comanda LIVE primita: %s", payload);

                if (strncmp(payload, "URGENTA_NS", 10) == 0) {
                    trigger_emergency(EMERG_NS);
                } 
                else if (strncmp(payload, "URGENTA_EW", 10) == 0) {
                    trigger_emergency(EMERG_EW);
                }
                else if (strncmp(payload, "HAZARD", 6) == 0) {
                    hazard_mode_active = !hazard_mode_active;
                    if (hazard_mode_active) {
                        clear_pedestrian_requests();
                    } else {
                        force_auto_red(); 
                        restart_car_traffic_thread();
                    }
                }
            } else {
                mqtt_read_publish_payload(client, payload, 0); 
            }
            break;
        }
        default:
            break;
    }
}

#define MQTT_STACK_SIZE 4096
#define MQTT_PRIORITY 7
K_THREAD_STACK_DEFINE(mqtt_stack_area, MQTT_STACK_SIZE);
struct k_thread mqtt_thread_data;

static void mqtt_thread_fn(void *p1, void *p2, void *p3) {
    int rc;
    struct zsock_addrinfo *res;
    struct zsock_addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    while (1) {
        /* Blocul de conectare/reconectare care se executa DOAR cand e nevoie */
        if (mqtt_needs_reconnect) {
            LOG_WRN("Se curata memoria si se pregateste o noua conexiune MQTT...");
            mqtt_needs_reconnect = false; /* Oprim repetarea abuziva */
            is_mqtt_connected = false;

            /* 1. Distrugem vechiul socket si stergem toata structura din memorie */
            mqtt_abort(&client_ctx);
            k_msleep(500);
            memset(&client_ctx, 0, sizeof(client_ctx)); 

            /* 2. Cautam IP-ul */
            rc = zsock_getaddrinfo(BROKER_ADDR, "1883", &hints, &res);
            if (rc != 0) {
                LOG_ERR("Eroare DNS. Reincercam in 5 secunde...");
                wifi_force_reconnect();
                mqtt_needs_reconnect = true;
                k_msleep(5000);
                continue; 
            }

            broker.sin_family = AF_INET;
            broker.sin_port = htons(BROKER_PORT);
            broker.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
            zsock_freeaddrinfo(res);

            /* 3. Re-initializam curat */
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

            /* 4. Lansam conectarea si o lasam in pace sa astepte CONNACK */
            rc = mqtt_connect(&client_ctx);
            if (rc != 0) {
                LOG_ERR("mqtt_connect a esuat: %d. Reincercam in 5s...", rc);
                wifi_force_reconnect();
                mqtt_needs_reconnect = true;
                k_msleep(5000);
                continue;
            }
        }

        /* Daca nu avem un socket valid inca, asteptam */
        if (client_ctx.transport.tcp.sock < 0) {
            k_msleep(100);
            continue;
        }

        /* Gestionam comunicarea retelei pe socket-ul activ */
        fds[0].fd = client_ctx.transport.tcp.sock;
        fds[0].events = ZSOCK_POLLIN;

        rc = zsock_poll(fds, 1, 500);
        if (rc > 0) {
            if (fds[0].revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP)) {
                LOG_ERR("Socket inchis / Eroare TCP!");
                mqtt_needs_reconnect = true;
                continue;
            }
            
            int input_rc = mqtt_input(&client_ctx);
            if (input_rc < 0) {
                LOG_ERR("Eroare la procesare mesaje: %d", input_rc);
                mqtt_needs_reconnect = true;
                continue;
            }
        } else if (rc < 0) {
            LOG_ERR("Eroare zsock_poll: %d", rc);
            mqtt_needs_reconnect = true;
            continue;
        }

        /* Daca suntem confirmati, tinem conexiunea treaza */
        if (is_mqtt_connected) {
            int live_rc = mqtt_live(&client_ctx);
            if (live_rc < 0 && live_rc != -EAGAIN) {
                LOG_ERR("Conexiune moarta (Ping esuat)!");
                mqtt_needs_reconnect = true;
                continue;
            }
        }

        k_msleep(100);
    }
}

void init_mqtt(void) {
    k_thread_create(&mqtt_thread_data, mqtt_stack_area,
                    K_THREAD_STACK_SIZEOF(mqtt_stack_area),
                    mqtt_thread_fn, NULL, NULL, NULL,
                    MQTT_PRIORITY, 0, K_NO_WAIT);
}

int mqtt_publish_to_topic(const char *topic, const char *payload) {
    if (!is_mqtt_connected) return -ENOTCONN;
    return publish(&client_ctx, topic, payload);
}

void mqtt_reconnect(void) {
    LOG_WRN("Semnal de la Wi-Fi primit. Declanșăm procedura de reconectare MQTT!");
    mqtt_needs_reconnect = true;
}