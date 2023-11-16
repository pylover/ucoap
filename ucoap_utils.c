/**
 * ucoap_utils.c
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */



#include "ucoap_utils.h"


#define UCOAP_OPT_MIN                13
#define UCOAP_OPT_MED                269

#define UCOAP_OPT_1BYTE              13
#define UCOAP_OPT_2BYTE              14
#define UCOAP_OPT_DIS                15

#define UCOAP_PAYLOAD_PREFIX         0xff



/**
 * @brief See description in the header file.
 *
 */
uint32_t encoding_options(uint8_t * const buf, const ucoap_option_data * options)
{
    uint32_t idx;
    uint32_t local_idx;

    uint16_t delta;
    uint16_t delta_sum;

    delta_sum = 0;
    idx = 0;

    do {
        local_idx = idx;

        /* option */
        delta = options->num - delta_sum;
        delta_sum += delta;

        if (delta < UCOAP_OPT_MIN) {

            buf[idx++] = (delta << 4);
        } else if (delta < UCOAP_OPT_MED) {

            buf[idx++] = (UCOAP_OPT_1BYTE << 4);
            buf[idx++] = delta - UCOAP_OPT_MIN;
        } else {

            buf[idx++] = (UCOAP_OPT_2BYTE << 4);
            buf[idx++] = (delta - UCOAP_OPT_MED) >> 8;
            buf[idx++] = (delta - UCOAP_OPT_MED) & 0x00FF;
        }

        /* length */
        if (options->len < UCOAP_OPT_MIN) {

            buf[local_idx] |= options->len;
        } else if (options->len < UCOAP_OPT_MED) {

            buf[local_idx] |= UCOAP_OPT_1BYTE;
            buf[idx++] = options->len - UCOAP_OPT_MIN;
        } else {

            buf[local_idx] |= UCOAP_OPT_2BYTE;
            buf[idx++] = (options->len - UCOAP_OPT_MED) >> 8;
            buf[idx++] = (options->len - UCOAP_OPT_MED) & 0x00FF;
        }

        /* value */
        mem_copy(buf + idx, options->value, options->len);
        idx += options->len;

        options = options->next;

    } while(options != NULL);


    return idx;
}


/**
 * @brief See description in the header file.
 *
 */
enum ucoap_error
decoding_options(const ucoap_data * const response,
        ucoap_option_data * options,
        const uint32_t const opt_start_idx,
        uint32_t * const payload_start_idx) {
    ucoap_error err;
    uint32_t idx;

    uint8_t opt;
    uint16_t delta_sum;

    /* initialize */
    err = UCOAP_NO_OPTIONS_ERROR;
    idx = opt_start_idx;
    opt = response->buf[idx++];

    /* decoding */
    if (response->len > idx && opt != UCOAP_PAYLOAD_PREFIX) {
        delta_sum = 0;
        options->next = NULL;

        do {

            if (options->next != NULL) {
                options = options->next;
            }

            /* option */
            switch (opt >> 4) {
                case UCOAP_OPT_1BYTE:
                    options->num = response->buf[idx++] + UCOAP_OPT_MIN + delta_sum;
                    delta_sum = options->num;
                    break;

                case UCOAP_OPT_2BYTE:
                    options->num = response->buf[idx++];
                    options->num <<= 8;
                    options->num |= response->buf[idx++];
                    options->num += delta_sum + UCOAP_OPT_MED;

                    delta_sum = options->num;
                    break;

                case UCOAP_OPT_DIS:
                    err = UCOAP_WRONG_OPTIONS_ERROR;
                    goto return_label;

                default:
                    options->num = (opt >> 4) + delta_sum;
                    delta_sum += (opt >> 4);
                    break;
            }

            /* length */
            switch (opt & 0x0F) {
                case UCOAP_OPT_1BYTE:
                    options->len = response->buf[idx++] + UCOAP_OPT_MIN;
                    break;

                case UCOAP_OPT_2BYTE:
                    options->len = response->buf[idx++];
                    options->len <<= 8;
                    options->len |= response->buf[idx++];
                    options->len += UCOAP_OPT_MED;
                    break;

                case UCOAP_OPT_DIS:
                    err = UCOAP_WRONG_OPTIONS_ERROR;
                    goto return_label;

                default:
                    options->len = (opt & 0x0F);
                    break;
            }

            /* value */
            options->value = response->buf + idx;

            /* shift counters */
            idx += options->len;
            options->next = (options + 1);

            opt = response->buf[idx++];

        } while (opt != UCOAP_PAYLOAD_PREFIX);

        options->next = NULL;
        err = UCOAP_OK;
    }

/***********/
return_label:
/***********/

    *payload_start_idx = idx;
    return err;
}


/**
 * @brief See description in the header file.
 *
 */
uint32_t fill_payload(uint8_t * const buf, const ucoap_data * const payload)
{
    *buf = UCOAP_PAYLOAD_PREFIX;

    mem_copy(buf + 1, payload->buf, payload->len);

    return payload->len + 1;
}


