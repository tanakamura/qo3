#ifndef QO3_KERNEL_DISPLAY_H
#define QO3_KERNEL_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

struct disp_mode {
	int width;
	int height;
};

struct display_info {
	int num_mode;
	struct disp_mode *modes;
};

#ifdef __cplusplus
}
#endif

#endif
