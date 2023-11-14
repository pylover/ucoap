/**
 * ucoap.h
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 *
 * Acknowledgement:
 *
 * 1) californium   https://github.com/eclipse/californium
 *
 * Aims: Implementation of CoAP client for mcu with ram 1-4 kB.
 *
 */


#ifndef UCOAP_UCOAP_H_
#define UCOAP_UCOAP_H_


#include <stdint.h>
#include <stdbool.h>


#define UCOAP_DEFAULT_VERSION           1
#define UCOAP_CODE(CLASS,CODE)          (int)((CLASS<<5)|CODE)
#define UCOAP_EXTRACT_CLASS(c)          (int)((c)>>5)

#define UCOAP_TCP_URI_SCHEME            "coap+tcp"
#define UCOAP_TCP_SECURE_URI_SCHEME     "coaps+tcp"
#define UCOAP_UDP_URI_SCHEME            "coap"
#define UCOAP_UDP_SECURE_URI_SCHEME     "coaps"

#define UCOAP_TCP_DEFAULT_PORT          5683
#define UCOAP_TCP_DEFAULT_SECURE_PORT   5684
#define UCOAP_UDP_DEFAULT_PORT          5683
#define UCOAP_UDP_DEFAULT_SECURE_PORT   5684


#ifndef UCOAP_RESP_TIMEOUT_MS
#define UCOAP_RESP_TIMEOUT_MS           9000
#endif /* UCOAP_RESP_TIMEOUT_MS */

#ifndef UCOAP_ACK_TIMEOUT_MS
#define UCOAP_ACK_TIMEOUT_MS            5000
#endif /* UCOAP_ACK_TIMEOUT_MS */

#ifndef UCOAP_MAX_RETRANSMIT
#define UCOAP_MAX_RETRANSMIT            3
#endif /* UCOAP_MAX_RETRANSMIT */

#ifndef UCOAP_ACK_RANDOM_FACTOR
#define UCOAP_ACK_RANDOM_FACTOR         130       /* 1.3 -> 130 to rid from float */
#endif /* UCOAP_ACK_RANDOM_FACTOR */

#ifndef UCOAP_MAX_PDU_SIZE
#define UCOAP_MAX_PDU_SIZE              96        /* maximum size of a CoAP PDU */
#endif /* UCOAP_MAX_PDU_SIZE */



typedef enum {

    UCOAP_OK = 0,
    UCOAP_BUSY_ERROR,
    UCOAP_PARAM_ERROR,

    UCOAP_NO_FREE_MEM_ERROR,
    UCOAP_TIMEOUT_ERROR,
    UCOAP_NRST_ANSWER,
    UCOAP_NO_ACK_ERROR,
    UCOAP_NO_RESP_ERROR,

    UCOAP_RX_BUFF_FULL_ERROR,
    UCOAP_WRONG_STATE_ERROR,

    UCOAP_NO_OPTIONS_ERROR,
    UCOAP_WRONG_OPTIONS_ERROR

} ucoap_error;


typedef enum {

    UCOAP_ROUTINE_PACKET_WILL_START = 0,
    UCOAP_ROUTINE_PACKET_DID_FINISH,

    UCOAP_TX_RETR_PACKET,
    UCOAP_TX_ACK_PACKET,

    UCOAP_ACK_DID_RECEIVE,
    UCOAP_NRST_DID_RECEIVE,
    UCOAP_WRONG_PACKET_DID_RECEIVE,

    UCOAP_RESPONSE_BYTE_DID_RECEIVE,
    UCOAP_RESPONSE_TO_LONG_ERROR,
    UCOAP_RESPONSE_DID_RECEIVE

} ucoap_out_signal;


typedef enum {

    UCOAP_UDP = 0,
    UCOAP_TCP,
    UCOAP_SMS

} ucoap_transport;


typedef enum {

    UCOAP_MESSAGE_CON = 0,   /* confirmable message (requires ACK/RST) */
    UCOAP_MESSAGE_NON = 1,   /* non-confirmable message (one-shot message) */
    UCOAP_MESSAGE_ACK = 2,   /* used to acknowledge confirmable messages */
    UCOAP_MESSAGE_RST = 3    /* indicates error in received messages */

} ucoap_udp_message;


typedef enum {

    UCOAP_REQUEST_CLASS = 0,
    UCOAP_SUCCESS_CLASS = 2,
    UCOAP_BAD_REQUEST_CLASS = 4,
    UCOAP_SERVER_ERR_CLASS = 5,

    UCOAP_TCP_SIGNAL_CLASS = 7

} ucoap_class;


typedef enum {

    UCOAP_CODE_EMPTY_MSG = UCOAP_CODE(0, 0),

    UCOAP_REQ_GET = UCOAP_CODE(UCOAP_REQUEST_CLASS, 1),
    UCOAP_REQ_POST = UCOAP_CODE(UCOAP_REQUEST_CLASS, 2),
    UCOAP_REQ_PUT = UCOAP_CODE(UCOAP_REQUEST_CLASS, 3),
    UCOAP_REQ_DEL = UCOAP_CODE(UCOAP_REQUEST_CLASS, 4),

    UCOAP_RESP_SUCCESS_OK_200 = UCOAP_CODE(UCOAP_SUCCESS_CLASS, 0),
    UCOAP_RESP_SUCCESS_CREATED_201 = UCOAP_CODE(UCOAP_SUCCESS_CLASS, 1),
    UCOAP_RESP_SUCCESS_DELETED_202 = UCOAP_CODE(UCOAP_SUCCESS_CLASS, 2),
    UCOAP_RESP_SUCCESS_VALID_203 = UCOAP_CODE(UCOAP_SUCCESS_CLASS, 3),
    UCOAP_RESP_SUCCESS_CHANGED_204 = UCOAP_CODE(UCOAP_SUCCESS_CLASS, 4),
    UCOAP_RESP_SUCCESS_CONTENT_205 = UCOAP_CODE(UCOAP_SUCCESS_CLASS, 5),

    UCOAP_RESP_ERROR_BAD_REQUEST_400 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 0),
    UCOAP_RESP_ERROR_UNAUTHORIZED_401 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 1),
    UCOAP_RESP_BAD_OPTION_402 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 2),
    UCOAP_RESP_FORBIDDEN_403 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 3),
    UCOAP_RESP_NOT_FOUND_404 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 4),
    UCOAP_RESP_METHOD_NOT_ALLOWED_405 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 5),
    UCOAP_RESP_METHOD_NOT_ACCEPTABLE_406 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 6),
    UCOAP_RESP_PRECONDITION_FAILED_412 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 12),
    UCOAP_RESP_REQUEST_ENTITY_TOO_LARGE_413 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 13),
    UCOAP_RESP_UNSUPPORTED_CONTENT_FORMAT_415 = UCOAP_CODE(UCOAP_BAD_REQUEST_CLASS, 15),

    UCOAP_RESP_INTERNAL_SERVER_ERROR_500 = UCOAP_CODE(UCOAP_SERVER_ERR_CLASS, 0),
    UCOAP_RESP_NOT_IMPLEMENTED_501 = UCOAP_CODE(UCOAP_SERVER_ERR_CLASS, 1),
    UCOAP_RESP_BAD_GATEWAY_502 = UCOAP_CODE(UCOAP_SERVER_ERR_CLASS, 2),
    UCOAP_RESP_SERVICE_UNAVAILABLE_503 = UCOAP_CODE(UCOAP_SERVER_ERR_CLASS, 3),
    UCOAP_RESP_GATEWAY_TIMEOUT_504 = UCOAP_CODE(UCOAP_SERVER_ERR_CLASS, 4),
    UCOAP_RESP_PROXYING_NOT_SUPPORTED_505 = UCOAP_CODE(UCOAP_SERVER_ERR_CLASS, 5),

    UCOAP_TCP_SIGNAL_700 = UCOAP_CODE(UCOAP_TCP_SIGNAL_CLASS, 0),
    UCOAP_TCP_SIGNAL_CSM_701 = UCOAP_CODE(UCOAP_TCP_SIGNAL_CLASS, 1),
    UCOAP_TCP_SIGNAL_PING_702 = UCOAP_CODE(UCOAP_TCP_SIGNAL_CLASS, 2),
    UCOAP_TCP_SIGNAL_PONG_703 = UCOAP_CODE(UCOAP_TCP_SIGNAL_CLASS, 3),
    UCOAP_TCP_SIGNAL_RELEASE_704 = UCOAP_CODE(UCOAP_TCP_SIGNAL_CLASS, 4),
    UCOAP_TCP_SIGNAL_ABORT_705 = UCOAP_CODE(UCOAP_TCP_SIGNAL_CLASS, 5)

} ucoap_packet_code;


/**
  * Critical    = (optnum & 1)
  * UnSafe      = (optnum & 2)
  * NoCacheKey  = (optnum & 0x1e) == 0x1c
  *
  */
typedef enum {

    UCOAP_IF_MATCH_OPT         = 1,
    UCOAP_URI_HOST_OPT         = 3,
    UCOAP_ETAG_OPT             = 4,
    UCOAP_IF_NON_MATCH_OPT     = 5,
    UCOAP_URI_PORT_OPT         = 7,
    UCOAP_LOCATION_PATH_OPT    = 8,
    UCOAP_URI_PATH_OPT         = 11,
    UCOAP_CONTENT_FORMAT_OPT   = 12,

    UCOAP_MAX_AGE_OPT          = 14,
    UCOAP_URI_QUERY_OPT        = 15,
    UCOAP_ACCEPT_OPT           = 17,
    UCOAP_LOCATION_QUERY_OPT   = 20,

    UCOAP_BLOCK2_OPT           = 23,  /* blockwise option for GET */
    UCOAP_BLOCK1_OPT           = 27,  /* blockwise option for POST */

    UCOAP_PROXY_URI_OPT        = 35,
    UCOAP_PROXY_SCHEME_OPT     = 39,
    UCOAP_SIZE1_OPT            = 60

} ucoap_option;


typedef enum {

    UCOAP_TEXT_PLAIN = 0,   /* default value */
    UCOAP_TEXT_XML = 1,
    UCOAP_TEXT_CSV = 2,
    UCOAP_TEXT_HTML = 3,
    UCOAP_IMAGE_GIF = 21,
    UCOAP_IMAGE_JPEG = 22,
    UCOAP_IMAGE_PNG = 23,
    UCOAP_IMAGE_TIFF = 24,
    UCOAP_AUDIO_RAW = 25,
    UCOAP_VIDEO_RAW = 26,
    UCOAP_APPLICATION_LINK_FORMAT = 40,
    UCOAP_APPLICATION_XML = 41,
    UCOAP_APPLICATION_OCTET_STREAM = 42,
    UCOAP_APPLICATION_RDF_XML = 43,
    UCOAP_APPLICATION_SOAP_XML = 44,
    UCOAP_APPLICATION_ATOM_XML = 45,
    UCOAP_APPLICATION_XMPP_XML = 46,
    UCOAP_APPLICATION_EXI = 47,
    UCOAP_APPLICATION_FASTINFOSET = 48,
    UCOAP_APPLICATION_SOAP_FASTINFOSET = 49,
    UCOAP_APPLICATION_JSON = 50,
    UCOAP_APPLICATION_X_OBIX_BINARY = 51,
    UCOAP_APPLICATION_CBOR = 60

} ucoap_media_type;


typedef struct ucoap_option_data {

    uint16_t num;
    uint16_t len;
    uint8_t * value;   /* may be string/int/long */

    struct ucoap_option_data * next;

} ucoap_option_data;


typedef struct ucoap_data {

    uint8_t * buf;
    uint32_t len;

} ucoap_data;


typedef struct ucoap_result_data {

    uint8_t resp_code;
    ucoap_data payload;
    ucoap_option_data * options;   /* NULL terminated linked list of options */

} ucoap_result_data;


typedef struct ucoap_request_descriptor {

    uint8_t type;
    uint8_t code;
    uint16_t tkl;

    ucoap_data payload;            /* should not be NULL */
    ucoap_option_data * options;   /* should be NULL if there are no options */

    /**
     * @brief Callback with results of request
     *
     * @param reqd - pointer on the request data (struct 'ucoap_request_descriptor')
     * @param result - pointer on result data (struct 'ucoap_result_data')
     */
    void (* response_callback) (const struct ucoap_request_descriptor * const reqd, const struct ucoap_result_data * const result);

} ucoap_request_descriptor;


typedef struct ucoap_handle {

    const char * name;
    uint16_t transport;

    uint16_t statuses_mask;

    ucoap_data request;
    ucoap_data response;

} ucoap_handle;


/**
 * @brief In this function user should implement a transmission given data via
 *        hardware interface (e.g. serial port)
 *
 */
extern ucoap_error ucoap_tx_data(ucoap_handle * const handle, const uint8_t * buf, const uint32_t len);


/**
 * @brief In this function user should implement a functionality of waiting response.
 *        This function has to return a control when timeout will expired or
 *        when response from server will be received.
 */
extern ucoap_error ucoap_wait_event(ucoap_handle * const handle, const uint32_t timeout_ms);


/**
 * @brief Through this function the 'ucoap' lib will be notifing about events.
 *        See possible events here 'ucoap_out_signal'.
 */
extern ucoap_error ucoap_tx_signal(ucoap_handle * const handle, const ucoap_out_signal signal);


/**
 * @brief In this function user should implement a generating of message id.
 *
 */
extern uint16_t ucoap_get_message_id(ucoap_handle * const handle);


/**
 * @brief In this function user should implement a generating of token.
 *
 */
extern ucoap_error ucoap_fill_token(ucoap_handle * const handle, uint8_t * token, const uint32_t tkl);


/**
 * @brief These functions are using for debug purpose, if user will enable debug mode.
 *
 */
extern void ucoap_debug_print_packet(ucoap_handle * const handle, const char * msg, uint8_t * data, const uint32_t len);
extern void ucoap_debug_print_options(ucoap_handle * const handle, const char * msg, const ucoap_option_data * options);
extern void ucoap_debug_print_payload(ucoap_handle * const handle, const char * msg, const ucoap_data * const payload);


/**
 * @brief In this function user should implement an allocating block of memory.
 *        In simple case it may be a static buffer. The 'UCOAP' will make
 *        two calls of this function before starting work (for rx and tx buffers).
 *        So, you should have minimum two separate blocks of memory.
 *
 */
extern ucoap_error ucoap_alloc_mem_block(uint8_t ** block, const uint32_t min_len);


/**
 * @brief In this function user should implement a freeing mem block.
 *
 */
extern ucoap_error ucoap_free_mem_block(uint8_t * block, const uint32_t min_len);


/**
 * @brief In this function user should implement a copying mem block.
 *
 */
extern void mem_copy(void * dst, const void * src, uint32_t cnt);


/**
 * @brief In this function user should implement a comparing two mem blocks.
 *
 */
extern bool mem_cmp(const void * dst, const void * src, uint32_t cnt);


/**
 * @brief Enable/disable CoAP debug.
 *        Also you should implement 'ucoap_debug_print..' methods in your code.
 *
 */
void ucoap_debug(ucoap_handle * const handle, const bool enable);


/**
 * @brief Send CoAP request to the server
 *
 * @param handle - coap handle
 * @param reqd - descriptor of request
 *
 * @return status of operation
 *
 */
ucoap_error ucoap_send_coap_request(ucoap_handle * const handle, const ucoap_request_descriptor * const reqd);


/**
 * @brief Receive a packet step-by-step (sequence of bytes).
 *        You may to use it if you communicate with server over serial port
 *        or you haven't a free mem for cumulative buffer. Detecting of the
 *        end of packet is a user responsibility (through byte-timeout).
 *
 * @param handle - coap handle
 * @param byte - received byte
 *
 * @return status of operation
 *
 */
ucoap_error ucoap_rx_byte(ucoap_handle * const handle, const uint8_t byte);


/**
 * @brief Receive whole packet
 *
 * @param handle - coap handle
 * @param buf - pointer on buffer with data
 * @param len - length of data
 *
 * @return status of operation
 *
 */
ucoap_error ucoap_rx_packet(ucoap_handle * const handle, const uint8_t * buf, const uint32_t len);


#endif /* _UCOAP_UCOAP_H_ */
