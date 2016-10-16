#include "ctlr.h"
#include "devices.h"
#include "devices/device_impl.h"

struct ctlr_dev_connect_func_t {
	enum ctlr_dev_id_t id;
	struct ctlr_dev_base_t base;
};

static const struct ctlr_dev_connect_func_t devices[] = {
	{CTLR_DEV_NI_MASCHINE, ni_maschine},
};
#define NUM_DEVS (sizeof(devices) / sizeof(devices[0]))

struct ctlr_dev_t *ctlr_dev_connect(enum ctlr_dev_id_t dev_id, void *future)
{
	if(dev_id >= NUM_DEVS)
		return 0;
	
	if(devices[dev_id].base.connect)
		return devices[dev_id].base.connect(future);

	return 0;
}
