#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/logging/log.h>
#include "wifi_manager.h"
#include "mqtt_manager.h"

LOG_MODULE_REGISTER(wifi_manager, LOG_LEVEL_INF);

#define WIFI_SSID "simi"
#define WIFI_PSK "12345679"

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;
static struct k_work_delayable wifi_reconnect_work;

static bool mqtt_started = false;

static void connect_to_wifi(void) {
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params wifi_params = {
        .ssid = WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = WIFI_PSK,
        .psk_length = strlen(WIFI_PSK),
        .security = WIFI_SECURITY_TYPE_PSK,
        .channel = WIFI_CHANNEL_ANY
    };

    LOG_INF("Cautam reteaua Wi-Fi...");
    
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(wifi_params));
    
    if (ret < 0) {
        LOG_ERR("Reteaua nu a fost gasita fizic (eroare %d). Reincercam in 5s...", ret);
        k_work_reschedule(&wifi_reconnect_work, K_SECONDS(5));
    }
}

static void wifi_reconnect_fn(struct k_work *work) {
    connect_to_wifi();
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event, struct net_if *iface) {
    
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        
        if (status->status) {
            LOG_ERR("Conectare Wi-Fi respinsa (status %d)! Reincercam in 5s...", status->status);
            k_work_reschedule(&wifi_reconnect_work, K_SECONDS(5));
        } else {
            LOG_INF("Conectat la Wi-Fi cu succes!");
        }
    } 
    else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        LOG_WRN("Conexiune Wi-Fi pierduta brusc! Cautam reteaua in fundal...");
        is_mqtt_connected = false; 
        k_work_reschedule(&wifi_reconnect_work, K_SECONDS(5));
    }
    else if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        LOG_INF("IP obtinut prin DHCP!");
        
        if (!mqtt_started) {
            mqtt_started = true;
            init_mqtt(); 
        } else {
            mqtt_reconnect();
        }
        
    }
}

void init_wifi(void) {
    k_work_init_delayable(&wifi_reconnect_work, wifi_reconnect_fn);

    net_mgmt_init_event_callback(&wifi_cb, net_mgmt_event_handler, 
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);
    
    net_mgmt_init_event_callback(&ipv4_cb, net_mgmt_event_handler, NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&ipv4_cb);

    connect_to_wifi();
}

void wifi_force_reconnect(void) {
    LOG_WRN("Forțăm restartarea modulului Wi-Fi...");
    struct net_if *iface = net_if_get_default();
    
    net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
    
    k_work_reschedule(&wifi_reconnect_work, K_SECONDS(2));
}