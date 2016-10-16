#include "ctlr.h"
#include "devices.h"

// Make available to devices
typedef struct ctlr_dev_t *(*ctlr_impl_dev_connect)(void *future);

struct ctlr_dev_connect_func_t {
	enum ctlr_dev_id_t id;
	ctlr_impl_dev_connect connect;
};

static const struct ctlr_dev_connect_func_t devices[] = {
	{CTLR_DEV_NI_MASCHINE, ctlr_ni_maschine_connect},
};
#define NUM_DEVS (sizeof(devices) / sizeof(devices[0]))

struct ctlr_dev_t *ctlr_dev_connect(enum ctlr_dev_id_t dev_id, void *future)
{
	if(dev_id < NUM_DEVS)
		return devices[dev_id].connect(future);

	return 0;
}
