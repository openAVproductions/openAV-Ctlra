#include "ctlr/ctlr.h"

int main()
{
	int dev_id = CTLR_DEV_NI_MASCHINE;
	struct ctlr_dev_t *dev = ctlr_dev_connect(dev_id, 0);
	uint32_t i = 5;

	while(i > 0)
	{
		ctlr_dev_poll(dev);
		i--;
	}

	ctlr_dev_disconnect(dev);
	return 0;
}
