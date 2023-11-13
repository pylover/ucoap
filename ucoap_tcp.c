/**
 * ucoap_tcp.c
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#include "ucoap_tcp.h"
#include "ucoap_utils.h"



#define UCOAP_MIN_TCP_HEADER_LEN     2u

#define UCOAP_TCP_LEN_1BYTE          13
#define UCOAP_TCP_LEN_2BYTES         14
#define UCOAP_TCP_LEN_4BYTES         15

#define UCOAP_TCP_LEN_MIN            13
#define UCOAP_TCP_LEN_MED            269
#define UCOAP_TCP_LEN_MAX            65805


/**
 * Auxiliary data structures
 *
 */
typedef union {

    struct {
        uint8_t tkl  : 4;         /* length of Token */
        uint8_t len  : 4;         /* length of Options & Payload */
    } fields;

    uint8_t byte;
} ucoap_tcp_len_header;


typedef struct {

    ucoap_tcp_len_header len_header;

    uint8_t code;
    uint32_t data_len;

} ucoap_tcp_header;



static void asemble_request(ucoap_handle * const handle, ucoap_data * const request, const ucoap_request_descriptor * const reqd);
static uint32_t parse_response(const ucoap_data * const request, const ucoap_data * const response, uint32_t * const options_shift);
static uint32_t extract_data_length(ucoap_tcp_header * const header, const uint8_t * const buf);
static void shift_data(uint8_t * dst, const uint8_t * src, uint32_t len);



/**
 * @brief See description in the header file.
 *
 */
ucoap_error ucoap_send_coap_request_tcp(ucoap_handle * const handle, const ucoap_request_descriptor * const reqd)
{
    ucoap_error err;
    uint32_t resp_mask;
    uint32_t option_start_idx;
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

    /* waiting response if needed */
    resp_mask = UCOAP_RESP_EMPTY;
    if (reqd->response_callback != NULL) {

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
            ucoap_debug_print_packet(handle, "coap << ", handle->response.buf, handle->response.len);
        }

        /* parsing incoming packet */
        resp_mask = parse_response(&handle->request, &handle->response, &option_start_idx);

        if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_INVALID_PACKET)) {

            ucoap_tx_signal(handle, UCOAP_WRONG_PACKET_DID_RECEIVE);
            err = UCOAP_NO_RESP_ERROR;

            return err;
        } else if (UCOAP_CHECK_RESP(resp_mask, UCOAP_RESP_NRST)) {

            ucoap_tx_signal(handle, UCOAP_NRST_DID_RECEIVE);
            err = UCOAP_NRST_ANSWER;

            return err;
        }

        /* We are using the same request buffer for storing incoming options, bcoz
         * outgoing packet is not needed already. It allows us to save ram-memory.
         */
        err = decoding_options(&handle->response,
                (ucoap_option_data *)handle->request.buf,
                option_start_idx,
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

        /* response_code_idx = option_start_idx - (handle->response.buf[0] & 0x0f) - 1 */
        result.resp_code = handle->response.buf[option_start_idx - (handle->response.buf[0] & 0x0f) - 1];
        result.options = err == UCOAP_NO_OPTIONS_ERROR ? NULL : (ucoap_option_data *)handle->request.buf;

        reqd->response_callback(reqd, &result);

        /* debug support */
        if (UCOAP_CHECK_STATUS(handle, UCOAP_DEBUG_ON)) {
            ucoap_debug_print_options(handle, "coap opt << ", result.options);
            ucoap_debug_print_payload(handle, "coap pld << ", &result.payload);
        }
    }

    return err;
}


/**
 * @brief Assemble CoAP over TCP request.
 *
 * @param handle - coap handle
 * @param request - data struct for storing request
 * @param reqd - descriptor of request
 *
 */
static void asemble_request(ucoap_handle * const handle, ucoap_data * const request, const ucoap_request_descriptor * const reqd)
{
    uint32_t options_shift;
    uint32_t options_len;
    ucoap_tcp_len_header header;

/**
  * CoAP over TCP has a header with variable length. Therefore we should calculate
  * length of Options & Payload before assembling the header.
  * At first we will try to predict length of header.
  * We should be shift a data, if we will predict wrong length of header.
  *
  */
    options_len = 0;
    options_shift = UCOAP_MIN_TCP_HEADER_LEN + reqd->tkl;

    if (reqd->payload.len > 10) {
        options_shift += 1;
    }

    /* assemble options */
    if (reqd->options != NULL) {
        options_len += encoding_options(request->buf + options_shift, reqd->options);
    }

    /* assemble header */
    request->len = options_len + (reqd->payload.len ? reqd->payload.len + 1 : 0);
    header.fields.tkl = reqd->tkl;

    if (request->len < UCOAP_TCP_LEN_MIN) {

        header.fields.len = request->len;

        request->buf[0] = header.byte;
        request->buf[1] = reqd->code;

        /* check on shift data */
        if (options_shift > (UCOAP_MIN_TCP_HEADER_LEN + reqd->tkl)) {
            shift_data(request->buf + reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN, request->buf + options_shift, options_len);
        }

        request->len = 2;

    } else if (request->len < UCOAP_TCP_LEN_MED) {

        header.fields.len = UCOAP_TCP_LEN_1BYTE;

        request->buf[0] = header.byte;
        request->buf[1] = request->len - UCOAP_TCP_LEN_MIN;

        /* check on shift data */
        if (options_shift > reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 1) {

            shift_data(request->buf + reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 1,
                    request->buf + options_shift, options_len);

        } else if (options_shift < reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 1) {

            shift_data(request->buf + options_len + reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN,
                    request->buf + options_shift + options_len - 1, options_len);
        }

        request->buf[2] = reqd->code;
        request->len = 3;

    } else if (request->len < UCOAP_TCP_LEN_MAX) {

        header.fields.len = UCOAP_TCP_LEN_2BYTES;
        request->buf[0] = header.byte;

        /* check on shift data */
        if (options_shift > reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 2) {

            shift_data(request->buf + reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 2,
                    request->buf + options_shift, options_len);

        } else if (options_shift < reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 2) {

            shift_data(request->buf + options_len + reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 1,
                    request->buf + options_shift + options_len - 1, options_len);
        }

        request->buf[1] = (request->len - UCOAP_TCP_LEN_MED) >> 8;
        request->buf[2] = (request->len - UCOAP_TCP_LEN_MED);
        request->buf[3] = reqd->code;
        request->len = 4;

    } else {

        header.fields.len = UCOAP_TCP_LEN_4BYTES;
        request->buf[0] = header.byte;

        /* check on shift data */
        if (options_shift > reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 4) {

            shift_data(request->buf + reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 1,
                    request->buf + options_shift, options_len);

        } else if (options_shift < reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 4) {

            shift_data(request->buf + options_len + reqd->tkl + UCOAP_MIN_TCP_HEADER_LEN + 3,
                    request->buf + options_shift + options_len - 1, options_len);
        }

        request->buf[1] = (request->len - UCOAP_TCP_LEN_MAX) >> 24;
        request->buf[2] = (request->len - UCOAP_TCP_LEN_MAX) >> 16;
        request->buf[3] = (request->len - UCOAP_TCP_LEN_MAX) >> 8;
        request->buf[4] = (request->len - UCOAP_TCP_LEN_MAX);
        request->buf[5] = reqd->code;
        request->len = 6;
    }

    /* assemble token */
    if (reqd->tkl) {
        ucoap_fill_token(handle, request->buf + request->len, reqd->tkl);
        request->len += reqd->tkl;
    }

    request->len += options_len;

    /* assemble payload */
    if (reqd->payload.len) {
        request->len += fill_payload(request->buf + request->len, &reqd->payload);
    }
}


/**
 * @brief Parse CoAP response
 *
 * @param request - pointer on outgoing packet
 * @param response - pointer on incoming packet
 * @param options_shift - in this variable will be stored start of options index
 *
 * @return bit mask of parsing results, see 'ucoap_parsing_result'
 */
static uint32_t parse_response(const ucoap_data * const request, const ucoap_data * const response, uint32_t * const options_shift)
{
    ucoap_tcp_header resp_header;
    ucoap_tcp_header req_header;

    uint32_t resp_mask;
    uint32_t resp_idx;
    uint32_t req_idx;

    /* checking header */
    if (response->len > 1) {
        resp_mask = UCOAP_RESP_SEPARATE;
        resp_idx = 0;
        req_idx = 0;

        resp_header.len_header.byte = response->buf[resp_idx++];
        req_header.len_header.byte = request->buf[req_idx++];

        /* fast checking tkl */
        if (resp_header.len_header.fields.tkl != req_header.len_header.fields.tkl) {
            goto return_err_label;
        }

        resp_idx += extract_data_length(&resp_header, response->buf + resp_idx);
        req_idx += extract_data_length(&req_header, request->buf + req_idx);

        /* check length */
        if ((resp_header.data_len + resp_header.len_header.fields.tkl + resp_idx + 1) > response->len) {
            goto return_err_label;
        }

        /* get code */
        resp_header.code = response->buf[resp_idx++];

        /* check code */
        if (UCOAP_EXTRACT_CLASS(resp_header.code) != UCOAP_SUCCESS_CLASS
                && UCOAP_EXTRACT_CLASS(resp_header.code) != UCOAP_BAD_REQUEST_CLASS
                && UCOAP_EXTRACT_CLASS(resp_header.code) != UCOAP_SERVER_ERR_CLASS
                && UCOAP_EXTRACT_CLASS(resp_header.code) != UCOAP_TCP_SIGNAL_CLASS) {
            goto return_err_label;
        }

        /* check token */
        if (resp_header.len_header.fields.tkl) {
            if (!mem_cmp(response->buf + resp_idx, request->buf + req_idx + 1, resp_header.len_header.fields.tkl)) {
                goto return_err_label;
            }
        }

        if (UCOAP_EXTRACT_CLASS(resp_header.code) == UCOAP_SUCCESS_CLASS) {
            UCOAP_SET_RESP(resp_mask, UCOAP_RESP_SUCCESS_CODE);
        } else if (UCOAP_EXTRACT_CLASS(resp_header.code) == UCOAP_TCP_SIGNAL_CLASS) {
            UCOAP_SET_RESP(resp_mask, UCOAP_RESP_TCP_SIGNAL_CODE);
        } else {
            UCOAP_SET_RESP(resp_mask, UCOAP_RESP_FAILURE_CODE);
        }

        /* packet is valid */
        *options_shift = response->len - resp_header.data_len;
        return resp_mask;
    }

/***********/
return_err_label:
/***********/

    *options_shift = 0;
    resp_mask = UCOAP_RESP_INVALID_PACKET;
    return resp_mask;
}


/**
 * @brief Extract length of data from header for TCP packet (payload + options)
 *
 * @param header - pointer on 'ucoap_tcp_header'
 * @param buf - pointer on packet buffer
 *
 * @return shift for length
 */
static uint32_t extract_data_length(ucoap_tcp_header * const header, const uint8_t * const buf)
{
    uint32_t idx;

    idx = 0;

    switch (header->len_header.fields.len) {
        case UCOAP_TCP_LEN_1BYTE:
            header->data_len = buf[idx++] + UCOAP_TCP_LEN_MIN;
            break;

        case UCOAP_TCP_LEN_2BYTES:
            header->data_len = buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len += UCOAP_TCP_LEN_MED;
            break;

        case UCOAP_TCP_LEN_4BYTES:
            header->data_len = buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len += UCOAP_TCP_LEN_MAX;
            break;

        default:
            header->data_len = header->len_header.fields.len;
            break;
    }

    return idx;
}


/**
 * @brief Shift the data in the packet if we did predict a wrong length
 *
 * @param dst - pointer on new position of data
 * @param src - pointer on current position of data
 * @param len - length of the shift
 */
static void shift_data(uint8_t * dst, const uint8_t * src, uint32_t len)
{
    if (dst < src) {
        while (len--) *dst++ = *src++;
    } else {
        while (len--) *dst-- = *src--;
    }
}

