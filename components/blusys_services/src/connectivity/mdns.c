#include "blusys/connectivity/mdns.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED && defined(BLUSYS_HAS_MDNS)

#include "mdns.h"
#include "esp_netif.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

struct blusys_mdns {
    blusys_lock_t lock;
};

static const char *proto_to_str(blusys_mdns_proto_t proto)
{
    return (proto == BLUSYS_MDNS_PROTO_TCP) ? "_tcp" : "_udp";
}

blusys_err_t blusys_mdns_open(const blusys_mdns_config_t *config, blusys_mdns_t **out_mdns)
{
    if (config == NULL || out_mdns == NULL || config->hostname == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_mdns_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    esp_err_t esp_err = mdns_init();
    if (esp_err != ESP_OK) {
        blusys_lock_deinit(&h->lock);
        free(h);
        return blusys_translate_esp_err(esp_err);
    }

    esp_err = mdns_hostname_set(config->hostname);
    if (esp_err != ESP_OK) {
        mdns_free();
        blusys_lock_deinit(&h->lock);
        free(h);
        return blusys_translate_esp_err(esp_err);
    }

    if (config->instance_name) {
        esp_err = mdns_instance_name_set(config->instance_name);
        if (esp_err != ESP_OK) {
            mdns_free();
            blusys_lock_deinit(&h->lock);
            free(h);
            return blusys_translate_esp_err(esp_err);
        }
    }

    *out_mdns = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_mdns_close(blusys_mdns_t *mdns)
{
    if (mdns == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    mdns_free();
    blusys_lock_deinit(&mdns->lock);
    free(mdns);
    return BLUSYS_OK;
}

blusys_err_t blusys_mdns_add_service(blusys_mdns_t *mdns,
                                      const char *instance_name,
                                      const char *service_type,
                                      blusys_mdns_proto_t proto,
                                      uint16_t port,
                                      const blusys_mdns_txt_item_t *txt,
                                      size_t txt_count)
{
    if (mdns == NULL || service_type == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&mdns->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    mdns_txt_item_t *esp_txt = NULL;
    if (txt && txt_count > 0) {
        esp_txt = calloc(txt_count, sizeof(mdns_txt_item_t));
        if (esp_txt == NULL) {
            blusys_lock_give(&mdns->lock);
            return BLUSYS_ERR_NO_MEM;
        }
        for (size_t i = 0; i < txt_count; i++) {
            esp_txt[i].key   = txt[i].key;
            esp_txt[i].value = txt[i].value;
        }
    }

    esp_err_t esp_err = mdns_service_add(instance_name, service_type,
                                          proto_to_str(proto), port,
                                          esp_txt, txt_count);
    free(esp_txt);
    blusys_lock_give(&mdns->lock);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_mdns_remove_service(blusys_mdns_t *mdns,
                                         const char *service_type,
                                         blusys_mdns_proto_t proto)
{
    if (mdns == NULL || service_type == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&mdns->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = mdns_service_remove(service_type, proto_to_str(proto));
    blusys_lock_give(&mdns->lock);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_mdns_query(blusys_mdns_t *mdns,
                                const char *service_type,
                                blusys_mdns_proto_t proto,
                                int timeout_ms,
                                blusys_mdns_result_t *results,
                                size_t max_results,
                                size_t *out_count)
{
    if (mdns == NULL || service_type == NULL || results == NULL ||
        max_results == 0 || out_count == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    int query_timeout = (timeout_ms < 0) ? 5000 : timeout_ms;

    /* mdns_query_ptr() blocks for up to query_timeout ms.  Do NOT hold the
       blusys_lock across this call — it would stall add_service / remove_service
       for the entire query duration.  The ESP-IDF mDNS component is internally
       thread-safe, so concurrent add/query/remove is fine without our lock. */
    mdns_result_t *esp_results = NULL;
    esp_err_t esp_err = mdns_query_ptr(service_type, proto_to_str(proto),
                                        query_timeout, (int)max_results,
                                        &esp_results);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    size_t count = 0;
    mdns_result_t *r = esp_results;
    while (r != NULL && count < max_results) {
        memset(&results[count], 0, sizeof(results[count]));

        if (r->instance_name) {
            strncpy(results[count].instance_name, r->instance_name,
                    sizeof(results[count].instance_name) - 1);
        }
        if (r->hostname) {
            strncpy(results[count].hostname, r->hostname,
                    sizeof(results[count].hostname) - 1);
        }
        results[count].port = r->port;

        /* Copy first IPv4 address */
        mdns_ip_addr_t *addr = r->addr;
        while (addr) {
            if (addr->addr.type == ESP_IPADDR_TYPE_V4) {
                snprintf(results[count].ip, sizeof(results[count].ip),
                         IPSTR, IP2STR(&addr->addr.u_addr.ip4));
                break;
            }
            addr = addr->next;
        }

        count++;
        r = r->next;
    }

    mdns_query_results_free(esp_results);

    *out_count = count;
    return BLUSYS_OK;
}

#else /* !SOC_WIFI_SUPPORTED || !BLUSYS_HAS_MDNS */

blusys_err_t blusys_mdns_open(const blusys_mdns_config_t *config, blusys_mdns_t **out_mdns)
{
    (void)config; (void)out_mdns;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mdns_close(blusys_mdns_t *mdns)
{
    (void)mdns;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mdns_add_service(blusys_mdns_t *mdns,
                                      const char *instance_name,
                                      const char *service_type,
                                      blusys_mdns_proto_t proto,
                                      uint16_t port,
                                      const blusys_mdns_txt_item_t *txt,
                                      size_t txt_count)
{
    (void)mdns; (void)instance_name; (void)service_type;
    (void)proto; (void)port; (void)txt; (void)txt_count;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mdns_remove_service(blusys_mdns_t *mdns,
                                         const char *service_type,
                                         blusys_mdns_proto_t proto)
{
    (void)mdns; (void)service_type; (void)proto;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mdns_query(blusys_mdns_t *mdns,
                                const char *service_type,
                                blusys_mdns_proto_t proto,
                                int timeout_ms,
                                blusys_mdns_result_t *results,
                                size_t max_results,
                                size_t *out_count)
{
    (void)mdns; (void)service_type; (void)proto;
    (void)timeout_ms; (void)results; (void)max_results; (void)out_count;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
