#include "ctlr/ctlr.h"

int main()
{
	struct ctlr_dev_t *dev = ctlr_dev_connect(CTLR_DEV_NI_MASCHINE,0);
	return 0;
}
