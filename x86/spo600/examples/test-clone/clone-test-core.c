// clone-test-core.c
// Chris Tyler 2017.11.29-2024.11.19 - Licensed under GPLv3.
// For the Seneca College SPO600 Course
//
// This code is designed to be compiled with different values for
// the CLONE_ATTRIBUTE macro for different architectures and for
// both 'prune' (cloned functions are the same after optimization)
// and 'noprune' (cloned functions are different after optimization)
// build cases
//
// The cloned function is scale_samples

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "vol.h"

int sum_sample (int16_t *buff, size_t samples) {
  int t;
  for (int x = 0; x < SAMPLES; x++) {
    t += buff[x];
  }
  return t;
}

CLONE_ATTRIBUTE
void scale_samples(int16_t *in, int16_t *out, int cnt, int volume) {
	for (int x = 0; x < cnt; x++) {
		out[x] = ((((int32_t) in[x]) * ((int32_t) (32767 * volume / 100) <<1) ) >> 16);
	}
}

CLONE_ATTRIBUTE
void convert_samples(int16_t *in, int16_t *out, int cnt) {
        for (int x = 0; x < cnt; x++) {
                // ---- Simple logic for PRUNE scenario (e.g., just pass through or scale) ----
                out[x] = in[x] / 2;
        }
}

int main() {
	int		x;
	int		ttl=0;

// ---- Create in[] and out[] arrays
	int16_t*	in;
	int16_t*	out;
	int16_t*        converted;
	in =  (int16_t*) calloc(SAMPLES, sizeof(int16_t));
	out = (int16_t*) calloc(SAMPLES, sizeof(int16_t));
        converted = (int16_t*) calloc(SAMPLES, sizeof(int16_t));

// ---- Create dummy samples in in[]
	vol_createsample(in, SAMPLES);	

// ---- This is the part we're interested in!
// ---- Scale the samples from in[], placing results in out[]
	scale_samples(in, out, SAMPLES, VOLUME);

// ---- Convert the samples from in[], placing results in converted[] ----
        convert_samples(in, converted, SAMPLES);

// ---- This part sums the samples. 
        ttl=sum_sample(out, SAMPLES);
        ttl+=sum_sample(converted, SAMPLES);

// ---- Print the sum of the samples.
	printf("Result: %d\n", ttl);

	return 0;

}
