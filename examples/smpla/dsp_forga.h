/*
 * name: "Faust Organ"
 * Code generated with Faust 2.5.32 (https://faust.grame.fr)
 * Compilation options: c, -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __forga_H__
#define  __forga_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif


#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdlib.h>

#ifndef FAUSTCLASS
#define FAUSTCLASS forga
#endif
#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

typedef struct {

	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fButton0;
	FAUSTFLOAT fHslider1;
	float fRec0[2];
	int fSamplingFreq;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider2;
	float fRec1[2];
	float fConst2;
	float fRec2[2];
	float fConst3;
	float fRec3[2];

} forga_t;

static inline forga_t* newforga()
{
	forga_t* dsp = (forga_t*)calloc(1, sizeof(forga_t));
	return dsp;
}

static inline void deleteforga(forga_t* dsp)
{
	free(dsp);
}

/*
void metadataforga(MetaGlue* m)
{
	m->declare(m->metaInterface, "filename", "Forga");
	m->declare(m->metaInterface, "maths.lib/author", "GRAME");
	m->declare(m->metaInterface, "maths.lib/copyright", "GRAME");
	m->declare(m->metaInterface, "maths.lib/license", "LGPL with exception");
	m->declare(m->metaInterface, "maths.lib/name", "Faust Math Library");
	m->declare(m->metaInterface, "maths.lib/version", "2.1");
	m->declare(m->metaInterface, "name", "Forga Organ");
}
*/

static inline int getSampleRateforga(forga_t* dsp)
{
	return dsp->fSamplingFreq;
}

static inline int getNumInputsforga(forga_t* dsp)
{
	return 0;

}
static inline int getNumOutputsforga(forga_t* dsp)
{
	return 1;

}
static inline int getInputRateforga(forga_t* dsp, int channel)
{
	int rate;
	switch (channel) {
	default: {
		rate = -1;
		break;
	}

	}
	return rate;
}

static inline int getOutputRateforga(forga_t* dsp, int channel)
{
	int rate;
	switch (channel) {
	case 0: {
		rate = 1;
		break;
	}
	default: {
		rate = -1;
		break;
	}

	}
	return rate;

}

static inline void classInitforga(int samplingFreq)
{

}

static inline void instanceResetUserInterfaceforga(forga_t* dsp)
{
	dsp->fHslider0 = (FAUSTFLOAT)0.5f;
	dsp->fButton0 = (FAUSTFLOAT)0.0f;
	dsp->fHslider1 = (FAUSTFLOAT)0.5f;
	dsp->fHslider2 = (FAUSTFLOAT)440.0f;
}

static inline void instanceClearforga(forga_t* dsp)
{
	/* C99 loop */
	{
		int l0;
		for (l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			dsp->fRec0[l0] = 0.0f;

		}

	}
	/* C99 loop */
	{
		int l1;
		for (l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			dsp->fRec1[l1] = 0.0f;

		}

	}
	/* C99 loop */
	{
		int l2;
		for (l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			dsp->fRec2[l2] = 0.0f;

		}

	}
	/* C99 loop */
	{
		int l3;
		for (l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			dsp->fRec3[l3] = 0.0f;

		}

	}

}

static inline void instanceConstantsforga(forga_t* dsp, int samplingFreq)
{
	dsp->fSamplingFreq = samplingFreq;
	dsp->fConst0 = fmin(48000.0f, fmax(1.0f, (float)dsp->fSamplingFreq));
	dsp->fConst1 = (1.0f / dsp->fConst0);
	dsp->fConst2 = (2.0f / dsp->fConst0);
	dsp->fConst3 = (3.0f / dsp->fConst0);

}

static inline void instanceInitforga(forga_t* dsp, int samplingFreq)
{
	instanceConstantsforga(dsp, samplingFreq);
	instanceResetUserInterfaceforga(dsp);
	instanceClearforga(dsp);
}

static inline void initforga(forga_t* dsp, int samplingFreq)
{
	classInitforga(samplingFreq);
	instanceInitforga(dsp, samplingFreq);
}

/*
void buildUserInterfaceforga(forga_t* dsp, UIGlue* ui_interface)
{
	ui_interface->openVerticalBox(ui_interface->uiInterface, "myFaustProgram");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider2, "unit", "Hz");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "freq", &dsp->fHslider2, 440.0f, 20.0f, 20000.0f, 1.0f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "gain", &dsp->fHslider1, 0.5f, 0.0f, 10.0f, 0.00999999978f);
	ui_interface->addButton(ui_interface->uiInterface, "gate", &dsp->fButton0);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "volume", &dsp->fHslider0, 0.5f, 0.0f, 1.0f, 0.00999999978f);
	ui_interface->closeBox(ui_interface->uiInterface);
}
*/

static inline void computeforga(forga_t* dsp, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
{
	FAUSTFLOAT* output0 = outputs[0];
	float fSlow0 = (float)dsp->fHslider0;
	float fSlow1 = (0.000500000024f * ((float)dsp->fButton0 * (float)dsp->fHslider1));
	float fSlow2 = (float)dsp->fHslider2;
	float fSlow3 = (dsp->fConst1 * fSlow2);
	float fSlow4 = (dsp->fConst2 * fSlow2);
	float fSlow5 = (dsp->fConst3 * fSlow2);
	/* C99 loop */
	{
		int i;
		for (i = 0; (i < count); i = (i + 1)) {
			dsp->fRec0[0] = (fSlow1 + (0.999499977f * dsp->fRec0[1]));
			dsp->fRec1[0] = fmodf((fSlow3 + dsp->fRec1[1]), 1.0f);
			dsp->fRec2[0] = fmodf((fSlow4 + dsp->fRec2[1]), 1.0f);
			dsp->fRec3[0] = fmodf((fSlow5 + dsp->fRec3[1]), 1.0f);
			output0[i] = (FAUSTFLOAT)(fSlow0 * (dsp->fRec0[0] * ((sinf((6.28318548f * dsp->fRec1[0])) + (0.5f * sinf((6.28318548f * dsp->fRec2[0])))) + (0.25f * sinf((6.28318548f * dsp->fRec3[0]))))));
			dsp->fRec0[1] = dsp->fRec0[0];
			dsp->fRec1[1] = dsp->fRec1[0];
			dsp->fRec2[1] = dsp->fRec2[0];
			dsp->fRec3[1] = dsp->fRec3[0];

		}

	}

}

#ifdef __cplusplus
}
#endif


#endif
