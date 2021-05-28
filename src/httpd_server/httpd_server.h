#ifndef HTTPD_SERVER_H
#define HTTPD_SERVER_H

#include "counter.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Init HTTPD server */
void httpd_server_init(counter_status_t *n_cs, volatile uint8_t *fb_free);

#ifdef __cplusplus
}
#endif

#endif // HTTPD_SERVER_H