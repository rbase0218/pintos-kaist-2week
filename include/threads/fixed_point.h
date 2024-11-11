/* Fixed point number format: 17.14 */
#define F (1 << 14)     /* 2^14 = 16384 */

/* Convert n to fixed point: n * f */
#define CONV_TO_FP(n) ((n) * F)

/* Convert x to integer (rounding toward zero): x / f */
#define CONV_TO_INT(x) ((x) / F)

/* Convert x to integer (rounding to nearest) */
#define CONV_TO_INT_ROUND(x) \
    ((x) >= 0 ? ((x) + F / 2) / F : ((x) - F / 2) / F)

/* Add x and y */
#define FP_ADD(x, y) ((x) + (y))
/* Add x and n */
#define FP_ADD_INT(x, n) ((x) + (n) * F)

/* Subtract y from x */
#define FP_SUB(x, y) ((x) - (y))
/* Subtract n from x */
#define FP_SUB_INT(x, n) ((x) - (n) * F)

/* Multiply x by y */
#define FP_MUL(x, y) ((int64_t)(x)) * (y) / F
/* Multiply x by n */
#define FP_MUL_INT(x, n) ((x) * (n))

/* Divide x by y */
#define FP_DIV(x, y) ((int64_t)(x)) * F / (y)
/* Divide x by n */
#define FP_DIV_INT(x, n) ((x) / (n))