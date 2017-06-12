#ifndef GMATH_H
#define GMATH_H

#include "matrix.h"

double *calculate_normal(struct matrix *polygons, int i);
void normalize(double * vector);

#endif
