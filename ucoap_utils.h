// Copyright 2023 Vahid Mardani
/*
 * This file is part of ucoap.
 *  ucoap is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  ucoap is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with ucoap. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#ifndef _UCOAP_UCOAP_UTILS_H_
#define _UCOAP_UCOAP_UTILS_H_


#include "ucoap.h"


#define UCOAP_CHECK_STATUS(h,s)      ((h)->statuses_mask & (s))
#define UCOAP_SET_STATUS(h,s)        ((h)->statuses_mask |= (s))
#define UCOAP_RESET_STATUS(h,s)      ((h)->statuses_mask &= ~(s))

#define UCOAP_CHECK_RESP(m,s)        ((m) & (s))
#define UCOAP_SET_RESP(m,s)          ((m) |= (s))
#define UCOAP_RESET_RESP(m,s)        ((m) = ~(s))



typedef enum {

     UCOAP_UNKNOWN         = (int) 0x0000,
     UCOAP_ALL_STATUSES    = (int) 0xffff,

     UCOAP_SENDING_PACKET  = (int) 0x0001,
     UCOAP_WAITING_RESP    = (int) 0x0002,

     UCOAP_DEBUG_ON        = (int) 0x0080

} ucoap_handle_status;


typedef enum {

    UCOAP_RESP_EMPTY            = (int) 0x00000000,

    UCOAP_RESP_ACK              = (int) 0x00000001,
    UCOAP_RESP_PIGGYBACKED      = (int) 0x00000002,
    UCOAP_RESP_NRST             = (int) 0x00000004,
    UCOAP_RESP_SEPARATE         = (int) 0x00000008,

    UCOAP_RESP_SUCCESS_CODE     = (int) 0x00000010,
    UCOAP_RESP_FAILURE_CODE     = (int) 0x00000020,
    UCOAP_RESP_TCP_SIGNAL_CODE  = (int) 0x00000020,

    UCOAP_RESP_NEED_SEND_ACK    = (int) 0x00000100,

    UCOAP_RESP_INVALID_PACKET   = (int) 0x80000000

} ucoap_parsing_result;



/**
 * @brief Encoding options and add it to the packet
 *
 * @param buf - pointer on packet buffer
 * @param option - pointer on first element of linked list of options.
          Must not be NULL.
 *
 * @return length of data that was added to the buffer
 */
uint32_t encoding_options(uint8_t * const buf,
        const ucoap_option_data * option);


/**
 * @brief Decoding options from response
 *
 * @param response - incoming packet
 * @param option - pointer on first element of linked list
 * @param opt_start_idx - index of options in the incoming packet
 * @param payload_start_idx - pointer on variable for storing idx of payload
          in the incoming packet
 *
 * @return status of operations
 */
enum ucoap_error
decoding_options(const ucoap_data * const response,
        ucoap_option_data * option,
        const uint32_t opt_start_idx,
        uint32_t * const payload_start_idx);


/**
 * @brief Add payload to the packet
 *
 * @param buf - pointer on packet buffer
 * @param payload - data with payload
 *
 * @return length of data that was added to the buffer
 */
uint32_t fill_payload(uint8_t * const buf, const ucoap_data * const payload);


#endif /* _UCOAP_UCOAP_UTILS_H_ */
