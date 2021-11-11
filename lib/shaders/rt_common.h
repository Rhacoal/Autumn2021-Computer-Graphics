#ifndef ASSIGNMENT_RT_COMMON_H
#define ASSIGNMENT_RT_COMMON_H

#ifdef __cplusplus
#define CPP_INLINE inline
#define debugger do {int a = 0;}while(0)
#else
#define CPP_INLINE
#define debugger
#endif

uint next(ulong *, int);

float randomFloat(ulong *);

float pow2(float v);

CPP_INLINE uint next(ulong *seed, int bits) {
    *seed = (*seed * 0x5DEECE66DL + 0xBL) & (((ulong) 1ul << 48) - 1);
    return (uint) (*seed >> (48 - bits));
}

CPP_INLINE float randomFloat(ulong *seed) {
    return (float) next(seed, 24) / (float) (1 << 24);
}

CPP_INLINE uint randomInt(ulong *seed, uint upper) {
    return next(seed, 32) % upper;
}

CPP_INLINE float pow2(float v) {
    return v * v;
}

CPP_INLINE uint pow3i(uint p) {
    return p * p * p;
}

CPP_INLINE float pow5(float v) {
    float t = v * v;
    return t * t * v;
}

#endif //ASSIGNMENT_RT_COMMON_H
