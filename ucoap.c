/**
 * ucoap.c
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#include "ucoap.h"

#include "ucoap_udp.h"
#include "ucoap_tcp.h"
#include "ucoap_utils.h"



static ucoap_error init_coap_driver(ucoap_handle * const handle,
        const ucoap_request_descriptor * const reqd);
static void deinit_coap_driver(ucoap_handle * handle);



/**
 * @brief See description in the header file.
 *
 */
void ucoap_debug(ucoap_handle * const handle, const bool enable)
{
    if (enable) {
        UCOAP_SET_STATUS(handle, UCOAP_DEBUG_ON);
    } else {
        UCOAP_RESET_STATUS(handle, UCOAP_DEBUG_ON);
    }
}


/**
 * @brief See description in the header file.
 *
 */
ucoap_error ucoap_send_coap_request(ucoap_handle * const handle,
        const ucoap_request_descriptor * const reqd) {
    ucoap_error err;

    if (UCOAP_CHECK_STATUS(handle, UCOAP_SENDING_PACKET)) {
        return UCOAP_BUSY_ERROR;
    }

    UCOAP_SET_STATUS(handle, UCOAP_SENDING_PACKET);
    err = init_coap_driver(handle, reqd);

    if (err == UCOAP_OK) {

        switch (handle->transport) {
            case UCOAP_UDP:
                err = ucoap_send_coap_request_udp(handle, reqd);
                break;

            case UCOAP_TCP:
                err = ucoap_send_coap_request_tcp(handle, reqd);
                break;

            case UCOAP_SMS:
            default:
                /* not supported yet */
                err = UCOAP_PARAM_ERROR;
                break;
        }
    }

    deinit_coap_driver(handle);

    UCOAP_RESET_STATUS(handle, UCOAP_SENDING_PACKET);
    ucoap_tx_signal(handle, UCOAP_ROUTINE_PACKET_DID_FINISH);

    return err;
}


/**
 * @brief See description in the header file.
 *
 */
ucoap_error ucoap_rx_byte(ucoap_handle * const handle, const uint8_t byte)
{
    if (UCOAP_CHECK_STATUS(handle, UCOAP_WAITING_RESP)) {

        if (handle->response.len < UCOAP_MAX_PDU_SIZE) {
            handle->response.buf[handle->response.len++] = byte;

            ucoap_tx_signal(handle, UCOAP_RESPONSE_BYTE_DID_RECEIVE);
            return UCOAP_OK;
        }

        return UCOAP_RX_BUFF_FULL_ERROR;
    }

    return UCOAP_WRONG_STATE_ERROR;
}


/**
 * @brief See description in the header file.
 *
 */
ucoap_error ucoap_rx_packet(ucoap_handle * const handle, const uint8_t * buf,
        const uint32_t len) {
    if (UCOAP_CHECK_STATUS(handle, UCOAP_WAITING_RESP)) {

        mem_copy(handle->response.buf, buf,
                len < UCOAP_MAX_PDU_SIZE? len: UCOAP_MAX_PDU_SIZE);
        handle->response.len = len;

        if (len < UCOAP_MAX_PDU_SIZE) {
            ucoap_tx_signal(handle, UCOAP_RESPONSE_DID_RECEIVE);
            return UCOAP_OK;
        }

        return UCOAP_RX_BUFF_FULL_ERROR;
    }

    return UCOAP_WRONG_STATE_ERROR;
}


/**
 * @brief Init CoAP driver
 *
 * @param handle - coap handle
 * @param reqd - descriptor of request
 *
 * @return status of operation
 */
static ucoap_error init_coap_driver(ucoap_handle * const handle,
        const ucoap_request_descriptor * const reqd) {
    ucoap_error err;

    err = UCOAP_OK;
    handle->request.len = 0;
    handle->response.len = 0;

    if (reqd->code == UCOAP_CODE_EMPTY_MSG && reqd->tkl) {
        return UCOAP_PARAM_ERROR;
    }

    if (handle->request.buf == NULL) {
        err = ucoap_alloc_mem_block(&handle->request.buf, UCOAP_MAX_PDU_SIZE);

        if (err != UCOAP_OK) {
            return err;
        }
    }

    if (reqd->type == UCOAP_MESSAGE_CON || reqd->response_callback != NULL) {
        if (handle->response.buf == NULL) {
            err = ucoap_alloc_mem_block(&handle->response.buf,
                    UCOAP_MAX_PDU_SIZE);
        }
    }

    return err;
}


/**
 * @brief Deinit CoAP driver
 *
 * @param handle - coap handle
 *
 */
static void deinit_coap_driver(ucoap_handle * handle)
{
    if (handle->response.buf != NULL) {
        ucoap_free_mem_block(handle->response.buf, UCOAP_MAX_PDU_SIZE);
        handle->response.buf = NULL;
    }

    if (handle->request.buf != NULL) {
        ucoap_free_mem_block(handle->request.buf, UCOAP_MAX_PDU_SIZE);
        handle->request.buf = NULL;
    }

    handle->request.len = 0;
    handle->response.len = 0;
}
