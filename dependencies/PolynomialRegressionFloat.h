#ifndef _POLYNOMIAL_REGRESSION_FLOAT_H
#define _POLYNOMIAL_REGRESSION_FLOAT_H
  #ifndef TYPE_PR
  #define TYPE_PR float
  #endif
/**
 * This is a Polynomial Regression based on Chris Engelsma algorithms however
 * simplified. It is destined to use within embeded systems without support
 * of stl and vector libraries. Above define: TYPE_PR defines the basic type
 * for calculations and arrays used.
 * It was also changed into a function working on pointers as Chrises classed
 * version was expensive to use in terms of memory usage for runtime obtained
 * datasets.
 * Consider uncommenting "#include <cmath>" depending on your enviroment
 *
 * LICENSE: MIT License
 *
 * Copyright (c) 09.2021 Gen0me
 *
 * @author Gen0me
 *
 ******************************************************************************
 *
 * PURPOSE:
 *
 *  Polynomial Regression aims to fit a non-linear relationship to a set of
 *  points. It approximates this by solving a series of linear equations using
 *  a least-squares approach.
 *
 *  We can model the expected value y as an nth degree polynomial, yielding
 *  the general polynomial regression model:
 *
 *  y = a0 + a1 * x + a2 * x^2 + ... + an * x^n
 *
 * LICENSE:
 *
 * MIT License
 *
 * Copyright (c) 2020 Chris Engelsma
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author Chris Engelsma
 */

#include <stdlib.h>
//#include <cmath>

/// Here is how this function works + REQUIREMENTS FOR SAFE MEMORY USAGE
/// x,y are pointers to arrays of datapoints, datapoints_num is the size of array underneath x (=y)
/// order is the order of searched polynomial. coeffs has to point to an array size of (order+1)
/// coeffs after function finishes stores coefficients. c + ax + ax^2 ... + ax^order
/// function returns sum of rests squares
TYPE_PR PolynomialRegression(TYPE_PR * x, TYPE_PR * y, int datapoints_num, int order, TYPE_PR * coeffs)
{
  for (int i=0; i<order; i++)
    coeffs[i] = 0;
  size_t N = datapoints_num;
  int n = order;
  int np1 = n + 1;
  int np2 = n + 2;
  int tnp1 = 2 * n + 1;
  TYPE_PR tmp;

  // X = vector that stores values of sigma(xi^2n)
  TYPE_PR X[tnp1]; //TYPE_PR * X = (TYPE_PR*)malloc (tnp1*sizeof(TYPE_PR));
  for (int i = 0; i < tnp1; ++i) {
    X[i] = 0;
    for (int j = 0; j < N; ++j)
      X[i] += (TYPE_PR)pow(x[j], i);
  }

  // B = normal augmented matrix that stores the equations.
  TYPE_PR B[np1][np2];

  for (int i = 0; i <= n; ++i)
    for (int j = 0; j <= n; ++j)
      B[i][j] = X[i + j];

  // Y = vector to store values of sigma(xi^n * yi)
  TYPE_PR Y[np1];
  for (int i = 0; i < np1; ++i) {
    Y[i] = (TYPE_PR)0;
    for (int j = 0; j < N; ++j) {
      Y[i] += (TYPE_PR)pow(x[j], i)*y[j];
    }
  }

  // Load values of Y as last column of B
  for (int i = 0; i <= n; ++i)
    B[i][np1] = Y[i];

  n += 1;
  int nm1 = n-1;

  // Pivotisation of the B matrix.
  for (int i = 0; i < n; ++i)
    for (int k = i+1; k < n; ++k)
      if (B[i][i] < B[k][i])
        for (int j = 0; j <= n; ++j) {
          tmp = B[i][j];
          B[i][j] = B[k][j];
          B[k][j] = tmp;
        }

  // Performs the Gaussian elimination.
  // (1) Make all elements below the pivot equals to zero
  //     or eliminate the variable.
  for (int i=0; i<nm1; ++i)
    for (int k =i+1; k<n; ++k) {
      TYPE_PR t = B[k][i] / B[i][i];
      for (int j=0; j<=n; ++j)
        B[k][j] -= t*B[i][j];         // (1)
    }

  // Back substitution.
  // (1) Set the variable as the rhs of last equation
  // (2) Subtract all lhs values except the target coefficient.
  // (3) Divide rhs by coefficient of variable being calculated.
  for (int i=nm1; i >= 0; --i) {
    coeffs[i] = B[i][n];                   // (1)
    for (int j = 0; j<n; ++j)
      if (j != i)
        coeffs[i] -= B[i][j] * coeffs[j];       // (2)
    coeffs[i] /= B[i][i];                  // (3)
  }
//  free (X);

  TYPE_PR sum_square_rest = 0;
  TYPE_PR tmp_rest;
  for (int k=0; k<datapoints_num; k++){
    tmp_rest = 0;
    for (int i=0; i<=order; i++){
      tmp_rest += coeffs[i] * pow(x[k],i);
    }
     sum_square_rest += pow (y[k] - tmp_rest, 2);
  }

  return sum_square_rest;
}
#endif
