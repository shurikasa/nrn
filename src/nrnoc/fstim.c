#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/fstim.c,v 1.2 1997/08/15 13:04:11 hines Exp */
/* copy of synapse.c modified to simulate current stimulus pulses */
/* 4/9/2002 modified to conform to new treeset.c */

/*
fstim(maxnum)
 allocates space for maxnum synapses. Space for
 previously existing synapses is released. All synapses initialized to
 0 maximum conductance.

fstim(i, loc, delay, duration, stim)
 The ith current stimulus is injected at parameter `loc'
 different current stimuli do not concatenate but can ride on top of
 each other. delay refers to onset of stimulus relative to t=0
 delay and duration are in msec.
 stim in namps.
  
fstimi(i)
 returns stimulus current for ith stimulus at the value of the
 global time t.

*/

#include "neuron.h"
#include "section.h"

typedef struct Stimulus {
	double loc;	/* parameter location (0--1) */
	double delay;	/* value of t in msec for onset */
	double duration;/* turns off at t = delay + duration */
	double mag;	/* magnitude in namps */
	double mag_seg;	/* value added to rhs, depends on area of seg*/
	Node *pnd;	/* segment location */
	Section* sec;
} Stimulus;

static maxstim = 0;		/* size of stimulus array */
static Stimulus *pstim;		/* pointer to stimulus array */

extern double *getarg(), chkarg();
extern char *secname();

print_stim() {
	int i;
	
	if (maxstim == 0) return;
	/*SUPPRESS 440*/
	Printf("fstim(%d)\n/* section	fstim( #, loc, delay(ms), duration(ms), magnitude(namp)) */\n", maxstim);
	for (i=0; i<maxstim; i++) {
		Printf("%-15s fstim(%2d,%4g,%10g,%13g,%16g)\n",
		secname(pstim[i].sec), i,
		pstim[i].loc, pstim[i].delay, pstim[i].duration, pstim[i].mag);
	}
}

static double stimulus();

fstimi() {
	int i;
	double cur;
	
	i = chkarg(1, 0., (double)(maxstim-1));
	if ((cur = stimulus(i)) != 0.) {
		cur = pstim[i].mag;
	}
	ret(cur);
}

static stim_record();

fstim() {
	int i;

	i = chkarg(1, 0., 10000.);
	if (ifarg(2)) {
		if (i >= maxstim) {
			hoc_execerror("index out of range", (char *)0);
		}
		pstim[i].loc = chkarg(2, 0., 1.);
		pstim[i].delay = chkarg(3, 0., 1e21);
		pstim[i].duration = chkarg(4, 0., 1e21);
		pstim[i].mag = *getarg(5);
		pstim[i].sec = chk_access();
		section_ref(pstim[i].sec);
		stim_record(i);
	} else {
		free_stim();
		maxstim = i;
		if (maxstim) {
			pstim = (Stimulus *)emalloc((unsigned)(maxstim * sizeof(Stimulus)));
		}
		for (i = 0; i<maxstim; i++) {
			pstim[i].loc = 0;
			pstim[i].mag = 0.;
			pstim[i].delay = 1e20;
			pstim[i].duration = 0.;
			pstim[i].sec = 0;
			stim_record(i);
		}
	}
	ret(0.);
}

free_stim() {
	int i;
	if (maxstim) {
		for (i=0; i < maxstim; ++i) {
			if (pstim[i].sec) {
				section_unref(pstim[i].sec);
			}
		}
		free((char *)pstim);
		maxstim=0;
	}
}

static
stim_record(i)	/*fill in the section info*/
	int i;
{
	Node *node_ptr();
	double area;
	Section* sec;

	sec = pstim[i].sec;
	if (sec) {
		if (sec->prop) {
			pstim[i].pnd = node_ptr(sec, pstim[i].loc, &area);
			pstim[i].mag_seg = 1.e2*pstim[i].mag / area;
		}else{
			section_unref(sec);
			pstim[i].sec = 0;
		}
	}
}

stim_prepare() {
	int i;
	
	for (i=0; i<maxstim; i++) {
		stim_record(i);
	}
}

static double
stimulus(i)
	int i;
{
#if CVODE
	at_time(pstim[i].delay);
	at_time(pstim[i].delay + pstim[i].duration);
#endif
	if (t < pstim[i].delay-1e-9
	  || t > pstim[i].delay + pstim[i].duration - 1e-9) {
		return 0.0;
	}
	return pstim[i].mag_seg;
}

activstim_rhs() {
	int i;

	for (i=0; i<maxstim; i++) {
		if (pstim[i].sec) {
			NODERHS(pstim[i].pnd) += stimulus(i);
		}
	}
}
