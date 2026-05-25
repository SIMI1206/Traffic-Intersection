#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/logging/log.h>
#include "wifi_manager.h"
#include "mqtt_manager.h" /* Integrat aici */

LOG_MODULE_REGISTER(wifi_manager, LOG_LEVEL_INF);

#define WIFI_SSID "simi"
#define WIFI_PSK "12345679"

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event, struct net_if *iface) {
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        LOG_INF("Conectat la Wi-Fi!");
    } 
    else if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        LOG_INF("IP obtinut prin DHCP!");
        init_mqtt(); /* Pornim MQTT imediat ce avem IP */
    }
}

void init_wifi(void) {
    struct net_if *iface = net_if_get_default();

    /* Inregistram callback-urile de retea */
    net_mgmt_init_event_callback(&wifi_cb, net_mgmt_event_handler, NET_EVENT_WIFI_CONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_init_event_callback(&ipv4_cb, net_mgmt_event_handler, NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&ipv4_cb);

    /* Parametrii Wi-Fi */
    struct wifi_connect_req_params wifi_params = {
        .ssid = WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = WIFI_PSK,
        .psk_length = strlen(WIFI_PSK),
        .security = WIFI_SECURITY_TYPE_PSK,
        .channel = WIFI_CHANNEL_ANY
    };

    LOG_INF("Pornire Wi-Fi...");
    net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(wifi_params));
}