#include "mudconf.h"

#include <stdarg.h>
#include <string.h>


extern btshim_send_channel(const char *chan, const char *msg);

void
send_channel(char *chan, const char *format, ...)
{
	char tmp_buf[LBUF_SIZE];
	va_list ap;

	va_start(ap, format);
	vsnprintf(tmp_buf, LBUF_SIZE, format, ap);
	va_end(ap);

	btshim_send_channel(chan, tmp_buf);
}
