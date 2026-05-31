#ifndef _T2G_EASING_H_
#define _T2G_EASING_H_

double clamp01(double t);
double lerp(double a, double b, double t);
double ease_in_out_cubic(double t);
double ease_out_cubic(double t);
double ease_in_cubic(double t);
double ease_out_elastic(double t);
double ease_out_bounce(double t);
/* Smoothest standard easing — used for transitions */
double ease_in_out_sine(double t);

#endif
