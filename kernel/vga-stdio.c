#include <string.h>

static int con_x, con_y;

void
vga_write_str(const char *str, int len)
{
	char *p = (char*)0xb8000;
	int i;
	for (i=0; i<len; i++) {
		char c = str[i];

		if (c == '\n') {
			con_x = 0;
			con_y++;
			if (con_y >= 25)
				con_y = 0;
		} else {
			p[con_x*2 + con_y*160] = str[i];
			con_x++;
			if (con_x >= 80) {
				con_y++;
				con_x=0;
			}
		}
	}
}

int
vga_puts(const char *str)
{
	char crlf[] = "\n";
	int len = strlen(str);
	vga_write_str(str, len);
	vga_write_str(crlf, 1);

	return len+2;
}
