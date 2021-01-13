#define CPU_FREQUENCY 16000000
#define F_CPU CPU_FREQUENCY
#define CLI __asm__("sim");
#define SEI __asm__("rim");

#include <stm8/stm8s.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "serial.h"
#include "printf.h"
#include "millis.h"
#include "reflex.h"
#include "persistent_config.h"

// if changing, update EXTI interrupt sensitivity
// port in reed_init() and interrupt# in EXTI3_IRQ
#define REED_SENSE_PORT GPIOC
#define REED_SENSE_PIN GPIO_PIN_7
#define REED_PORT_EXTI_MASK EXTI_CR1_PCIS
#define REED_PORT_IRQ 5

#define INDICATOR_LED_PORT GPIOB
#define INDICATOR_LED_PIN GPIO_PIN_5
#define MOSFET_PORT GPIOC
#define MOSFET_PIN GPIO_PIN_6

static void initialize_clock();
static void initialize_ms_timer();
static void led_init();
static void mosfet_init();
static void reed_init() __critical;

static void uartcmd_update();
static void uartcmd_execute();
static void uartcmd_print_help();

void TIM4_UPDATE_IRQ() __interrupt(23)
{
	// clear update interrupt flag
	TIM4->SR1 &= ~TIM4_SR1_UIF;
	_millisMutex = 1;
	_milliseconds++;
	_millisMutex = 0;
}

volatile uint8_t reed_changed = 0;
uint8_t reed_is_closed;

void EXTI_REED_PORT_IRQ() __interrupt(REED_PORT_IRQ)
{
	// Port change interrupt (reed)
	reed_changed = 1;
}

// sync with persistent_config if change
uint16_t reflex_threshold;

uint16_t reflex_current = 0;

uint8_t reflex_value_printing_enabled = 0;

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

#define REFLEX_POLL_DELAY 50
#define REFLEX_PRINT_DELAY 1000

void main()
{
	initialize_clock();
	initialize_ms_timer();
	Serial_begin();

	__asm__("sim");
	UART1->CR2 |= UART1_CR2_RIEN;
	__asm__("rim");

	reflex_threshold = config_read_reflex_threshold();

	reflex_init();
	reed_init();
	led_init();
	mosfet_init();
	Serial_print("\n");
	Serial_println("Hello");

	uint16_t lastReflexPoll = 0;
	uint16_t lastReflexPrint = 0;
	while (1)
	{
		uint16_t now = millis();
		if (now - lastReflexPoll >= REFLEX_POLL_DELAY)
		{
			lastReflexPoll = now;
			reflex_current = reflex_poll();
		}
		if (now - lastReflexPrint >= REFLEX_PRINT_DELAY && reflex_value_printing_enabled)
		{
			lastReflexPrint = now;
			printf("Reflex = %u\n", reflex_current);
		}

		if (reed_changed)
		{
			reed_is_closed = !!GPIO_ReadInputPin(REED_SENSE_PORT, REED_SENSE_PIN);
			printf("reed_is_closed: %x\n", reed_is_closed);
			reed_changed = 0;
		}

		if (reed_is_closed && reflex_current >= reflex_threshold)
		{
			GPIO_WriteHigh(INDICATOR_LED_PORT, INDICATOR_LED_PIN);
			GPIO_WriteHigh(MOSFET_PORT, MOSFET_PIN);
		}
		else
		{
			GPIO_WriteLow(INDICATOR_LED_PORT, INDICATOR_LED_PIN);
			GPIO_WriteLow(MOSFET_PORT, MOSFET_PIN);
		}

		uartcmd_update();
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

	reed_is_closed = !!GPIO_ReadInputPin(REED_SENSE_PORT, REED_SENSE_PIN);
}

static void led_init()
{
	GPIO_Init(INDICATOR_LED_PORT, INDICATOR_LED_PIN, GPIO_MODE_OUT_PP_HIGH_SLOW);
}

static void mosfet_init()
{
	GPIO_Init(MOSFET_PORT, MOSFET_PIN, GPIO_MODE_OUT_PP_HIGH_SLOW);
}

static void uartcmd_update()
{
	if (uart_recv_producer_count - uart_recv_consumer_count == 0)
	{
		return;
	}

	// we have data
	char data = uart_recv_buffer[uart_recv_consumer_count % UART_RECV_BUFFER_SIZE];
	++uart_recv_consumer_count;

	uart_cmd_buffer[uart_cmd_buffer_ptr++] = data;

	if (uart_cmd_buffer_ptr >= UART_CMD_BUFFER_SIZE || data == '\n')
	{
		if (uart_cmd_buffer_ptr >= UART_CMD_BUFFER_SIZE)
		{
			// buffer is full -> treat as a line
			uart_cmd_buffer[UART_CMD_BUFFER_SIZE - 1] = '\n';
		}

		uartcmd_execute();
		uart_cmd_buffer_ptr = 0;
	}
}

static void uartcmd_execute()
{
	// deal with commands
	if (memcmp("RXT?\n", uart_cmd_buffer, 5) == 0)
	{
		printf("RXT=%d\n", reflex_threshold);
		return;
	}

	if (memcmp("RXT=", uart_cmd_buffer, 4) == 0)
	{
		const uint8_t cmdLen = 4;
		uint8_t numbers[5] = {0};

		for (uint8_t ptr = 0; ptr < 4 && uart_cmd_buffer[ptr + cmdLen] != '\n'; ptr++)
		{
			if (!isdigit(uart_cmd_buffer[ptr + cmdLen]))
			{
				// oops not a number!
				uartcmd_print_help();
				return;
			}
			numbers[ptr] = uart_cmd_buffer[ptr + cmdLen];
		}
		reflex_threshold = (uint16_t)atoi(numbers);
		config_write_reflex_threshold(reflex_threshold);

		printf("T=%d\n", reflex_threshold);
		return;
	}

	if (memcmp("RX\n", uart_cmd_buffer, 3) == 0)
	{
		reflex_value_printing_enabled = !reflex_value_printing_enabled;
		printf("RX=%u\n", reflex_value_printing_enabled);
		return;
	}

	// no match
	uartcmd_print_help();
}

static void uartcmd_print_help()
{
	Serial_println("COMMANDS:");
	Serial_println("  ?          - Print this help");
	Serial_println("  RX         - Toggle reflex sensor value printing");
	Serial_println("  RXT=value  - Set reflex sensor threshold value 0-1024");
	Serial_println("  RXT?       - Read reflex sensor threshold value");
}