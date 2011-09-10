/////////////////////////////////////////////////////////////////////////////////
// 
//  Demonstration driver program for the Levenberg - Marquardt minimization
//  algorithm
//  Copyright (C) 2004-05  Manolis Lourakis (lourakis at ics forth gr)
//  Institute of Computer Science, Foundation for Research & Technology - Hellas
//  Heraklion, Crete, Greece.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
/////////////////////////////////////////////////////////////////////////////////

/******************************************************************************** 
 * Levenberg-Marquardt minimization demo driver. Only the double precision versions
 * are tested here. See the Meyer case for an example of verifying the Jacobian 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "levmar.h"2

#define SD .76

#ifndef LM_DBL_PREC
#error Demo program assumes that levmar has been compiled with double precision, see LM_DBL_PREC!
#endif
void gaussian_fit(double *p, double *x, int m, int n, void *data)
{
int j, k;
int t[5];
t[0] = -2;
t[1] = -1;
t[2] = 0;
t[3] = 1;
t[4] = 2;
//printf("Parameters are: %.7g %.7g %.7g %.7g\n", p[0], p[1], p[2], p[3]);
//printf("Fitted values: ");
	for(j=0; j<5; ++j)
		for(k=0; k<5; ++k) {
			x[j*5+k]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD));
			//printf("%.7g,",x[j*3+k]);
			}
//printf("\n");
} 

void jac_gaussian_fit(double *p, double *jac, int m, int n, void *data)
{
int l;
int j, k;
int t[5];
t[0] = -2;
t[1] = -1;
t[2] = 0;
t[3] = 1;
t[4] = 2;
//printf("Parameters are: %.7g %.7g %.7g %.7g\n", p[0], p[1], p[2], p[3]);
//printf("Fitted values: ");
	for(j=0; j<5; ++j)
		for(k=0; k<5; ++k) {
			jac[l++]=exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD));
			jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*2*(t[j]-p[1])/(2*SD*SD);
			jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*2*(t[k]-p[2])/(2*SD*SD);
		//	jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*(((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2])))/(SD*SD*SD);
	//		x[j*3+k]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*p[3]*p[3]));
			//printf("%.7g,",x[j*3+k]);
			}
//printf("\n");
} 

/* Meyer's (reformulated) problem, minimum at (2.48, 6.18, 3.45) */
void meyer(double *p, double *x, int m, int n, void *data)
{
register int i;
double ui;
printf("Parameters are: %.7g %.7g %.7g %.7g\n", p[0], p[1], p[2], p[3]);

	for(i=0; i<n; ++i){
		ui=0.45+0.05*i;
		x[i]=p[0]*exp(10.0*p[1]/(ui+p[2]) - 13.0);
	}
}

void jacmeyer(double *p, double *jac, int m, int n, void *data)
{
register int i, j;
double ui, tmp;

  for(i=j=0; i<n; ++i){
	  ui=0.45+0.05*i;
	  tmp=exp(10.0*p[1]/(ui+p[2]) - 13.0);

	  jac[j++]=tmp;
	  jac[j++]=10.0*p[0]*tmp/(ui+p[2]);
	  jac[j++]=-10.0*p[0]*p[1]*tmp/((ui+p[2])*(ui+p[2]));
  }
}

/* Osborne's problem, minimum at (0.3754, 1.9358, -1.4647, 0.0129, 0.0221) */
void osborne(double *p, double *x, int m, int n, void *data)
{
register int i;
double t;

	for(i=0; i<n; ++i){
    t=10*i;
    x[i]=p[0] + p[1]*exp(-p[3]*t) + p[2]*exp(-p[4]*t);
	}
}

void jacosborne(double *p, double *jac, int m, int n, void *data)
{
register int i, j;
double t, tmp1, tmp2;

  for(i=j=0; i<n; ++i){
    t=10*i;
	  tmp1=exp(-p[3]*t);
    tmp2=exp(-p[4]*t);

	  jac[j++]=1.0;
	  jac[j++]=tmp1;
	  jac[j++]=tmp2;
    jac[j++]=-p[1]*t*tmp1;
    jac[j++]=-p[2]*t*tmp2;
  }
}

int runlm(double *p, double *x, double *r)
{
register int i, j;
int problem, ret;
int m, n;
double opts[LM_OPTS_SZ], info[LM_INFO_SZ];


  opts[0]=LM_INIT_MU; opts[1]=1E-15; opts[2]=1E-15; opts[3]=1E-20;
  opts[4]= LM_DIFF_DELTA; // relevant only if the Jacobian is approximated using finite differences; specifies forward differencing 
  //opts[4]=-LM_DIFF_DELTA; // specifies central differencing to approximate Jacobian; more accurate but more expensive to compute!

    m=3; n=25;

 //   printf("About to call dlevmar_dif:\n"); 	
//    ret=dlevmar_dif(gaussian_fit, p, x, m, n, 500, opts, info, NULL, NULL, NULL); // no Jacobian, caller allocates work memory, covariance estimated
    ret=dlevmar_der(gaussian_fit, jac_gaussian_fit, p, x, m, n, 500, opts, info, NULL, NULL, NULL);	
 //   printf("dlevmar_dif finished");
  
//  printf("Levenberg-Marquardt returned %d in %g iter, reason %g\nSolution: ", ret, info[5], info[6]);
//  for(i=0; i<m; ++i)
//    printf("%.7g ", p[i]);
//  printf("\n\nMinimization info:\n");
//  for(i=0; i<LM_INFO_SZ; ++i)
//    printf("%g ", info[i]);
//  printf("\n");
	r[0] = p[1]; r[1] = p[2];
  return 0;
}
