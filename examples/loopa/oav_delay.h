/* ------------------------------------------------------------
name: "delay_otto"
Code generated with Faust 2.5.21 (https://faust.grame.fr)
Compilation options: c, -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __delay_t_H__
#define  __delay_t_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 


#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdlib.h>

static float delay_t_faustpower2_f(float value) {
	return (value * value);
	
}

#ifndef FAUSTCLASS 
#define FAUSTCLASS delay_t
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

typedef struct {
	
	FAUSTFLOAT fHslider0;
	float fRec3[2];
	FAUSTFLOAT fHslider1;
	FAUSTFLOAT fCheckbox0;
	int fSamplingFreq;
	float fConst0;
	FAUSTFLOAT fHslider2;
	float fConst1;
	FAUSTFLOAT fHslider3;
	int IOTA;
	float fVec0[524288];
	float fVec1[131072];
	float fRec5[2];
	float fRec4[2];
	float fConst2;
	FAUSTFLOAT fHslider4;
	float fRec6[2];
	float fRec2[3];
	float fRec0[524288];
	float fVec2[131072];
	float fRec7[3];
	float fRec1[524288];
	
} delay_t;

delay_t* newdelay_t() { 
	delay_t* dsp = (delay_t*)malloc(sizeof(delay_t));
	return dsp;
}

void deletedelay_t(delay_t* dsp) { 
	free(dsp);
}

/*
void metadatadelay_t(MetaGlue* m) { 
	m->declare(m->metaInterface, "basics.lib/name", "Faust Basic Element Library");
	m->declare(m->metaInterface, "basics.lib/version", "0.0");
	m->declare(m->metaInterface, "delays.lib/name", "Faust Delay Library");
	m->declare(m->metaInterface, "delays.lib/version", "0.0");
	m->declare(m->metaInterface, "filters.lib/name", "Faust Filters Library");
	m->declare(m->metaInterface, "filters.lib/version", "0.0");
	m->declare(m->metaInterface, "maths.lib/author", "GRAME");
	m->declare(m->metaInterface, "maths.lib/copyright", "GRAME");
	m->declare(m->metaInterface, "maths.lib/license", "LGPL with exception");
	m->declare(m->metaInterface, "maths.lib/name", "Faust Math Library");
	m->declare(m->metaInterface, "maths.lib/version", "2.1");
	m->declare(m->metaInterface, "misceffects.lib/name", "Faust Math Library");
	m->declare(m->metaInterface, "misceffects.lib/version", "2.0");
	m->declare(m->metaInterface, "name", "delay_otto");
	m->declare(m->metaInterface, "routes.lib/name", "Faust Signal Routing Library");
	m->declare(m->metaInterface, "routes.lib/version", "0.0");
	m->declare(m->metaInterface, "signals.lib/name", "Faust Signal Routing Library");
	m->declare(m->metaInterface, "signals.lib/version", "0.0");
}
*/

int getSampleRatedelay_t(delay_t* dsp) { return dsp->fSamplingFreq; }

int getNumInputsdelay_t(delay_t* dsp) {
	return 2;
	
}
int getNumOutputsdelay_t(delay_t* dsp) {
	return 2;
	
}
int getInputRatedelay_t(delay_t* dsp, int channel) {
	int rate;
	switch (channel) {
		case 0: {
			rate = 1;
			break;
		}
		case 1: {
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
int getOutputRatedelay_t(delay_t* dsp, int channel) {
	int rate;
	switch (channel) {
		case 0: {
			rate = 1;
			break;
		}
		case 1: {
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

void classInitdelay_t(int samplingFreq) {
	
}

void instanceResetUserInterfacedelay_t(delay_t* dsp) {
	dsp->fHslider0 = (FAUSTFLOAT)50.0f;
	dsp->fHslider1 = (FAUSTFLOAT)0.0f;
	dsp->fCheckbox0 = (FAUSTFLOAT)0.0f;
	dsp->fHslider2 = (FAUSTFLOAT)0.5f;
	dsp->fHslider3 = (FAUSTFLOAT)133.0f;
	dsp->fHslider4 = (FAUSTFLOAT)1000.0f;
	
}

void instanceCleardelay_t(delay_t* dsp) {
	/* C99 loop */
	{
		int l0;
		for (l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			dsp->fRec3[l0] = 0.0f;
			
		}
		
	}
	dsp->IOTA = 0;
	/* C99 loop */
	{
		int l1;
		for (l1 = 0; (l1 < 524288); l1 = (l1 + 1)) {
			dsp->fVec0[l1] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l2;
		for (l2 = 0; (l2 < 131072); l2 = (l2 + 1)) {
			dsp->fVec1[l2] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l3;
		for (l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			dsp->fRec5[l3] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l4;
		for (l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			dsp->fRec4[l4] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l5;
		for (l5 = 0; (l5 < 2); l5 = (l5 + 1)) {
			dsp->fRec6[l5] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l6;
		for (l6 = 0; (l6 < 3); l6 = (l6 + 1)) {
			dsp->fRec2[l6] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l7;
		for (l7 = 0; (l7 < 524288); l7 = (l7 + 1)) {
			dsp->fRec0[l7] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l8;
		for (l8 = 0; (l8 < 131072); l8 = (l8 + 1)) {
			dsp->fVec2[l8] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l9;
		for (l9 = 0; (l9 < 3); l9 = (l9 + 1)) {
			dsp->fRec7[l9] = 0.0f;
			
		}
		
	}
	/* C99 loop */
	{
		int l10;
		for (l10 = 0; (l10 < 524288); l10 = (l10 + 1)) {
			dsp->fRec1[l10] = 0.0f;
			
		}
		
	}
	
}

void instanceConstantsdelay_t(delay_t* dsp, int samplingFreq) {
	dsp->fSamplingFreq = samplingFreq;
	dsp->fConst0 = fminf(192000.0f, fmaxf(1.0f, (float)dsp->fSamplingFreq));
	dsp->fConst1 = (15.0f * dsp->fConst0);
	dsp->fConst2 = (3.14159274f / dsp->fConst0);
	
}

void instanceInitdelay_t(delay_t* dsp, int samplingFreq) {
	instanceConstantsdelay_t(dsp, samplingFreq);
	instanceResetUserInterfacedelay_t(dsp);
	instanceCleardelay_t(dsp);
}

void initdelay_t(delay_t* dsp, int samplingFreq) {
	classInitdelay_t(samplingFreq);
	instanceInitdelay_t(dsp, samplingFreq);
}

/*
void buildUserInterfacedelay_t(delay_t* dsp, UIGlue* ui_interface) {
	ui_interface->openVerticalBox(ui_interface->uiInterface, "delay_otto");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "BPM", &dsp->fHslider3, 133.0f, 30.0f, 350.0f, 1.0f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider2, "unit", "s");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Echo Delay", &dsp->fHslider2, 0.5f, 0.00999999978f, 0.999000013f, 0.00100000005f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Feedback", &dsp->fHslider0, 50.0f, 0.0f, 99.0f, 1.0f);
	ui_interface->addCheckButton(ui_interface->uiInterface, "Follow BPM", &dsp->fCheckbox0);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Stereo Spread", &dsp->fHslider1, 0.0f, 0.0f, 99.0f, 1.0f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Tone", &dsp->fHslider4, 1000.0f, 100.0f, 2000.0f, 0.00999999978f);
	ui_interface->closeBox(ui_interface->uiInterface);
	
}
*/

void computedelay_t(delay_t* dsp, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
	FAUSTFLOAT* input0 = inputs[0];
	FAUSTFLOAT* input1 = inputs[1];
	FAUSTFLOAT* output0 = outputs[0];
	FAUSTFLOAT* output1 = outputs[1];
	float fSlow0 = (9.99999975e-06f * (float)dsp->fHslider0);
	float fSlow1 = (float)dsp->fHslider1;
	int iSlow2 = (fSlow1 == 99.0f);
	int iSlow3 = (1 - iSlow2);
	float fSlow4 = fmax(0.00999999978f, fmin(1.0f, (float)dsp->fHslider2));
	float fSlow5 = ((int)(float)dsp->fCheckbox0?(dsp->fConst1 * (ceilf((4.0f * fSlow4)) / (float)dsp->fHslider3)):(float)(int)(dsp->fConst0 * fSlow4));
	int iSlow6 = (int)fSlow5;
	int iSlow7 = (iSlow6 + 1);
	int iSlow8 = ((int)((float)((int)(10.0f * fSlow1) * iSlow3) + fSlow5) + 1);
	float fSlow9 = (0.00100000005f * (float)dsp->fHslider4);
	/* C99 loop */
	{
		int i;
		for (i = 0; (i < count); i = (i + 1)) {
			dsp->fRec3[0] = (fSlow0 + (0.999000013f * dsp->fRec3[1]));
			dsp->fVec0[(dsp->IOTA & 524287)] = fSlow5;
			float fTemp0 = (dsp->fRec3[0] * (iSlow3?dsp->fRec0[((dsp->IOTA - iSlow8) & 524287)]:dsp->fRec1[((dsp->IOTA - iSlow7) & 524287)]));
			dsp->fVec1[(dsp->IOTA & 131071)] = fTemp0;
			dsp->fRec5[0] = ((0.999000013f * dsp->fRec5[1]) + (0.0120000001f * ((dsp->fVec0[((dsp->IOTA - iSlow6) & 524287)] - fSlow5) / fSlow5)));
			dsp->fRec4[0] = fmodf((dsp->fRec4[1] + (251.0f - powf(2.0f, (0.0833333358f * dsp->fRec5[0])))), 250.0f);
			int iTemp1 = (int)dsp->fRec4[0];
			int iTemp2 = fmin(65537, fmax(0, iTemp1));
			float fTemp3 = floorf(dsp->fRec4[0]);
			float fTemp4 = (fTemp3 + (1.0f - dsp->fRec4[0]));
			float fTemp5 = (dsp->fRec4[0] - fTemp3);
			int iTemp6 = fmin(65537, fmax(0, (iTemp1 + 1)));
			float fTemp7 = fmin((0.0199999996f * dsp->fRec4[0]), 1.0f);
			float fTemp8 = (dsp->fRec4[0] + 250.0f);
			int iTemp9 = (int)fTemp8;
			int iTemp10 = fmin(65537, fmax(0, iTemp9));
			float fTemp11 = floorf(fTemp8);
			float fTemp12 = (fTemp11 + (-249.0f - dsp->fRec4[0]));
			float fTemp13 = (dsp->fRec4[0] + (250.0f - fTemp11));
			int iTemp14 = fmin(65537, fmax(0, (iTemp9 + 1)));
			float fTemp15 = (1.0f - fTemp7);
			dsp->fRec6[0] = (fSlow9 + (0.999000013f * dsp->fRec6[1]));
			float fTemp16 = tanf((dsp->fConst2 * dsp->fRec6[0]));
			float fTemp17 = (1.0f / fTemp16);
			float fTemp18 = (1.0f - ((1.0f - fTemp17) / fTemp16));
			float fTemp19 = (1.0f - (1.0f / delay_t_faustpower2_f(fTemp16)));
			float fTemp20 = (((fTemp17 + 1.0f) / fTemp16) + 1.0f);
			dsp->fRec2[0] = (((((dsp->fVec1[((dsp->IOTA - iTemp2) & 131071)] * fTemp4) + (fTemp5 * dsp->fVec1[((dsp->IOTA - iTemp6) & 131071)])) * fTemp7) + (((dsp->fVec1[((dsp->IOTA - iTemp10) & 131071)] * fTemp12) + (fTemp13 * dsp->fVec1[((dsp->IOTA - iTemp14) & 131071)])) * fTemp15)) - (((dsp->fRec2[2] * fTemp18) + (2.0f * (dsp->fRec2[1] * fTemp19))) / fTemp20));
			float fTemp21 = (0.0f - fTemp17);
			float fTemp22 = (float)input0[i];
			dsp->fRec0[(dsp->IOTA & 524287)] = ((((dsp->fRec2[2] * fTemp21) + (dsp->fRec2[0] / fTemp16)) / fTemp20) + ((float)iSlow3 * fTemp22));
			float fTemp23 = (dsp->fRec3[0] * (iSlow3?dsp->fRec1[((dsp->IOTA - iSlow7) & 524287)]:dsp->fRec0[((dsp->IOTA - iSlow8) & 524287)]));
			dsp->fVec2[(dsp->IOTA & 131071)] = fTemp23;
			dsp->fRec7[0] = (((fTemp7 * ((fTemp4 * dsp->fVec2[((dsp->IOTA - iTemp2) & 131071)]) + (fTemp5 * dsp->fVec2[((dsp->IOTA - iTemp6) & 131071)]))) + (fTemp15 * ((fTemp12 * dsp->fVec2[((dsp->IOTA - iTemp10) & 131071)]) + (fTemp13 * dsp->fVec2[((dsp->IOTA - iTemp14) & 131071)])))) - (((fTemp18 * dsp->fRec7[2]) + (2.0f * (fTemp19 * dsp->fRec7[1]))) / fTemp20));
			dsp->fRec1[(dsp->IOTA & 524287)] = ((((fTemp21 * dsp->fRec7[2]) + (dsp->fRec7[0] / fTemp16)) / fTemp20) + fTemp22);
			output0[i] = (FAUSTFLOAT)(dsp->fRec0[((dsp->IOTA - 0) & 524287)] + ((float)iSlow2 * (float)input1[i]));
			output1[i] = (FAUSTFLOAT)dsp->fRec1[((dsp->IOTA - 0) & 524287)];
			dsp->fRec3[1] = dsp->fRec3[0];
			dsp->IOTA = (dsp->IOTA + 1);
			dsp->fRec5[1] = dsp->fRec5[0];
			dsp->fRec4[1] = dsp->fRec4[0];
			dsp->fRec6[1] = dsp->fRec6[0];
			dsp->fRec2[2] = dsp->fRec2[1];
			dsp->fRec2[1] = dsp->fRec2[0];
			dsp->fRec7[2] = dsp->fRec7[1];
			dsp->fRec7[1] = dsp->fRec7[0];
			
		}
		
	}
	
}

#ifdef __cplusplus
}
#endif


#endif
