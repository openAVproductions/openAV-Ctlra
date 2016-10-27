//#undef NDEBUG
#include <cassert>
#include <cstddef>
#include <stdio.h>

#include "ctlr/ctlr.h"
#include "benchmark/benchmark.h"

static uint32_t counter;
static struct ctlr_dev_t* dev;

static void clobber()
{
	asm volatile("" : : : "memory");
}
static void escape(void *p)
{
	asm volatile("" : : "g"(p) : "memory");
}

void perf_event_empty(struct ctlr_dev_t* dev,
                      uint32_t num_events,
                      struct ctlr_event_t** events,
                      void *userdata)
{
	counter++;
}

void perf_event_switch(struct ctlr_dev_t* dev,
                       uint32_t num_events,
                       struct ctlr_event_t** events,
                       void *userdata)
{
	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlr_event_t *e = events[i];
		switch(e->id) {
		case CTLR_EVENT_BUTTON:
			break;
		case CTLR_EVENT_ENCODER:
			break;
		case CTLR_EVENT_GRID:
			break;
		default:
			break;
		};
	}
	counter++;
}

void ctlr_dev_empty(benchmark::State& state)
{
	dev = ctlr_dev_connect(CTLR_DEV_SIMPLE, perf_event_empty, 0, 0);
	counter = 0;
	while (state.KeepRunning()) {
		ctlr_dev_poll(dev);
	}
	ctlr_dev_disconnect(dev);
}

void ctlr_dev_switch(benchmark::State& state)
{
	dev = ctlr_dev_connect(CTLR_DEV_SIMPLE, perf_event_switch, 0, 0);
	counter = 0;
	while (state.KeepRunning()) {
		ctlr_dev_poll(dev);
	}
	ctlr_dev_disconnect(dev);
}

BENCHMARK(ctlr_dev_empty);
BENCHMARK(ctlr_dev_switch);

BENCHMARK_MAIN()
