### CoAP client library for embedded devices in C

Allows to add the CoAP functionality for embedded device.

#### Features

- Very small memory consumption (in the common case it may be about of 200 
  bytes for both rx/tx buffers, you can tune a PDU size)
- Implemented CoAP over UDP 
  [rfc7252](https://tools.ietf.org/html/rfc7252)
- Implemented CoAP over TCP 
  [draft-coap-tcp-tls-07](https://tools.ietf.org/html/draft-ietf-core-coap-tcp-tls-07)
- Retransmition/acknowledgment functionality
- Parsing of responses. Received data will be return to the user via callback.
- Helpers for block-wise mode. The block-wise mode is located at a higher 
  level of abstraction than this implementation.
  See `examples` directory.


#### How to send CoAP request to server

1) Include `ucoap.h` in your code.

```C
#include "ucoap.h"

```

There are several functions in the `ucoap.h` which are declared how 
`external`. You should provide it implementation in your code. 
See `examples` directory for common case of their implementation.

```C
/**
 * @brief In this function user should implement a transmission given data via 
 *        hardware interface (e.g. serial port)
 * 
 */
extern ucoap_error 
ucoap_tx_data(ucoap_handle * const handle, const uint8_t * buf, 
        const uint32_t len);


/**
 * @brief In this function user should implement a functionality of waiting 
 *        response. 
 *        This function has to return a control when timeout will expired or 
 *        when response from server will be received.
 */
extern ucoap_error 
ucoap_wait_event(ucoap_handle * const handle, const uint32_t timeout_ms);


/**
 * @brief Through this function the 'ucoap' lib will be notifying about 
 *        events.
 *        See possible events here 'ucoap_out_signal'.
 */
extern ucoap_error 
ucoap_tx_signal(ucoap_handle * const handle, const ucoap_out_signal signal);


/**
 * @brief In this function user should implement a generating of message id.
 * 
 */
extern uint16_t 
ucoap_get_message_id(ucoap_handle * const handle);


/**
 * @brief In this function user should implement a generating of token.
 * 
 */
extern ucoap_error 
ucoap_fill_token(ucoap_handle * const handle, uint8_t * token, 
        const uint32_t tkl);


/**
 * @brief These functions are using for debug purpose, if user will enable 
 *        debug mode.
 * 
 */
extern void ucoap_debug_print_packet(ucoap_handle * const handle, 
        const char * msg, uint8_t * data, const uint32_t len);
extern void ucoap_debug_print_options(ucoap_handle * const handle, 
        const char * msg, const ucoap_option_data * options);
extern void ucoap_debug_print_payload(ucoap_handle * const handle, 
        const char * msg, const ucoap_data * const payload);


/**
 * @brief In this function user should implement an allocating block of 
 *        memory.
 *        In the simple case it may be a static buffer. The 'UCOAP' will make
 *        two calls of this function before starting work (for rx and tx 
 *        buffer).
 *        So, you should have minimum two separate blocks of memory.
 * 
 */
extern ucoap_error ucoap_alloc_mem_block(uint8_t ** block, 
        const uint32_t min_len);


/**
 * @brief In this function user should implement a freeing block of memory.
 * 
 */
extern ucoap_error ucoap_free_mem_block(uint8_t * block, 
        const uint32_t min_len);

extern void mem_copy(void * dst, const void * src, uint32_t cnt);
extern bool mem_cmp(const void * dst, const void * src, uint32_t cnt);

```

2) Define a `ucoap_handle` object, e.g.

```C
ucoap_handle tc_handle = {
        .name = "coap_over_gsm",
        .transport = UCOAP_UDP
};

```


3) Implement a transfer of incoming data from your hardware interface 
(e.g. serial port) to the `ucoap` either `ucoap_rx_byte` or `ucoap_rx_packet`. 
E.g.

```C
void uart1_rx_irq_handler() {
    uint8_t byte = UART1->DR;    
    ucoap_rx_byte(&tc_handle, byte);
}

void eth_rx_irq_handler(uint8_t * data, uint32_t len) {
    ucoap_rx_packet(&tc_handle, data, len);
}

```


4) Send a coap request and get back response data in the provided callback:

```C

static void data_resource_response_callback(
        const struct ucoap_request_descriptor * const reqd, 
        const ucoap_result_data * const result) {
    // ... check response
}


void send_request_to_data_resource(uint32_t *token_etag, 
        uint8_t * data, uint32_t len) {
    ucoap_error err;
    ucoap_request_descriptor data_request;

    ucoap_option_data opt_etag;
    ucoap_option_data opt_path;
    ucoap_option_data opt_content;

    /* fill options - we should adhere an order of options */
    opt_content.num = UCOAP_CONTENT_FORMAT_OPT;
    /* 42 = UCOAP_APPLICATION_OCTET_STREAM */
    opt_content.value = (uint8_t *)"\x2A";
    opt_content.len = 1;
    opt_content.next = NULL;

    opt_path.num = UCOAP_URI_PATH_OPT;
    opt_path.value = (uint8_t *)"data";
    opt_path.len = 4;
    opt_path.next = &opt_content;

    opt_etag.num = UCOAP_ETAG_OPT;
    opt_etag.value = (uint8_t *)token_etag;
    opt_etag.len = 4;
    opt_etag.next = &opt_path;

    /* fill the request descriptor */
    data_request.payload.buf = data;
    data_request.payload.len = len;

    data_request.code = UCOAP_REQ_POST;
    data_request.tkl = 2;
    data_request.type = tc_handle.transport == UCOAP_UDP? UCOAP_MESSAGE_CON: 
        UCOAP_MESSAGE_NON;
    data_request.options = &opt_etag;
    
    /* define the callback for response data */
    data_request.response_callback = data_resource_response_callback;

    /* enable debug */
    ucoap_debug(&tc_handle, true);
    
    /* send request */
    err = ucoap_send_coap_request(&tc_handle, &data_request);

    if (err != UCOAP_OK) {
        // error handling
    }
}

```
