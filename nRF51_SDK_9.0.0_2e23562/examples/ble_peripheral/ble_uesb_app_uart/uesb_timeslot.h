#ifndef __UESB_TIMESLOT_H__
#define __UESB_TIMESLOT_H__

#include <stdbool.h>
#include <stdint.h>

#include "micro_esb.h"

#define UESB_TIMESLOT_MAX_PKT_LEN UESB_CORE_MAX_PAYLOAD_LENGTH
#define UESB_TIMESLOT_PIPE        0
// Max retries should be a function of packet length, ack delay, and timeslot size
// A conservative estimate for 100 byte packets would be 1 ms per pkt+ack
#define UESB_NUM_RETRIES          3

/**@brief Communication identifiers */
typedef struct
{
    uint8_t  rf_chn;     /* Packet-level RF channel. Must be the same for both transmitter and receiver */
    uint8_t  rf_addr[5]; /* Packet-level RF address. Must be the same for both transmitter and receiver */
} ut_access_data_t;

/**@brief Data handler type. */
typedef void (*ut_data_handler_t)(void * p_data, uint16_t length);

/**@brief Initialize UESB Timeslot library 
 *
 * @note app_scheduler is used to execute the event handler, and must be initialized before calling @ref ut_start
 *
 * @param[in] evt_handler Event handler for received data
 * @retval NRF_SUCCESS
 * @retval NRF_INVALID_PARAM
 */
uint32_t ut_init(ut_data_handler_t evt_handler);

uint32_t ut_access_data_get(ut_access_data_t * p_access_data);
uint32_t ut_access_data_set(ut_access_data_t * p_access_data);

/**@brief SoftDevice system event handler. Must be called when a system event occurs */
void ut_on_sys_evt(uint32_t sys_evt);

/**@brief Start requesting timeslots to run micro-ESB
 *
 * @param[in] rf_channel RF channel to use [0-80]. Must be the same for both devices.
 * @param[in] rf_address Packet address to use. Must be the same for both devices.
 *
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_INVALID_STATE
 */
uint32_t ut_start(void);

/**@brief Send string via micro-ESB
 *
 * @note Function blocks until previous transmission has finished
 * @details String is put into internal buffer. Transmission will be started at the beginning of the next timeslot or timeslot extension.
 * @param[in] p_str  String
 * @param[in] length String length
 *
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_NO_MEM
 */
uint32_t ut_send_str(uint8_t * p_str, uint32_t length);

/**@brief Close the timeslot session
 *
 * @retval NRF_ERROR_NOT_SUPPORTED
 */
uint32_t ut_stop(void);


#endif /* __UESB_TIMESLOT_H__ */
