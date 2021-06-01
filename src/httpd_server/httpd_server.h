#ifndef HTTPD_SERVER_H
#define HTTPD_SERVER_H

#include "device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Init HTTPD server */

void httpd_server_init(device_t *ndev);

#ifdef __cplusplus
}
#endif

#endif // HTTPD_SERVER_H