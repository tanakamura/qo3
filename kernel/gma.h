#ifndef QO3_KERNEL_GMA_H
#define QO3_KERNEL_GMA_H

#include "kernel/display.h"
#include "kernel/event.h"

enum gma_init_error_code {
	GMA_NOT_FOUND
};

struct gma_init_error {
	enum gma_init_error_code code;
};

int gma_init(struct gma_init_error *error);

void gma_setmode(int width, int height);

void gma_disable_vga(void);
void gma_enable_vga(void);

enum gma_read_edid_error_code {
	GMA_READ_EDID_TOO_LARGE
};

struct gma_read_edid_error {
	enum gma_read_edid_error_code code;

	union {
		int buffer_required; /* GMA_READ_EDID_TOO_LARGE */
	} u;
};

/* returns size of edid
 * or negative if failed */
int gma_read_edid(char *buffer,
		  int size_buffer,
		  struct gma_read_edid_error *e);

#define GMA_I2C_PIN_ANALOG 0x02
#define GMA_I2C_PIN_LVDS 0x3
#define GMA_I2C_PIN_DP_D 0x3
#define GMA_I2c_PIN_SDVO 0x5
#define GMA_I2c_PIN_HDMI 0x5

void gma_i2c_receive(char *dest, int len, int addr, int pin,
		     event_bits_t *ready_ptr, event_bits_t ready_bits);
void gma_i2c_transfer(const char *src, int len, int addr, int pin,
		      event_bits_t *ready_ptr, event_bits_t ready_bits);

#endif
