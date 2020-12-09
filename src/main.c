#define CPU_FREQUENCY 16000000
#define F_CPU CPU_FREQUENCY
#define CLI __asm__("sim");
#define SEI __asm__("rim");

#include <stm8/stm8s.h>
#include <string.h>
#include "serial.h"
#include "printf.h"
#include "millis.h"
#include "reflex.h"

// if changing, update EXTI interrupt sensitivity
// port in reed_init() and interrupt# in EXTI3_IRQ
#define REED_SENSE_PORT GPIOC
#define REED_SENSE_PIN GPIO_PIN_7
#define REED_PORT_EXTI_MASK EXTI_CR1_PCIS
#define REED_PORT_IRQ 5

#define LED_PORT GPIOB
#define LED_PIN GPIO_PIN_5

static void initialize_clock();
static void initialize_ms_timer();
static void led_init();
static void reed_init() __critical;

void TIM4_UPDATE_IRQ() __interrupt(23)
{
	// clear update interrupt flag
	TIM4->SR1 &= ~TIM4_SR1_UIF;
	_millisMutex = 1;
	_milliseconds++;
	_millisMutex = 0;
}

volatile uint8_t reed_changed = 0;

void EXTI_REED_PORT_IRQ() __interrupt(REED_PORT_IRQ)
{
	// Port change interrupt (reed)
	reed_changed = 1;
}

uint16_t reflex_threshold = 400;

#define UART_RECV_BUFFER_SIZE 32
volatile char uart_recv_buffer[UART_RECV_BUFFER_SIZE];
volatile uint8_t uart_recv_producer_count = 0;
volatile uint8_t uart_recv_consumer_count = 0;

#define UART_CMD_BUFFER_SIZE UART_RECV_BUFFER_SIZE
char uart_cmd_buffer[UART_RECV_BUFFER_SIZE];
uint8_t uart_cmd_buffer_ptr = 0;

void UART1_DATA_FULL_IRQ() __interrupt(18)
{
	if (uart_recv_producer_count - uart_recv_consumer_count == UART_RECV_BUFFER_SIZE)
	{
		// sry buffer full
		return;
	}

	uart_recv_buffer[uart_recv_producer_count % UART_RECV_BUFFER_SIZE] = UART1->DR;
	++uart_recv_producer_count;
}

void main()
{
	initialize_clock();
	initialize_ms_timer();
	Serial_begin();

	__asm__("sim");
	UART1->CR2 |= UART1_CR2_RIEN;
	__asm__("rim");

	reflex_init();
	reed_init();
	led_init();
	Serial_print("\n");
	Serial_println("Hello");

	uint16_t lastDo = 0;
	while (1)
	{
		uint16_t now = millis();
		if (now - lastDo >= 1000)
		{
			lastDo = now;

			uint16_t val = reflex_poll();
			printf("%u\n", val);
		}

		if (reed_changed)
		{
			uint8_t value = !!GPIO_ReadInputPin(REED_SENSE_PORT, REED_SENSE_PIN);
			printf("reed changed: %x\n", value);
			reed_changed = 0;
		}

		if (uart_recv_producer_count - uart_recv_consumer_count != 0)
		{
			// we have data
			char data = uart_recv_buffer[uart_recv_consumer_count % UART_RECV_BUFFER_SIZE];
			++uart_recv_consumer_count;

			uart_cmd_buffer[uart_cmd_buffer_ptr++] = data;
			if (uart_cmd_buffer_ptr >= UART_CMD_BUFFER_SIZE || data == '\n')
			{
				uart_cmd_buffer[UART_CMD_BUFFER_SIZE - 1] = '\n';
				// recv'd line or buffer is full->treat as full line recvd

				// deal with commands
				if (memcmp("T?\n", uart_cmd_buffer, 3) == 0)
				{
					printf("T=%d\n", reflex_threshold);
				}
				else
				{
					Serial_println("COMMANDS:");
					Serial_println("  ?        - Print this help");
					Serial_println("  T=value  - Set reflex sensor threshold value 0-1024");
					Serial_println("  T?       - Read reflex sensor threshold value");
				}
				uart_cmd_buffer_ptr = 0;
			}
		}
	}
}

static void initialize_clock() __critical
{
	// high-speed internal oscillator (HSI) enabled on reset
	CLK->CKDIVR = 0; // 16 MHz
}

static void initialize_ms_timer()
{
	// set prescaler to 2^7=128
	TIM4->PSCR |= 7;
	// we want interrupts every millisecond
	// timer after prescaler = 16_000_000/128 = 125_000
	// so every ms is 125_000/1_000 = 125
	TIM4->ARR = 125;
	// set counter to 0
	TIM4->CNTR = 0;
	// enable update interrupts
	TIM4->IER |= TIM4_IER_UIE;
	// clear any possible interrupts from register
	TIM4->SR1 &= ~TIM4_SR1_UIF;
	// counter enable
	TIM4->CR1 |= TIM4_CR1_CEN;
}

// __critical since we modify interrupts
static void reed_init() __critical
{
	GPIO_Init(REED_SENSE_PORT, REED_SENSE_PIN, GPIO_MODE_IN_FL_IT);
	// Set Port D interrupts to both rising and falling edge
	EXTI->CR1 |= REED_PORT_EXTI_MASK;
}

static void led_init()
{
	GPIO_Init(LED_PORT, LED_PIN, GPIO_MODE_OUT_PP_HIGH_SLOW);
}