#include <stdint.h>
#include "event.h"
#include "impl.h"

const char *ctlra_event_type_names[] = {
	"Button",
	"Encoder",
	"Slider",
	"Grid",
	"Feedback",
};
#define NAMES_SIZE (sizeof(ctlra_event_type_names) / \
		    sizeof(ctlra_event_type_names[0]))

void event_checks(void)
{
	BUILD_BUG_ON(NAMES_SIZE != CTLRA_EVENT_T_COUNT);
}
