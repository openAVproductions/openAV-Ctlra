#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "ctlra.h"

#include <opencv2/opencv.hpp>


static volatile uint32_t done;
static u_int16_t* image = NULL;

using namespace cv;

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void ni_s4_mk3_display_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
			void *userdata)
{
	/* Notifies application of device removal, also allows cleanup
	 * of any device specific structures. See daemon/ example, where
	 * the MIDI I/O is cleaned up in the remove() function */
	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(dev, &info);
	printf("ni_s4_mk3_display: removing %s %s\n", info.vendor, info.device);
}

int32_t ni_s4_mk3_display_screen_redraw_func(struct ctlra_dev_t *dev,
				  uint32_t screen_idx,
				  uint8_t *pixel_data,
				  uint32_t bytes,
				  struct ctlra_screen_zone_t *redraw_zone,
				  void *userdata)
{
	/* Here, we naively copy the image buffer, although it may not have changed 
	 * since the last frame */
	memcpy((uint16_t*)pixel_data, image, bytes);
	return 1;
}

int accept_dev_func(struct ctlra_t *ctlra,
		    const struct ctlra_dev_info_t *info,
		    struct ctlra_dev_t *dev,
                    void *userdata)
{
	printf("ni_s4_mk3_display: accepting %s %s\n", info->vendor, info->device);

	/* here we use the Ctlra APIs to set callback functions to get
	 * events and send feedback updates to/from the device */
	ctlra_dev_set_screen_feedback_func(dev, ni_s4_mk3_display_screen_redraw_func);
	ctlra_dev_set_remove_func(dev, ni_s4_mk3_display_remove_func);
	ctlra_dev_set_callback_userdata(dev, 0x0);

	return 1;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sighndlr);

	if (argc < 2){
		fprintf(stderr, "Usage: %s <RawMatrixOrImage>\n", argv[0]);
		return -1;
	}

	image = (u_int16_t*)malloc(sizeof(u_int16_t) * 320 * 240);

	if (strcmp(argv[1] + strlen(argv[1]) - 4, ".raw") == 0){
		// From a raw capture file
		int f = open(argv[1], O_RDONLY);

		if (f < 0){
			perror("Cannot open file");
			return 1;
		}

		read(f, image, 320 * 240 * 2);
		close(f);

	} else {
		// From an image file
		Mat original(imread(argv[1]));

		if (original.empty())
		{
			std::cout << "Failed to load image!" << std::endl;
			return -1;
		}

		if (original.size().width != 320 || original.size().height != 240){
			resize(original, original, Size(320, 240), INTER_LINEAR);
		}

		/* The S4 Mk3 screen use a limited palette of color, which includes 
		 * 4 value of red, 5 of blue and 4 of green, so we need to convert color value */
		uchar* ptr = original.ptr<uchar>();
		u_int8_t r = 0x1f, b = 0, g = 0;
		for (int i = 0; i < 320 * 240; i ++){
			g = ptr[i * 3] / 64;
			b = ptr[i * 3 + 1] / 32;
			r = ptr[i * 3 + 2] / 64;
			image[i] = r << 11 | (g & 0x3f) << 5 | (b & 0x1F);
		}
	}

    
	struct ctlra_create_opts_t opts = {
		.screen_redraw_target_fps = 10,
	};

	struct ctlra_t *ctlra = ctlra_create(&opts);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, 0x0);
	printf("connected devices %d\n", num_devs);

	while(!done) {
		ctlra_idle_iter(ctlra);
		usleep(10 * 1000);
	}

	free(image);

	ctlra_exit(ctlra);

	return 0;
}
