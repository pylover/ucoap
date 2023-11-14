/**
 * ucoap_tcp.h
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#ifndef _UCOAP_UCOAP_TCP_H_
#define _UCOAP_UCOAP_TCP_H_


#include "ucoap.h"


/**
 *  TCP CoAP header
 *
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Len=15 |  TKL  | Extended Length (32 bits)
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                  |    Code       |  Token (if any, TKL bytes) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |   Options (if any) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |1 1 1 1 1 1 1 1|    Payload (if any) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */


/**
 * @brief Send a CoAP packet over TCP. Do not use it directly.
 *
 * @param handle - coap handle
 * @param reqd - descriptor of request
 *
 * @return status of operation
 */
ucoap_error ucoap_send_coap_request_tcp(ucoap_handle * const handle, const ucoap_request_descriptor * const reqd);


#endif /* _UCOAP_UCOAP_TCP_H_ */
