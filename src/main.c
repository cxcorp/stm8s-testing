#include "osconfig.h"
#include <stm8s.h>
#include <os.h>

static void initialize_clock();
//static void initialize_tim1();

static inline void led_init(void);

static inline void led_set(void);
static inline void led_clear(void);
static inline void led_toggle(void);

/*void tim1_interrupt(void) __interrupt(11)
{
	// clear update interrupt flag
	TIM1->SR1 &= ~(TIM1_SR1_UIF);
}*/

OS_SEM(sem, 0, semBinary);

OS_TSK_DEF(sla)
{
	sem_wait(sem);
	led_toggle();
}

OS_TSK_DEF(mas)
{
	tsk_delay(MSEC * 200);
	sem_give(sem);
}


void main()
{
	led_init();
	sys_init();
	/*__asm__("sim");
	initialize_tim1();
	__asm__("rim");*/
	tsk_start(sla);
	tsk_start(mas);
	tsk_stop();
}

#define LED_PORT GPIOB
#define LED_PIN GPIO_PIN_5

static inline void led_init(void)
{
	GPIO_Init(LED_PORT, GPIO_PIN_5, GPIO_MODE_OUT_PP_HIGH_SLOW);
}

static inline void led_set(void)
{
	GPIO_WriteLow(LED_PORT, GPIO_PIN_5);
}
static inline void led_clear(void)
{
	GPIO_WriteHigh(LED_PORT, LED_PIN);
}
static inline void led_toggle(void)
{
	GPIO_WriteReverse(LED_PORT, LED_PIN);
}

/*static void initialize_tim1()
{
	TIM1->PSCRH = 0;
	TIM1->PSCRL = 0; // 1 - frequency division -> timer clock equals system clock = 16MHz

	// enable ARR register buffering through register
	// so when we write our value it's buffered and written
	// as entire 16-bit value
	TIM1->CR1 |= TIM1_CR1_ARPE;
	// count to 16000 (0x3E80)
	TIM1->ARRH = 0x3E;
	TIM1->ARRL = 0x80;

	// clear counter
	TIM1->CNTRH = 0;
	TIM1->CNTRL = 0;

	// enable Update interrupts for the timer
	TIM1->IER |= TIM1_IER_UIE;
	// clear any pending update interrupts
	TIM1->SR1 |= TIM1_SR1_UIF;

	// Set direction; use as down-counter
	// and enable counter
	TIM1->CR1 |= TIM1_CR1_DIR | TIM1_CR1_CEN;
}*/
