#ifndef TIMESLOT_H__
#define TIMESLOT_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
//#include "softdevice_handler.h"
#include "boards.h"


#include "nrf_esb.h"


typedef void (*ut_data_handler_t)(void * p_data, uint16_t length);


/**@brief Radio event handler
*/
void RADIO_timeslot_IRQHandler(void);


/**@brief Request next timeslot event in earliest configuration
 */
uint32_t request_next_event_earliest(void);


/**@brief Configure next timeslot event in earliest configuration
 */
void configure_next_event_earliest(void);


/**@brief Configure next timeslot event in normal configuration
 */
void configure_next_event_normal(void);


/**@brief Timeslot signal handler
 */
void nrf_evt_signal_handler(uint32_t evt_id);


/**@brief Timeslot event handler
 */
nrf_radio_signal_callback_return_param_t * radio_callback(uint8_t signal_type);


/**@brief Function for initializing.
 */
uint32_t esb_timeslot_init(ut_data_handler_t evt_handler);


/**@brief Function for starting the timeslot API.
 */
uint32_t esb_timeslot_sd_start(void);


/**@brief Function for stopping the timeslot API.
 */
uint32_t esb_timeslot_sd_stop(void);


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
uint32_t esb_timeslot_send_str(uint8_t * p_str, uint32_t length);

#endif  // TIMESLOT_H__
