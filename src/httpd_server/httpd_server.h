/** @file   httpd_server.h
 *  @brief  httpd service initialization header.
 *
 *  It is specifically requiried for sharing the device state and control struct
 *  with the httpd monitor handler.
 *
 *  @author Vladislav Rykov
 *  @bug    No known bugs.
*/

#ifndef HTTPD_SERVER_H
#define HTTPD_SERVER_H

#include "device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initilizes the httpd server. 
 * 
 * @details Sets up the httpd server and links endpoints with handlers.
 *          
 * @param[in] ndev Pointer to the device struct.
*/
void httpd_server_init(device_t *ndev);

#ifdef __cplusplus
}
#endif

#endif // HTTPD_SERVER_H