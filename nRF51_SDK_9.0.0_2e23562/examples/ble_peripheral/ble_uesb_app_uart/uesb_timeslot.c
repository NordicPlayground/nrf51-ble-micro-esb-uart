#include "uesb_timeslot.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_error.h"
#include "app_scheduler.h"
#include "debug_utils.h"
#include "nrf51.h"
#include "nrf_error.h"
#include "nrf_soc.h"
#include "uesb_error_codes.h"

#define TIMESLOT_BEGIN_IRQn        LPCOMP_IRQn
#define TIMESLOT_BEGIN_IRQHandler  LPCOMP_IRQHandler
#define TIMESLOT_BEGIN_IRQPriority 1
#define TIMESLOT_END_IRQn          QDEC_IRQn
#define TIMESLOT_END_IRQHandler    QDEC_IRQHandler
#define TIMESLOT_END_IRQPriority   1
#define UESB_RX_HANDLE_IRQn        WDT_IRQn
#define UESB_RX_HANDLE_IRQHandler  WDT_IRQHandler
#define UESB_RX_HANDLE_IRQPriority 3

#define MAX_TX_ATTEMPTS 10
  
#define TS_LEN_US                            (2000UL)
#define TX_LEN_EXTENSION_US                  (10000UL)
#define TS_SAFETY_MARGIN_US                  (700UL)    /**< The timeslot activity should be finished with this much to spare. */
#define TS_EXTEND_MARGIN_US                  (1000UL)   /**< The timeslot activity should request an extension this long before end of timeslot. */
#define TS_INTERVAL_US                       (100000UL)


#define MAIN_DEBUG                           0x12345678UL

void RADIO_IRQHandler(void);

static ut_data_handler_t m_evt_handler = 0;
static volatile bool    m_timeslot_session_open;
static uint32_t         m_blocked_cancelled_count;
static fifo_t           m_transmit_fifo;
static uint32_t         m_tx_attempts = 0; 
static uint32_t         m_total_timeslot_length = 0;
static uesb_config_t    m_uesb_config;
static volatile enum
{
    UT_STATE_IDLE, /* Default state */
    UT_STATE_RX,   /* Waiting for packets */
    UT_STATE_TX    /* Trying to transmit packet */
}                       m_ut_state = UT_STATE_IDLE;

/**< This will be used when requesting the first timeslot or any time a timeslot is blocked or cancelled. */
static nrf_radio_request_t m_timeslot_req_earliest = {
        NRF_RADIO_REQ_TYPE_EARLIEST,
        .params.earliest = {
            NRF_RADIO_HFCLK_CFG_FORCE_XTAL,
            NRF_RADIO_PRIORITY_NORMAL,
            TS_LEN_US,
            NRF_RADIO_EARLIEST_TIMEOUT_MAX_US
        }};

///**< This will be used at the end of each timeslot to request the next timeslot. */
//static nrf_radio_request_t m_timeslot_req_normal = {
//        NRF_RADIO_REQ_TYPE_NORMAL,
//        .params.normal = {
//            NRF_RADIO_HFCLK_CFG_FORCE_XTAL,
//            NRF_RADIO_PRIORITY_NORMAL,
//            TS_INTERVAL_US,
//            TS_LEN_US
//        }};

/**< This will be used at the end of each timeslot to request the next timeslot. */
static nrf_radio_signal_callback_return_param_t m_rsc_return_sched_next = {
        NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END,
        .params.request = {
                (nrf_radio_request_t*) &m_timeslot_req_earliest
        }};

/**< This will be used at the end of each timeslot to request an extension of the timeslot. */
static nrf_radio_signal_callback_return_param_t m_rsc_extend = {
        NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND,
        .params.extend = {TX_LEN_EXTENSION_US}
        };

/**< This will be used at the end of each timeslot to request the next timeslot. */
static nrf_radio_signal_callback_return_param_t m_rsc_return_no_action = {
        NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE,
        .params.request = {NULL}
        };

        
/**@brief   Function for handling timeslot events.
 */
static nrf_radio_signal_callback_return_param_t * radio_callback (uint8_t signal_type)
{   
    // NOTE: This callback runs at lower-stack priority (the highest priority possible).
    switch (signal_type) {
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START:
        // TIMER0 is pre-configured for 1Mhz.
        NRF_TIMER0->TASKS_STOP          = 1;
        NRF_TIMER0->TASKS_CLEAR         = 1;
        NRF_TIMER0->MODE                = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
        NRF_TIMER0->EVENTS_COMPARE[0]   = 0;
        NRF_TIMER0->EVENTS_COMPARE[1]   = 0;
        NRF_TIMER0->INTENSET            = (TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos) | 
                                          (TIMER_INTENSET_COMPARE1_Set << TIMER_INTENSET_COMPARE1_Pos);
        NRF_TIMER0->CC[0]               = (TS_LEN_US - TS_SAFETY_MARGIN_US);
        NRF_TIMER0->CC[1]               = (TS_LEN_US - TS_EXTEND_MARGIN_US);
        NRF_TIMER0->BITMODE             = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
        NRF_TIMER0->TASKS_START         = 1;

        NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);

        NVIC_EnableIRQ(TIMER0_IRQn);
        break;
    
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:
        if (NRF_TIMER0->EVENTS_COMPARE[0] &&
           (NRF_TIMER0->INTENSET & (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENCLR_COMPARE0_Pos)))
        {
            NRF_TIMER0->TASKS_STOP  = 1;
            NRF_TIMER0->EVENTS_COMPARE[0] = 0;
            
            // This is the "timeslot is about to end" timeout

            // Disabling UESB is done in a lower interrupt priority 
            NVIC_SetPendingIRQ(TIMESLOT_END_IRQn);
            
            // Schedule next timeslot
            return (nrf_radio_signal_callback_return_param_t*) &m_rsc_return_sched_next;
        }

        if (NRF_TIMER0->EVENTS_COMPARE[1] &&
           (NRF_TIMER0->INTENSET & (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENCLR_COMPARE1_Pos)))
        {
            NRF_TIMER0->EVENTS_COMPARE[1] = 0;
            
            // This is the "try to extend timeslot" timeout
            
            if (m_total_timeslot_length < (128000000UL - 1UL - TX_LEN_EXTENSION_US))
            {
                // Request timeslot extension if total length does not exceed 128 seconds
                return (nrf_radio_signal_callback_return_param_t*) &m_rsc_extend;
            }
            else
            {
                // Return with no action request
                return (nrf_radio_signal_callback_return_param_t*) &m_rsc_return_no_action;
            }
        }
        
        
        
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO:
        // Call the uesb IRQHandler
        RADIO_IRQHandler();
        break;
    
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED:
        // Don't do anything. Our timer will expire before timeslot ends
        return (nrf_radio_signal_callback_return_param_t*) &m_rsc_return_no_action;
    
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED:
        // Extension succeeded: update timer
        NRF_TIMER0->TASKS_STOP          = 1;
        NRF_TIMER0->EVENTS_COMPARE[0]   = 0;
        NRF_TIMER0->EVENTS_COMPARE[1]   = 0;
        NRF_TIMER0->CC[0]               += (TX_LEN_EXTENSION_US - 25);
        NRF_TIMER0->CC[1]               += (TX_LEN_EXTENSION_US - 25);
        NRF_TIMER0->TASKS_START         = 1;
    
        // Keep track of total length
        m_total_timeslot_length += TX_LEN_EXTENSION_US;
    
        // UESB packet receiption and transmission are synchronized at the beginning of timeslot extensions. 
        // Ideally we would also transmit at the beginning of the initial timeslot, not only extensions,
        // but this is to simplify a bit. 
        NVIC_SetPendingIRQ(TIMESLOT_BEGIN_IRQn);
        break;
    
    default:
        app_error_handler(MAIN_DEBUG, __LINE__, (const uint8_t*)__FILE__);
        break;
    };

    // Fall-through return: return with no action request
    return (nrf_radio_signal_callback_return_param_t*) &m_rsc_return_no_action;
}

/**@brief IRQHandler used for execution context management. 
  *        Any available handler can be used as we're not using the associated hardware.
  *        This handler is used to stop and disable UESB
  */
void TIMESLOT_END_IRQHandler(void)
{
    uint32_t err_code;
    
    // Timeslot is about to end: stop UESB
    
    err_code = uesb_stop_rx();
    if (err_code != UESB_SUCCESS)
    {
        // Override
        NRF_RADIO->INTENCLR      = 0xFFFFFFFF;
        NRF_RADIO->TASKS_DISABLE = 1;
    }
    
    uesb_disable();
    
    m_total_timeslot_length = 0;
}

/**@brief IRQHandler used for execution context management. 
  *        Any available handler can be used as we're not using the associated hardware.
  *        This handler is used to initiate UESB RX/TX
  */
void TIMESLOT_BEGIN_IRQHandler(void)
{
    uesb_payload_t payload;
    uint32_t       payload_len;
    uint32_t       err_code;
    
    uesb_init(&m_uesb_config);
    
    // Packet transmission is syncrhonized to the beginning of timeslots
    // Check FIFO for packets and transmit if not empty
    if (m_transmit_fifo.free_items < sizeof(m_transmit_fifo.buf) && m_ut_state != UT_STATE_TX)
    {
        // There are packets in the FIFO: Start transmitting
        payload_len = sizeof(payload);
        
        // Copy packet from FIFO. Packet isn't removed until transmissions succeeds or max retries has been exceeded
        if (m_tx_attempts < MAX_TX_ATTEMPTS)
        {        
            fifo_peek_pkt(&m_transmit_fifo, (uint8_t *) &payload, &payload_len);
            APP_ERROR_CHECK_BOOL(payload_len == sizeof(payload));
        }
        else
        {
            fifo_get_pkt(&m_transmit_fifo, (uint8_t *) &payload, &payload_len);
            APP_ERROR_CHECK_BOOL(payload_len == sizeof(payload));
            
            m_tx_attempts = 0;
        }
        
        if (m_ut_state == UT_STATE_RX)
        {
            uesb_stop_rx();
        }
        
        err_code = uesb_write_tx_payload(&payload);
        APP_ERROR_CHECK(err_code);
        
        m_ut_state = UT_STATE_TX;
    }
    else
    {
        // No packets in the FIFO: start reception
        err_code = uesb_start_rx();
        m_ut_state = UT_STATE_RX;

    }
}

/**@brief IRQHandler used for execution context management. 
  *        Any available handler can be used as we're not using the associated hardware.
  *        This handler is used to notify of received data
  */
void UESB_RX_HANDLE_IRQHandler(void)
{
    uesb_payload_t payload;
    uint32_t       err_code;

    // Get packet from UESB buffer
    uesb_read_rx_payload(&payload);

    // Give packet to main application via the scheduler
    err_code = app_sched_event_put(payload.data, payload.length, m_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
void ut_on_sys_evt(uint32_t sys_evt)
{
    switch(sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        case NRF_EVT_FLASH_OPERATION_ERROR:
            break;
        case NRF_EVT_RADIO_BLOCKED:
        case NRF_EVT_RADIO_CANCELED:
        {
            // Blocked events are rescheduled with normal priority. They could also
            // be rescheduled with high priority if necessary.
            uint32_t err_code = sd_radio_request((nrf_radio_request_t*) &m_timeslot_req_earliest);
            APP_ERROR_CHECK(err_code);

            m_blocked_cancelled_count++;
            
            break;
        }
        case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
            DEBUG_PRINT("NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN\r\n");
            app_error_handler(MAIN_DEBUG, __LINE__, (const uint8_t*)__FILE__);
            break;
        case NRF_EVT_RADIO_SESSION_CLOSED:
            {
                m_timeslot_session_open = false;
                
                DEBUG_PRINT("NRF_EVT_RADIO_SESSION_CLOSED\r\n");
            }
        
            break;
        case NRF_EVT_RADIO_SESSION_IDLE:
        {
            DEBUG_PRINT("NRF_EVT_RADIO_SESSION_IDLE\r\n");
            
            uint32_t err_code = sd_radio_session_close();
            APP_ERROR_CHECK(err_code);
            break;
        }
        default:
            // No implementation needed.
            break;
    }
}

/**@brief   Function for receiving callbacks from the micro-esb library.
 */
static void uesb_event_handler(void)
{
    uint32_t rf_interrupts;

    uesb_get_clear_interrupts(&rf_interrupts);

    if(rf_interrupts & UESB_INT_TX_FAILED_MSK)
    {
        // Transmit failed: flush buffer
        uesb_flush_tx();

        m_tx_attempts += 1;
        m_ut_state     = UT_STATE_RX;
    }
    if (rf_interrupts & UESB_INT_TX_SUCCESS_MSK)
    {
        uesb_payload_t payload;
        uint32_t       payload_len;
        
        // Successful transmission. Can now remove packet from our FIFO
        payload_len = sizeof(payload);
        fifo_get_pkt(&m_transmit_fifo, (uint8_t *) &payload, &payload_len);
        APP_ERROR_CHECK_BOOL(payload_len == sizeof(payload));
        
        m_tx_attempts = 0;
        m_ut_state    = UT_STATE_RX;
    }
    if(rf_interrupts & UESB_INT_RX_DR_MSK)
    {
        // Data reception is handled in a lower priority interrupt
        NVIC_SetPendingIRQ(UESB_RX_HANDLE_IRQn);
    }
}

uint32_t ut_init(ut_data_handler_t evt_handler)
{
    uesb_config_t tmp_config = UESB_DEFAULT_CONFIG;
    m_evt_handler = evt_handler;
    
    memcpy(&m_uesb_config, &tmp_config, sizeof(uesb_config_t));
    
    m_uesb_config.retransmit_count   = 2;
    m_uesb_config.event_handler      = uesb_event_handler;
    m_uesb_config.radio_irq_priority = 0; // Needs to match softdevice priority
    
    fifo_init(&m_transmit_fifo);
    
    // Using three avilable interrupt handlers for interrupt level management
    // These can be any available IRQ as we're not using any of the hardware,
    // simply triggering them through software
    NVIC_ClearPendingIRQ(TIMESLOT_END_IRQn);
    NVIC_SetPriority(TIMESLOT_END_IRQn, 1);
    NVIC_EnableIRQ(TIMESLOT_END_IRQn);
    
    NVIC_ClearPendingIRQ(TIMESLOT_BEGIN_IRQn);
    NVIC_SetPriority(TIMESLOT_BEGIN_IRQn, 1);
    NVIC_EnableIRQ(TIMESLOT_BEGIN_IRQn);
    
    NVIC_ClearPendingIRQ(UESB_RX_HANDLE_IRQn);
    NVIC_SetPriority(UESB_RX_HANDLE_IRQn, 1);
    NVIC_EnableIRQ(UESB_RX_HANDLE_IRQn);
    
    return NRF_SUCCESS;
}

uint32_t ut_start(uint32_t rf_channel, uint8_t rf_address[5])
{
    uint32_t        err_code;
    
    if (m_timeslot_session_open)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    m_uesb_config.rf_channel = rf_channel;
    memcpy(m_uesb_config.rx_address_p0, rf_address, 5);
    
    m_blocked_cancelled_count  = 0;

    err_code = sd_radio_session_open(radio_callback);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = sd_radio_request(&m_timeslot_req_earliest);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    m_timeslot_session_open = true;
    
    return NRF_SUCCESS;
}

uint32_t ut_send_str(uint8_t * p_str, uint32_t length)
{
    uesb_payload_t payload;
    
    if (m_transmit_fifo.free_items < sizeof(payload))
    {
        return NRF_ERROR_NO_MEM;
    }
    
    memset(&payload, 0, sizeof(payload));
    memcpy(payload.data, p_str, length);
    payload.length = length;
    payload.pipe   = UESB_TIMESLOT_PIPE;
    
    while (m_ut_state == UT_STATE_TX)
    {
        // Wait until previous transmission has finished.
        // This is to simplify design to avoid data integrity issues
        // Possibly at a small power penalty
        __NOP();
    }
    
    fifo_put_pkt(&m_transmit_fifo, (uint8_t *)&payload, sizeof(payload));
    
    return NRF_SUCCESS;
}
    
uint32_t ut_stop()
{
    return NRF_ERROR_NOT_SUPPORTED;
}
