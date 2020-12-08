#include "serial.h"

void Serial_begin()
{
    // Set baudrate
    // BRR2 must be written to before BRR1 as the value
    // is latched after write to BRR1
    // 115200 baud - values from STM8S reference manual
    UART1->BRR2 = 0x0B;
    UART1->BRR1 = 0x08;
    UART1->CR1 &= (uint8_t)(~UART1_CR1_UARTD);

    // word length: 1 start bit, 8 data bits, n stop bit
    UART1->CR1 &= ~UART1_CR1_M;
    // 0b00: 1 stop bit
    UART1->CR3 &= ~UART1_CR3_STOP;

    // enable UART TX
    UART1->CR2 |= UART1_CR2_TEN;
    // enable RX
    UART1->CR2 |= UART1_CR2_REN;
}

void Serial_write(const uint8_t value)
{
    // wait until previous data register contents
    // have been latched to the shift register
    while ((UART1->SR & UART1_SR_TXE) == 0)
        ;
    // write
    UART1->DR = value;
}

void _putchar(char character)
{
    while ((UART1->SR & UART1_SR_TXE) == 0)
        ;
    UART1->DR = character;
}

void Serial_print(const char *str)
{
    for (; *str != '\0'; str++)
    {
        Serial_write(*str);
    }
}

void Serial_println(const char *str)
{
    Serial_print(str);
    Serial_write('\n');
}

/** blocking */
char Serial_readchar()
{
    // wait until stuff received
    while ((UART1->SR & UART1_SR_RXNE) == 0)
        ;
    return (char)UART1->DR;
}

/**
 * blocking, treats \n as line separator, returns bytes read,
 * writes \0 to buffer
 * does not check that buffer_size > 0 so segfault if buffer_size = 0
 */
uint16_t Serial_nreadline(char *buffer, uint16_t buffer_size)
{
    uint16_t written = 0;
    char *bufEnd = buffer + buffer_size;
    for (; buffer != bufEnd; buffer++)
    {
        written++;
        char c = Serial_readchar();
        if (c == '\n')
        {
            *buffer = '\0';
            return written;
        }
        *buffer = c;
    }

    *(bufEnd - 1) = '\0';
    written++;

    // didn't reach newline and buffer is full -> read until newline
    // to flush the UART buffer
    char c;
    while ((c = Serial_readchar()) != '\n')
        ;

    return written;
}
