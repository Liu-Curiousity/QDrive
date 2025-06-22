#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>
#include "retarget.h"
#include <stdint.h>

#include "usbd_cdc_if.h"
#include "CharCircularQueue.h"

#if !defined(OS_USE_SEMIHOSTING)

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

void RetargetInit() {
#if USE_TinyPrintf == 0 && STDIO_SUPPORT == 1
    /* Disable I/O buffering for STDOUT stream, so that
     * chars are sent out as soon as they are printed. */
    setvbuf(stdout, NULL, _IONBF, 0);
#endif
}

CharCircularQueue char_queue{128};

void CDC_Receive_FS_Callback(uint8_t *Buf, uint32_t *Len) {
    auto length = *Len;
    while (length--) char_queue.enqueue(*(Buf++));
}

signed short shellRead(char *data, unsigned short len) {
    signed short i = 0;
    for (i = 0; i < len && !char_queue.isEmpty(); ++i, ++data) {
        char_queue.dequeue(*data);
    }
    delay(1); // 延时1ms,因为shellTask是死循环一点delay都没有,为了让IDLE线程能够运行以释放内存等
    return i;
}

signed short shellWrite(char *data, unsigned short len) {
    const uint32_t start_tick = HAL_GetTick();
    while (HAL_OK != CDC_Transmit_FS(reinterpret_cast<uint8_t *>(data), len))
        if (HAL_GetTick() - start_tick > 100) return -1;
    return 0;
}

#if USE_TinyPrintf == 1
// provide your own _putchar() low level function as console/serial output
void _putchar(char character) {
    shellWrite(&character, 1);
}
#endif

#if STDIO_SUPPORT == 1
int _write(int fd, char *ptr, int len) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        shellWrite(ptr, len);
        if (shellWrite(ptr, len) == 0)
            return len;
        else
            return EIO;
    }
    errno = EBADF;
    return -1;
}

int _read(int fd, char *ptr, int len) {
    if (fd == STDIN_FILENO) {
        shellRead(ptr, 1);
        if (shellRead(ptr, 1) == 0)
            return 1;
        else
            return EIO;
    }
    errno = EBADF;
    return -1;
}

int _close(int fd) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
        return 0;

    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat *st) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
        st->st_mode = S_IFCHR;
        return 0;
    }

    errno = EBADF;
    return 0;
}

int _isatty(int fd) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
        return 1;

    errno = EBADF;
    return 0;
}

int _lseek(int fd, int ptr, int dir) {
    (void)fd;
    (void)ptr;
    (void)dir;

    errno = EBADF;
    return -1;
}
#endif

#endif //#if !defined(OS_USE_SEMIHOSTING)
