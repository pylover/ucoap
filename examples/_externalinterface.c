__ucoap_error ucoap_tx_data(__ucoap_handle * const handle, const uint8_t *buf, const uint32_t len)
{
    (void)handle;

    /* send data via modem */
    modem_send_data(buf, len);

    return UCOAP_OK;
}


/* implementation for rtos */
__ucoap_error ucoap_wait_event(__ucoap_handle * const handle, const uint32_t timeout_ms)
{
    uint32_t err;

    err = rtos_wait_event(UCOAP_DATA_DID_RECEIVE_EVENTGR, timeout_ms);

    if (err == RTOS_OK) {
        return UCOAP_OK;
    } else if (err = RTOS_TIMEOUT) {
        return UCOAP_TIMEOUT_ERROR;
    }
}

/* events from ucoap */
__ucoap_error ucoap_tx_signal(__ucoap_handle * const handle, const __ucoap_out_signal signal)
{
    (void)handle;

    switch (signal) {

        case UCOAP_ROUTINE_PACKET_WILL_START:
            break;

        case UCOAP_ROUTINE_PACKET_DID_FINISH:
            break;

        case UCOAP_TX_RETR_PACKET:
            break;

        case UCOAP_TX_ACK_PACKET:
            break;

        case UCOAP_ACK_DID_RECEIVE:
            break;

        case UCOAP_NRST_DID_RECEIVE:
            break;

        case UCOAP_WRONG_PACKET_DID_RECEIVE:
            break;

        case UCOAP_RESPONSE_BYTE_DID_RECEIVE:
            break;

        case UCOAP_RESPONSE_TO_LONG_ERROR:
            break;

        case UCOAP_RESPONSE_DID_RECEIVE:
            rtos_send_event(UCOAP_DATA_DID_RECEIVE_EVENTGR);
            break;

        default:
            return UCOAP_PARAM_ERROR;
    }

    return UCOAP_OK;
}


uint16_t ucoap_get_message_id(__ucoap_handle * const handle)
{
    static uint16_t id;

    (void)handle;

    return id++;
}


__ucoap_error ucoap_fill_token(__ucoap_handle * const handle, uint8_t *token, const uint32_t tkl)
{
/**
  *  5.3.1.  Token
  *
  *     The Token is used to match a response with a request.  The token
  *     value is a sequence of 0 to 8 bytes.  (Note that every message
  *     carries a token, even if it is of zero length.)  Every request
  *     carries a client-generated token that the server MUST echo (without
  *     modification) in any resulting response.
  *
  *     The client SHOULD generate tokens in such a way that tokens currently
  *     in use for a given source/destination endpoint pair are unique.
  *     (Note that a client implementation can use the same token for any
  *     request if it uses a different endpoint each time, e.g., a different
  *     source port number.)  An empty token value is appropriate e.g., when
  *     no other tokens are in use to a destination, or when requests are
  *     made serially per destination and receive piggybacked responses.
  *     There are, however, multiple possible implementation strategies to
  *     fulfill this.
 */
    (void)handle;

    /* TODO - need to make support for real token*/
    long tkn = 123456789;

    if (tkl > 4) {
        return UCOAP_PARAM_ERROR;
    }

    if (tkl) {
        mem_copy(&token, tkn, tkl);
    }

    tkn++;

    return UCOAP_OK;
}


__ucoap_error ucoap_alloc_mem_block(uint8_t **block, const uint32_t min_len)
{
    bool success;

    success = mem_get_block((void **)block, min_len);

    return success ? UCOAP_OK : UCOAP_NO_FREE_MEM_ERROR;
}


__ucoap_error ucoap_free_mem_block(uint8_t *block, const uint32_t min_len)
{
    bool success;

    success = mem_release_block(block, min_len);

    return success ? UCOAP_OK : UCOAP_NO_FREE_MEM_ERROR;
}


void ucoap_debug_print_packet(__ucoap_handle * const handle, const char * msg, uint8_t *data, const uint32_t len)
{
    DEBUG_PRINTF("\r\n%s %d %s", handle->name, handle->transport, msg);

    for (uint32_t i = 0; i < len; i++) {
        DEBUG_PRINTF("%2X", data[i]);
    }

    DEBUG_PRINTF("\r\n");
}


void ucoap_debug_print_options(__ucoap_handle * const handle, const char * msg, const __ucoap_option_data * options)
{
    DEBUG_PRINTF("\r\n%s %d %s\r\n", handle->name, handle->transport, msg);

    if (options != NULL) {

        do {
            DEBUG_PRINTF("Num:%d\r\nLen:%d\r\nValue:", options->num, options->len);

            if (options->len) {
                for (uint32_t i = 0; i < options->len; i++) {
                    DEBUG_PRINTF("%2X", options->value[i]);
                }
                DEBUG_PRINTF("\r\n");
            } else {
                DEBUG_PRINTF("default\r\n");
            }

            DEBUG_PRINTF("\r\n");

            options = options->next;
        } while (options != NULL);

    } else {
        DEBUG_PRINTF("There are no options\r\n");
    }
}


void ucoap_debug_print_payload(__ucoap_handle * const handle, const char * msg, const __ucoap_data * const payload)
{
    DEBUG_PRINTF("\r\n%s %d %s", handle->name, handle->transport, msg);

    if (payload->len) {
        for (uint32_t i = 0; i < payload->len; i++) {
            DEBUG_PRINTF("%2X", payload->buf[i]);
        }

        DEBUG_PRINTF("\r\n");
    } else {
        DEBUG_PRINTF("There is no payload\r\n");
    }
}
