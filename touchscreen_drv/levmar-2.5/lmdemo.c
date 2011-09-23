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
void gaussian_fit5(double *p, double *x, int m, int n, void *data)
{
	int j, k;
	int t[5];
	t[0] = -2;
	t[1] = -1;
	t[2] = 0;
	t[3] = 1;
	t[4] = 2;
	for(j=0; j<5; ++j)
		for(k=0; k<5; ++k) {
			x[j*5+k]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD));
		}
} 

void gaussian_fit3(double *p, double *x, int m, int n, void *data)
{
	int j, k;
	int t[3];
	t[0] = -1;
	t[1] = 0;
	t[2] = 1;
	for(j=0; j<3; ++j)
		for(k=0; k<3; ++k) {
			x[j*3+k]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD));
		}
} 

void jac_gaussian_fit5(double *p, double *jac, int m, int n, void *data)
{
	int l;
	int j, k;
	int t[5];
	t[0] = -2;
	t[1] = -1;
	t[2] = 0;
	t[3] = 1;
	t[4] = 2;
	for(j=0; j<5; ++j)
		for(k=0; k<5; ++k) {
			jac[l++]=exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD));
			jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*2*(t[j]-p[1])/(2*SD*SD);
			jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*2*(t[k]-p[2])/(2*SD*SD);
		//	jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*(((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(
		}
} 

void jac_gaussian_fit3(double *p, double *jac, int m, int n, void *data)
{
	int l;
	int j, k;
	int t[3];
	t[0] = -1;
	t[1] = 0;
	t[2] = 1;
	for(j=0; j<3; ++j)
		for(k=0; k<3; ++k) {
			jac[l++]=exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD));
			jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*2*(t[j]-p[1])/(2*SD*SD);
			jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*2*(t[k]-p[2])/(2*SD*SD);
		//	jac[l++]=p[0]*exp(-((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(t[k]-p[2]))/(2*SD*SD))*(((t[j]-p[1])*(t[j]-p[1]) + (t[k]-p[2])*(
		}
} 

int runlm(int radius, double *p, double *x, double *r)
{
register int i, j;
int problem, ret;
int m, n;
double opts[LM_OPTS_SZ], info[LM_INFO_SZ];


  opts[0]=LM_INIT_MU; opts[1]=1E-15; opts[2]=1E-15; opts[3]=1E-20;
  opts[4]= LM_DIFF_DELTA; // relevant only if the Jacobian is approximated using finite differences; specifies forward differencing 
  //opts[4]=-LM_DIFF_DELTA; // specifies central differencing to approximate Jacobian; more accurate but more expensive to compute!

  m=3; 
  
  if(radius == 2)
  {
 	n=25;
  	ret=dlevmar_der(gaussian_fit5, jac_gaussian_fit5, p, x, m, n, 500, opts, info, NULL, NULL, NULL);	
  }
  if(radius == 1)
  {
 	n=9;
  	ret=dlevmar_der(gaussian_fit3, jac_gaussian_fit3, p, x, m, n, 500, opts, info, NULL, NULL, NULL);	
  }

  r[0] = p[1]; r[1] = p[2];
  return 0;
}
