#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "audio.h"

struct smpla_lv2_t {
	const float* a_in[2];
	float* a_out[2];

	struct smpla_t *s;
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate,
	    const char* bundle_path, const LV2_Feature* const* features)
{
	struct smpla_lv2_t *lv2 = calloc(1, sizeof(struct smpla_lv2_t));

	struct smpla_t *s = smpla_init(rate);
	struct mm_t *mm = calloc(1, sizeof(struct mm_t));
	mm->mode = MODE_PADS;
	s->controller_data = mm;

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, s);
	printf("sequencer: Connected controllers %d\n", num_devs);

	s->ctlra = ctlra;

	lv2->s = s;
	return (LV2_Handle)lv2;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	struct smpla_lv2_t *lv2 = instance;

	switch (port) {
	case A_IN_L: lv2->a_in[0] = data; break;
	case A_IN_R: lv2->a_in[1] = data; break;
	case A_OUT_L: lv2->a_out[0] = data; break;
	case A_OUT_R: lv2->a_out[1] = data; break;
	default:
		printf("Smpla (%p): connect port got invalid id %d\n",
		       lv2, port);
		break;
	}
}

static void
activate(LV2_Handle instance)
{
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	struct smpla_lv2_t *lv2 = instance;

	int ret = smpla_process(lv2->s, nframes, lv2->a_in, lv2->a_out);
	(void)ret;
}

static void
deactivate(LV2_Handle instance)
{
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	SMPLA_DSP_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
