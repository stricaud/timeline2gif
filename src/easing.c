#include <math.h>
#include "easing.h"

double clamp01(double t)
{
	return t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
}

double lerp(double a, double b, double t)
{
	return a + (b - a) * t;
}

double ease_in_out_cubic(double t)
{
	return t < 0.5 ? 4*t*t*t : 1.0 - pow(-2*t + 2, 3) / 2.0;
}

double ease_out_cubic(double t)
{
	return 1.0 - pow(1.0 - t, 3);
}

double ease_in_cubic(double t)
{
	return t * t * t;
}

double ease_out_elastic(double t)
{
	if (t <= 0.0) return 0.0;
	if (t >= 1.0) return 1.0;
	double c4 = (2.0 * M_PI) / 3.0;
	return pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * c4) + 1.0;
}

double ease_out_bounce(double t)
{
	double n1 = 7.5625, d1 = 2.75;
	if (t < 1.0 / d1) {
		return n1 * t * t;
	} else if (t < 2.0 / d1) {
		t -= 1.5 / d1;
		return n1 * t * t + 0.75;
	} else if (t < 2.5 / d1) {
		t -= 2.25 / d1;
		return n1 * t * t + 0.9375;
	} else {
		t -= 2.625 / d1;
		return n1 * t * t + 0.984375;
	}
}
