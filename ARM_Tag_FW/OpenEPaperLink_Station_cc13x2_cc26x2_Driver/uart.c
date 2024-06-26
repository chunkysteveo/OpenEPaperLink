#include "uart.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ti/drivers/UART2.h>
#include "ti_drivers_config.h"
#include <stdarg.h>

#define MAX_LENGTH1             256
char         input[MAX_LENGTH1];
size_t bytesToRead = MAX_LENGTH1;
volatile size_t bytesReadCount;

UART2_Handle uart;

static void printchar(char **str, int c) {
    if (str) {
        **str = c;
        ++(*str);
    } else
    {
        uartWrite((uint8_t *)&c, 1);
    }
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, const char *string, int width, int pad) {
    register int pc = 0, padchar = ' ';

    if (width > 0) {
        register int len = 0;
        register const char *ptr;
        for (ptr = string; *ptr; ++ptr)
            ++len;
        if (len >= width)
            width = 0;
        else
            width -= len;
        if (pad & PAD_ZERO)
            padchar = '0';
    }
    if (!(pad & PAD_RIGHT)) {
        for (; width > 0; --width) {
            printchar(out, padchar);
            ++pc;
        }
    }
    for (; *string; ++string) {
        printchar(out, *string);
        ++pc;
    }
    for (; width > 0; --width) {
        printchar(out, padchar);
        ++pc;
    }

    return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad,
        int letbase) {
    char print_buf[PRINT_BUF_LEN];
    register char *s;
    register int t, neg = 0, pc = 0;
    register unsigned int u = i;

    if (i == 0) {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return prints(out, print_buf, width, pad);
    }

    if (sg && b == 10 && i < 0) {
        neg = 1;
        u = -i;
    }

    s = print_buf + PRINT_BUF_LEN - 1;
    *s = '\0';

    while (u) {
        t = u % b;
        if (t >= 10)
            t += letbase - '0' - 10;
        *--s = t + '0';
        u /= b;
    }

    if (neg) {
        if (width && (pad & PAD_ZERO)) {
            printchar(out, '-');
            ++pc;
            --width;
        } else {
            *--s = '-';
        }
    }

    return pc + prints(out, s, width, pad);
}

static int print(char **out, const char *format, va_list args) {
    register int width, pad;
    register int pc = 0;
    char scr[2];

    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;
            width = pad = 0;
            if (*format == '\0')
                break;
            if (*format == '%')
                goto out;
            if (*format == '-') {
                ++format;
                pad = PAD_RIGHT;
            }
            while (*format == '0') {
                ++format;
                pad |= PAD_ZERO;
            }
            for (; *format >= '0' && *format <= '9'; ++format) {
                width *= 10;
                width += *format - '0';
            }
            if (*format == 's') {
                register char *s = (char *) va_arg( args, int );
                pc += prints(out, s ? s : "(null)", width, pad);
                continue;
            }
            if (*format == 'd') {
                pc += printi(out, va_arg( args, int ), 10, 1, width, pad, 'a');
                continue;
            }
            if (*format == 'x') {
                pc += printi(out, va_arg( args, int ), 16, 0, width, pad, 'a');
                continue;
            }
            if (*format == 'X') {
                pc += printi(out, va_arg( args, int ), 16, 0, width, pad, 'A');
                continue;
            }
            if (*format == 'u') {
                pc += printi(out, va_arg( args, int ), 10, 0, width, pad, 'a');
                continue;
            }
            if (*format == 'c') {
                /* char are converted to int then pushed on the stack */
                scr[0] = (char) va_arg( args, int );
                scr[1] = '\0';
                pc += prints(out, scr, width, pad);
                continue;
            }
        } else {
            out: printchar(out, *format);
            ++pc;
        }
    }
    if (out)
        **out = '\0';
    va_end( args );
    return pc;
}

int u_printf(const char *format, ...) {
    va_list args;
    va_start(args, format );
    print(0, format, args);
    return 0;
}

int u_sprintf(char *out, const char *format, ...) {
    va_list args;
    va_start( args, format );
    return print(&out, format, args);
}

void u_array_printf(unsigned char*data, unsigned int len) {
    u_printf("{");
    for(int i = 0; i < len; ++i){
        u_printf("%X%s", data[i], i<(len)-1? ":":" ");
    }
    u_printf("}\n");
}

void ReceiveonUARTcallback(UART2_Handle handle, void *buffer, size_t count, void *userArg, int_fast16_t status)
{
    if (status == UART2_STATUS_SUCCESS)
    {
        bytesReadCount = count;
    }else{
        UART2_read(uart, &input, bytesToRead, NULL);
    }

}


void initUart()
{
    UART2_Params uartParams;
    UART2_Params_init(&uartParams);
    uartParams.baudRate = 115200;
    uartParams.readMode = UART2_Mode_CALLBACK;
    uartParams.readCallback = ReceiveonUARTcallback;
    uartParams.readReturnMode = UART2_ReadReturnMode_PARTIAL;
    while(uart == NULL)
    {
        uart = UART2_open(CONFIG_UART2_0, &uartParams);
    }
    UART2_read(uart, &input, bytesToRead, NULL);
}

void uartEnableInt()
{
    UART2_read(uart, &input, bytesToRead, NULL);// Start UART interrupt again
}

void uartWrite(uint8_t *buffer, size_t len)
{
    size_t bytesWritten = 0;
    UART2_write(uart, buffer, len, &bytesWritten);
}
