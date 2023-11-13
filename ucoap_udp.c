/**
 * ucoap_udp.c
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#include "ucoap_udp.h"
#include "ucoap_utils.h"


#define UCOAP_RESPONSE_CODE(buf)     ((buf)[1])


/**
 * @brief CoAP header data struct
 */
typedef struct {

    uint8_t tkl   : 4;        /* length of Token */
    uint8_t type  : 2;        /* type flag */
    uint8_t vers  : 2;        /* protocol version */

    uint8_t code  : 8;        /* request method (value 1--10) or response
                                 code (value 40-255) */
    uint16_t mid;             /* transaction id (network byte order!) */

} ucoap_udp_header;



static void asemble_request(ucoap_handle * const handle, ucoap_data * const request, const ucoap_request_descriptor * const reqd);
static uint32_t parse_response(const ucoap_data * const request, const ucoap_data * const response);
static void asemble_ack(ucoap_data * const ack, const ucoap_data * const response);
static ucoap_error waiting_ack(ucoap_handle * const handle, const ucoap_data * const request);



/**
 * @brief See description in the header file.
 *
 */
ucoap_error ucoap_send_coap_request_udp(ucoap_handle * const handle, const ucoap_request_descriptor * const reqd)
{
    ucoap_error err;
    uint32_t resp_mask;
    ucoap_result_data result;

    /* assembling packet */
    asemble_request(handle, &handle->request, reqd);

    /* debug support */
    if (UCOAP_CHECK_STATUS(handle, UCOAP_DEBUG_ON)) {
        ucoap_debug_print_packet(handle, "coap >> ", handle->request.buf, handle->request.len);
    }

    /* sending packet */
    ucoap_tx_signal(handle, UCOAP_ROUTINE_PACKET_WILL_START);

    err = ucoap_tx_data(handle, handle->request.buf, handle->request.len);

    if (err != UCOAP_OK) {
        return err;
    }

    /* waiting ack if needed */
    resp_mask = UCOAP_RESP_EMPTY;
    if (reqd->type == UCOAP_MESSAGE_CON) {

        UCOAP_SET_STATUS(handle, UCOAP_WAITING_RESP);

        err = waiting_ack(handle, &handle->request);

        UCOAP_RESET_STATUS(handle, UCOAP_WAITING_RESP);

        if (err != UCOAP_OK) {
            return err;
        }

        /* debug support */
        if (UCOAP_CHECK_STATUS(handle, UCOAP_DEBUG_ON)) {
            ucoap_debug_print_packet(handle, "coap << ", handle->response.buf, handle->response.len);
        }

        /* parsing incoming ack packet */
        resp_mask = parse_response(&handle->request, &handle->response);

        if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_ACK)) {

            ucoap_tx_signal(handle, UCOAP_ACK_DID_RECEIVE);

        } else if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_NRST)) {

            ucoap_tx_signal(handle, UCOAP_NRST_DID_RECEIVE);
            err = UCOAP_NRST_ANSWER;

            return err;
        } else if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_INVALID_PACKET)) {

            ucoap_tx_signal(handle, UCOAP_WRONG_PACKET_DID_RECEIVE);
            err = UCOAP_NO_ACK_ERROR;

            return err;
        }
    }

    /* waiting response if needed */
    if (reqd->response_callback != NULL) {

        if (reqd->type != UCOAP_MESSAGE_CON || !UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_PIGGYBACKED)) {

            handle->response.len = 0;
            UCOAP_SET_STATUS(handle, UCOAP_WAITING_RESP);

            /* waiting either data arriving or timeout expiring */
            err = ucoap_wait_event(handle, UCOAP_RESP_TIMEOUT_MS);

            UCOAP_RESET_STATUS(handle, UCOAP_WAITING_RESP);

            if (err != UCOAP_OK) {
                return err;
            }

            /* debug support */
            if (UCOAP_CHECK_STATUS(handle, UCOAP_DEBUG_ON)) {
                ucoap_debug_print_packet(handle, "rcv coap << ", handle->response.buf, handle->response.len);
            }

            resp_mask = parse_response(&handle->request, &handle->response);

            if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_INVALID_PACKET)) {

                ucoap_tx_signal(handle, UCOAP_WRONG_PACKET_DID_RECEIVE);
                err = UCOAP_NO_RESP_ERROR;

                return err;
            } else if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_NRST)) {

                ucoap_tx_signal(handle, UCOAP_NRST_DID_RECEIVE);
                err = UCOAP_NRST_ANSWER;

                return err;
            }
        }

        /* We are using the same request buffer for storing incoming options, bcoz
         * outgoing packet is not needed already. It allows us to save ram-memory.
         */
        err = decoding_options(&handle->response,
                (ucoap_option_data *)handle->request.buf,
                ((handle->response.buf[0] & 0x0F) + 4),
                &handle->request.len);

        if (err == UCOAP_WRONG_OPTIONS_ERROR) {
            return err;
        }

        /* check the payload len */
        if (handle->response.len > handle->request.len) {
            result.payload.len = handle->response.len - handle->request.len;
            result.payload.buf = handle->response.buf + handle->request.len;
        } else {
            result.payload.len = 0;
            result.payload.buf = NULL;
        }

        result.resp_code = UCOAP_RESPONSE_CODE(handle->response.buf);
        result.options = err == UCOAP_NO_OPTIONS_ERROR ? NULL : (ucoap_option_data *)handle->request.buf;

        reqd->response_callback(reqd, &result);

        /* debug support */
        if (UCOAP_CHECK_STATUS(handle, UCOAP_DEBUG_ON)) {
            ucoap_debug_print_options(handle, "coap opt << ", result.options);
            ucoap_debug_print_payload(handle, "coap pld << ", &result.payload);
        }

        /* send ACK back if needed */
        if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_NEED_SEND_ACK)) {

            asemble_ack(&handle->request, &handle->response);
            ucoap_tx_signal(handle, UCOAP_TX_ACK_PACKET);

            err = ucoap_tx_data(handle, handle->request.buf, handle->request.len);
        }
    }

    return err;
}


/**
 * @brief Assemble CoAP over UDP request.
 *
 * @param handle - coap handle
 * @param request - data struct for storing request
 * @param reqd - descriptor of request
 *
 */
static void asemble_request(ucoap_handle * const handle, ucoap_data * const request, const ucoap_request_descriptor * const reqd)
{
    ucoap_udp_header header;

    request->len = sizeof(ucoap_udp_header);

    /* assemble header */
    header.vers = UCOAP_DEFAULT_VERSION;
    header.type = reqd->type;
    header.code = reqd->code;
    header.tkl = reqd->tkl;
    header.mid = ucoap_get_message_id(handle);

    /* assemble token */
    if (reqd->tkl) {
        ucoap_fill_token(handle, request->buf + request->len, reqd->tkl);
        request->len += reqd->tkl;
    }

    /* assemble options */
    if (reqd->options != NULL) {
        request->len += encoding_options(request->buf + request->len, reqd->options);
    }

    /* assemble payload */
    if (reqd->payload.len) {
        request->len += fill_payload(request->buf + request->len, &reqd->payload);
    }

    /* copy header */
    mem_copy(request->buf, &header, sizeof(ucoap_udp_header));
}


/**
 * @brief Parse CoAP response (it may be either an ACK response or separate response)
 *
 * @param request - pointer on outgoing packet data
 * @param response - pointer on incoming packet data
 *
 * @return bit mask of results parsing, see 'ucoap_parsing_result_t'
 */
static uint32_t parse_response(const ucoap_data * const request, const ucoap_data * const response)
{
    /**
     * 4.2.  Messages Transmitted Reliably
     * ...
     * ...
     * The Acknowledgement message MUST echo the Message ID of
     * the Confirmable message and MUST carry a response or be Empty (see
     * Sections 5.2.1 and 5.2.2).  The Reset message MUST echo the Message
     * ID of the Confirmable message and MUST be Empty.
     */

    ucoap_udp_header resp_header;
    ucoap_udp_header req_header;
    uint32_t resp_mask;

    /* check on header */
    if (response->len > 3) {

        resp_mask = UCOAP_RESP_EMPTY;
        mem_copy(&resp_header, response->buf, sizeof(ucoap_udp_header));
        mem_copy(&req_header, request->buf, sizeof(ucoap_udp_header));

        /* do fast checking */
        if (resp_header.vers != req_header.vers) {
            goto return_err_label;
        }

        /* do checking on the type of message */
        switch (resp_header.type) {

            case UCOAP_MESSAGE_ACK:
                UCOAP_SET_RESP(resp_mask, UCOAP_RESP_ACK);

                if (resp_header.mid != req_header.mid) {
                    goto return_err_label;
                }

                if (resp_header.code != UCOAP_CODE_EMPTY_MSG) {
                    UCOAP_SET_RESP(resp_mask, UCOAP_RESP_PIGGYBACKED);
                } else {
                    if (!resp_header.tkl && response->len == 4) {
                        return resp_mask;
                    } else {
                        goto return_err_label;
                    }
                }
                break;

            case UCOAP_MESSAGE_CON:
                UCOAP_SET_RESP(resp_mask, UCOAP_RESP_SEPARATE);
                UCOAP_SET_RESP(resp_mask, UCOAP_RESP_NEED_SEND_ACK);
                break;

            case UCOAP_MESSAGE_NON:
                UCOAP_SET_RESP(resp_mask, UCOAP_RESP_SEPARATE);
                break;

            case UCOAP_MESSAGE_RST:
                if (resp_header.code == UCOAP_CODE_EMPTY_MSG && !resp_header.tkl && response->len == 4) {
                    UCOAP_SET_RESP(resp_mask, UCOAP_RESP_NRST);
                    return resp_mask;
                } else {
                    goto return_err_label;
                }

            default:
                goto return_err_label;
        }

        /* if it is a separate response (msg id's should not be equals) */
        if (!UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_ACK)) {
            if (resp_header.mid == req_header.mid) {
                goto return_err_label;
            }
        }

        /* tkl's should be equals */
        if (resp_header.tkl != req_header.tkl) {
            goto return_err_label;
        }

        /* check length of msg */
        if (response->len < (uint32_t)(4 + resp_header.tkl)) {
            goto return_err_label;
        }

        /* check tokens */
        if (!mem_cmp(response->buf + 4, request->buf + 4, resp_header.tkl)) {
            goto return_err_label;
        }

        /* check code */
        if (UCOAP_EXTRACT_CLASS(resp_header.code) != UCOAP_SUCCESS_CLASS
                && UCOAP_EXTRACT_CLASS(resp_header.code) != UCOAP_BAD_REQUEST_CLASS
                && UCOAP_EXTRACT_CLASS(resp_header.code) != UCOAP_SERVER_ERR_CLASS) {
            goto return_err_label;
        }

        if (UCOAP_EXTRACT_CLASS(resp_header.code) == UCOAP_SUCCESS_CLASS) {
            UCOAP_SET_RESP(resp_mask, UCOAP_RESP_SUCCESS_CODE);
        } else {
            UCOAP_SET_RESP(resp_mask, UCOAP_RESP_FAILURE_CODE);
        }

        /* packet is valid */
        return resp_mask;
    }

/***********/
return_err_label:
/***********/

    resp_mask = UCOAP_RESP_INVALID_PACKET;
    return resp_mask;
}


/**
 * @brief Assemble ACK packet
 *
 * @param ack - data where was stored ACK packet
 * @param response - the response on the basis of which will be assemble ACK packet
 */
static void asemble_ack(ucoap_data * const ack, const ucoap_data * const response)
{
    ucoap_udp_header ack_header;

    /* get header from incoming packet */
    mem_copy(&ack_header, response, sizeof(ucoap_udp_header));

    /* assemble header */
    ack_header.type = UCOAP_MESSAGE_ACK;
    ack_header.code = UCOAP_CODE_EMPTY_MSG;
    ack_header.tkl = 0;

    /* copy header */
    mem_copy(ack->buf, &ack_header, sizeof(ucoap_udp_header));
    ack->len = sizeof(ucoap_udp_header);
}



/**
 * @brief Waiting ACK functionality (retransmission and etc)
 *
 * @param handle - coap handle
 * @param request - outgoing packet
 *
 * @return result of operation
 */
static ucoap_error waiting_ack(ucoap_handle * const handle, const ucoap_data * const request)
{
    ucoap_error err;
    uint32_t retransmition;

    retransmition = 0;

    do {

        err = ucoap_wait_event(handle, retransmition * ((UCOAP_ACK_TIMEOUT_MS * UCOAP_ACK_RANDOM_FACTOR) / 100) + UCOAP_ACK_TIMEOUT_MS);

        if (err == UCOAP_TIMEOUT_ERROR) {

            if (retransmition < UCOAP_MAX_RETRANSMIT) {
                /* retransmission */
                ucoap_tx_signal(handle, UCOAP_TX_RETR_PACKET);

                /* debug support */
                if (UCOAP_CHECK_STATUS(handle, UCOAP_DEBUG_ON)) {
                    ucoap_debug_print_packet(handle, "coap retr >> ", handle->request.buf, handle->request.len);
                }

                retransmition++;
                err = ucoap_tx_data(handle, request->buf, request->len);

                if (err != UCOAP_OK) {
                    break;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    } while (1);

    return err;
}

