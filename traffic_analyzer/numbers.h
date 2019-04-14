/* we store drops in 5 bits */
#define DROPS_M 2
#define DROPS_E 3

/* we store queue length in 11 bits */
#define QDELAY_M 7
#define QDELAY_E 4

/* Decode float value
 *
 * fl: Float value
 * m_b: Number of mantissa bits
 * e_b: Number of exponent bits
 */
u32 fl2int(u32 fl, u32 m_b, u32 e_b)
{
	const u32 m_max = 1 << m_b;

	fl &= ((m_max << e_b) - 1);

	if (fl < (m_max << 1)) {
		return fl;
	} else {
		return (((fl & (m_max - 1)) + m_max) << ((fl >> m_b) - 1));
	}
}

/* Encode integer value as float value
 * The value will be rounded down if needed
 *
 * val: Value to convert into a float
 * m_b: Number of mantissa bits
 * e_b: Number of exponent bits
 * r: Variable where the remainder will be stored
 */
u32 int2fl(u32 val, u32 m_b, u32 e_b, u32 *r)
{
	u32 len, exponent, mantissa;
	const u32 max_e = (1 << e_b) - 1;
	const u32 max_m = (1 << m_b) - 1;
	const u32 max_fl = ((max_m << 1) + 1) << (max_e - 1);
	*r = 0;

	if (val < (1 << (m_b + 1))) {
		/* possibly only first exponent included, no encoding needed */
		return val;
	}

	if (val >= max_fl) {
		/* avoid overflow */
		*r = val - max_fl;
		return (1 << (m_b + e_b)) - 1;
	}

	/* number of bits without leading 1 */
	len = (sizeof(u32) * 8) - __builtin_clz(val) - 1;

	exponent = len - m_b;
	mantissa = (val >> exponent) & ((1 << m_b) - 1);
	*r = val & ((1 << exponent) - 1);

	return ((exponent + 1) << m_b) | mantissa;
}

#ifdef TESTBED_ANALYZER /* don't include in kernel compilation due to SSE */
/* Decode queueing delay
 *
 * The code is left here so that this file is used as an API for
 * analyzer and can be swapped with other implementation during
 * compilation.
 *
 * Return value is in us
 */
u32 qdelay_decode(u32 value)
{
	/* Input value is originally time in ns right shifted 15 times
	 * to get division by 1000 and units of 32 us. The right shifting
	 * by 10 to do division by 1000 actually causes a rounding
	 * we correct by doing (x * (1024/1000)) here.
	 */
	return fl2int(value, QDELAY_M, QDELAY_E) * 32 * 1.024;
}
#endif
