#include "debug_utils.h"

#if defined(DEBUG) || defined(DEBUG_VERBOSE)

static uart_rx_handler_t m_rx_evt_handler;

void HardFault_Handler(void)
{
    uint32_t *sp = (uint32_t *) __get_MSP();
    uint32_t ia = sp[24/4];
    DEBUG_PRINT("Hard Fault at address: 0x%08x\r\n", (unsigned int)ia);
    while(1)
        ;
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    __disable_irq();
    
    DEBUG_PRINT("ASSERT\r\nCode: 0x%08x\r\nLine: %d\r\nFile: %s\r\n", (unsigned int)error_code, (unsigned int)line_num, p_file_name);
    
    while (true)
    {
        __NOP();
    }
}

uint32_t current_time_get(void)
{
    uint32_t cc0, cc1;
    
    NRF_TIMER2->TASKS_CAPTURE[1] = 1;
    NRF_TIMER1->TASKS_CAPTURE[0] = 1;
    
    cc0 = NRF_TIMER1->CC[0];
    cc1 = NRF_TIMER2->CC[1];
    
    return (cc1+(65535*cc0));
}

void current_time_start(void)
{
    NRF_TIMER2->TASKS_STOP  = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->BITMODE     = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER2->PRESCALER   = 4;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->CC[0]       = 65535;
    NRF_TIMER2->SHORTS      = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
    
    
    NRF_TIMER1->MODE = TIMER_MODE_MODE_Counter << TIMER_MODE_MODE_Pos;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->TASKS_START = 1;
    
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TIMER1->TASKS_COUNT;
    NRF_PPI->CHENSET = PPI_CHENSET_CH0_Msk;
    
    NRF_TIMER2->TASKS_START = 1;
}

#if !defined(__ICCARM__)
struct __FILE 
{
    int handle;
};
#endif

FILE __stdout;
FILE __stdin;


#if defined(__CC_ARM) ||  defined(__ICCARM__)
int fgetc(FILE * p_file)
{
    return 0;
}


int fputc(int ch, FILE * p_file)
{
    NRF_UART0->EVENTS_TXDRDY = 0;
    NRF_UART0->TXD = ch;
    while (NRF_UART0->EVENTS_TXDRDY == 0)
    {
        __NOP();
    }
    
    return ch;
}
#elif defined(__GNUC__)


int _write(int file, const char * p_char, int len)
{
    for (int i = 0; i < len; ++i)
    {
        NRF_UART0->EVENTS_TXDRDY = 0;
        NRF_UART0->TXD = p_char[i];
        while (NRF_UART0->EVENTS_TXDRDY == 0)
        {
            __NOP();
        }
    }
    
    return len;
}


int _read(int file, char * p_char, int len)
{
    return 1;
}
#endif

void UART0_IRQHandler(void)
{
    NRF_UART0->EVENTS_RXDRDY = 0;
    m_rx_evt_handler(NRF_UART0->RXD);
}

void uart_init(uart_rx_handler_t p_rx_handler)
{
    m_rx_evt_handler = p_rx_handler;
    
    NRF_UART0->PSELTXD = TX_PIN_NUMBER;
    NRF_UART0->PSELRXD = RX_PIN_NUMBER;

    NRF_UART0->BAUDRATE      = (UART_BAUDRATE_BAUDRATE_Baud1M << UART_BAUDRATE_BAUDRATE_Pos);
    NRF_UART0->CONFIG        = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
    NRF_UART0->ENABLE        = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    NRF_UART0->INTENSET      = (UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos);
    
    NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_SetPriority(UART0_IRQn, 3);
    NVIC_EnableIRQ(UART0_IRQn);
    
    NRF_UART0->TASKS_STARTTX = 1;
    NRF_UART0->TASKS_STARTRX = 1;
    NRF_UART0->EVENTS_RXDRDY = 0;
    
    DEBUG_PRINT("UART initialized\r\n");
}
#endif /* DEBUG */
