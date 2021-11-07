#ifndef ASSIGNMENT_RT_COMMON_H
#define ASSIGNMENT_RT_COMMON_H

#ifdef __cplusplus
#define CPP_INLINE inline
#define debugger do {int a = 0;}while(0)
#else
#define CPP_INLINE
#define debugger
#endif

int next(ulong *, int);

float randomFloat(ulong *);

float pow2(float v);

CPP_INLINE int next(ulong *seed, int bits) {
    *seed = (*seed * 0x5DEECE66DL + 0xBL) & (((ulong) 1ul << 48) - 1);
    return (int) (*seed >> (48 - bits));
}

CPP_INLINE float randomFloat(ulong *seed) {
    return (float) next(seed, 24) / (float) (1 << 24);
}

CPP_INLINE float pow2(float v) {
    return v * v;
}

CPP_INLINE float pow5(float v) {
    float t = v * v;
    return t * t * v;
}

#endif //ASSIGNMENT_RT_COMMON_H
