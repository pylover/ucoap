/*
 * @brief Example for block-wise.
 *        Get a configuration from server and write it to the flash memory.
 *
 * @Author:  Serge Maslyakov
 * @Created: 27 Apr 2017
 *
 */


#include "srv_get_config_task.h"

#include "includes.h"

#include "server_utils.h"
#include <ucoap_helpers.h>

#include "coap/coap_transport.h"
#include "configuration/configuration.h"



static struct {
    uint16_t block_num;
    uint16_t next_block_num;
} config_manager;


static void get_config_response_callback(const struct ucoap_request_descriptor * const reqd, const ucoap_result_data * const result);
static bool write_config(const ucoap_data * const data, const uint32_t shift);



/**
 *
 */
ucoap_error srv_get_config_task(ucoap_request_descriptor * const request, const uint32_t token)
{
    ucoap_error err;

    ucoap_blockwise_data bw;
    uint8_t value;

    ucoap_option_data opt_etag;
    ucoap_option_data opt_path;
    ucoap_option_data opt_block2;

    /* initialize */
    config_manager.block_num = 0;
    config_manager.next_block_num = 0;

    srv_fill_etag_opt(&opt_etag, &token);
    srv_fill_uri_path_opt(&opt_path, "config");

    bw.fld.block_szx = 2;  /* 64 bytes */
    bw.fld.more = 0;

    opt_etag.next = &opt_path;
    opt_path.next = &opt_block2;

    do {
        /* block wise */
        bw.fld.num = config_manager.next_block_num;
        ucoap_fill_block2_opt(&opt_block2, &bw, &value);

        request->payload.len = 0;
        request->code = UCOAP_REQ_GET;
        request->tkl = 2;
        request->type = tc_handle.transport == UCOAP_UDP ? UCOAP_MESSAGE_CON : UCOAP_MESSAGE_NON;
        request->options = &opt_etag;
        request->response_callback = get_config_response_callback;

        ucoap_debug(&tc_handle, true);
        err = ucoap_send_coap_request(&tc_handle, request);

    } while (config_manager.block_num != config_manager.next_block_num && err == UCOAP_OK);

    return err;
}


/**
 *
 */
static void get_config_response_callback(const struct ucoap_request_descriptor * const reqd, const ucoap_result_data * const result)
{
    (void)reqd;

    const ucoap_option_data * block2;
    ucoap_blockwise_data bw;

    if (UCOAP_EXTRACT_CLASS(result->resp_code) == UCOAP_SUCCESS_CLASS && result->payload.len > 0)
    {
        /* get block2 option */
        block2 = ucoap_find_option_by_number(result->options, UCOAP_BLOCK2_OPT);

        if (block2 != NULL) {
            ucoap_extract_block2_from_opt(block2, &bw);

            /* shift counters */
            config_manager.block_num = bw.fld.num;

            if (bw.fld.more) {
                config_manager.next_block_num = bw.fld.num + 1;
            }

            /* write data */
            write_config(&result->payload, config_manager.block_num * ucoap_decode_szx_to_size(bw.fld.block_szx));
        } else {
            /* finish transfer */
            config_manager.block_num = config_manager.next_block_num = 0;
        }
    }
}


static bool write_config(const ucoap_data * const data, const uint32_t shift)
{
    return cfg_write_raw_config(data->buf, data->len, shift);
}
