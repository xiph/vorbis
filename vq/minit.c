/****************************** MINIT *****************************************
 * This file contains procedures to solve a Linear Programming
 * problem of n variables and m constraints, the last p of which
 * are equality constraints by the dual simplex method.
 *
 * The code was originally in Algol 60 and was published in Collected
 * Algorithms from CACM (alg #333) by Rudolfo C. Salazar and Subrata
 * K. Sen in 1968 under the title MINIT (MINimum ITerations) algorithm
 * for Linear Programming.
 * It was directly translated into C by Badri Lokanathan, Dept. of EE,
 * University of Rochester, with no modification to code structure. 
 * Monty <xiphmont@mit.edu> restructured the code in C style, eliminating
 * some verbosity and uneeded vector storage.
 * 
 * The problem statement is
 * Maximise z = cX
 *
 * subject to
 * AX <= b
 * X >=0
 *
 * c is a (1*n) row vector
 * A is a (m*n) matrix
 * b is a (m*1) column vector.
 * e is a matrix with with (m+1) rows and lcol columns (lcol = m+n-p+1)
 *   and forms the initial tableau of the algorithm.
 * td is read into the procedure and should be a very small number,
 *   say 1e-9
 * 
 * The condition of optimality is the non-negativity of
 * e[1,j] for j = 1 ... lcol-1 and of e[1,lcol] = 2 ... m+1.
 * If the e[i,j] values are greater than or equal to -td they are
 * considered to be non-negative. The value of td should reflect the 
 * magnitude of the co-efficient matrix.
 *
 *****************************************************************************/

/*** Vorbis notes: We don't actually use the equality solving code,
   but I couldn't bring myself to remove it.  This is such a nice
   little piece of code that I don't have the heart to cripple it and
   perhaps someone else will want an unneutered version in the future.

   Note that the equality code has *not* been well tested after my
   restructuring due to me not having any real problems to throw at it
   :-) --Monty 19991122 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "minit.h"

/************************ minit_init ******************************************
 * This routine performs space maintenance and initializes local arrays.     
 * n dimensions, m total constraints (lte and eq), p equality constraints 
 *****************************************************************************/

void minit_init(minit *l,long n,long m,long p){
  long i;
  double **tmp;
  long MS = m + 1;
  long LS = m + n - p + 1;

  memset(l,0,sizeof(minit));

  l->m = m;
  l->n = n;
  l->p = p;
  l->l = m + n - p + 1;
      
  l->ind1 = calloc(MS,sizeof(long));
  l->chk  = calloc(MS,sizeof(long));
  l->ind  = calloc(LS,sizeof(long));
      
  tmp=l->e=malloc(MS*sizeof(double *));
  for(i=0;i<MS;i++)
    *(tmp++)=calloc(LS,sizeof(double));
}

/************************ minit_clear *****************************************
 * This routine clears the non-flat storage fromt he local arrays.           
 *****************************************************************************/

void minit_clear(minit *l){
  long i;
  if(l){
    if(l->ind1){
      free(l->ind1);
      free(l->chk);
      free(l->ind);
      for(i = 0; i <= l->m; i++)free(l->e[i]);
      free(l->e);
    }
    memset(l,0,sizeof(minit));
  }
}

/****************************** rowtrans **************************************
 * Performs the usual tableau transformations in a linear programming
 * problem, (p,q) being the pivotal element.
 * Returns the following error codes:
 * 0: Everything was OK.
 * 1: No solution.
 *****************************************************************************/
static int rowtrans(minit *l,long p,long q){
  long i, j;
  double dummy;
  
  if(p == -1 || q == -1) return(1);
  dummy = l->e[p][q];

  /*if(debug)printf("--\nPivot element is e[%ld][%ld] = %f\n",p,q,dummy);*/

  for(j = 0; j < l->l; j++)l->e[p][j] /= dummy; 
  
  for(i = 0; i < l->m + 1; i++){
    if(i != p){
      if(l->e[i][q] != 0.0){
	dummy = l->e[i][q];
	for(j = 0; j < l->l; j++)
	  l->e[i][j] -= l->e[p][j] * dummy;
      }
    }
  }

  l->chk[p] = q;
  return(0);
} 

/****************************** progamma **************************************
 * Performs calculations over columns to determine the pivot element.
 *****************************************************************************/
static long progamma(minit *l,long L, double td, long *im, double *gmin){
  long jmin = -1;
  long   ll = l->l-1;
  long   i, L1;

  for(L1 = 0; L1 < L; L1++){
    long   ind  = l->ind[L1];
    long   imin = 0;
    double thmin;

    for(i = 1; i < l->m + 1; i++){
      if(l->e[i][ind] > td && l->e[i][ll] >= -td){
	double theta = l->e[i][ll] / l->e[i][ind];
	if(!imin || theta < thmin){
	  thmin = theta;
	  imin  = i;
	}
      }
    }

    if(imin){
      double gamma = thmin * l->e[0][ind];
      if(jmin==-1 || gamma<*gmin){
	 jmin = ind;
	*gmin = gamma;
	*im   = imin;
      }
    }
  }
  
  return jmin;
} 

/****************************** prophi ****************************************
 * Performs calculations over rows to determine the pivot element.
 *****************************************************************************/
static long prophi(minit *l,long k, double td,long *jm, double *phimax){
  long imax = -1;
  long ll   = l->l-1;
  long j, k1;
	
  for(k1 = 0; k1 < k ; k1++){
    long ind =l->ind1[k1];
    long jmax=-1;
    double delmax;
    
    for(j = 0; j < ll; j++){
      if(l->e[ind][j] < -td && l->e[0][j] >= -td){
	double delta = l->e[0][j] / l->e[ind][j];
	if(jmax<0 || delta > delmax){
	  delmax = delta;
	  jmax   = j;
	}
      }
    }
    
    if(jmax!=-1){
      double phi = delmax * l->e[ind][ll];
      if(imax==-1 || phi>*phimax){
	 imax   = ind;
	*phimax = phi;
	*jm     = jmax;
      }
    }
  }
  return imax;
}

/****************************** tab *******************************************
 * The following procedure is for debugging. It simply prints the
 * current tableau.
 *****************************************************************************/
static void tab(minit *l){
  long i, j;

  printf("\n");
  for(i = 0; i < 35; i++)
    printf("-");
  printf(" TABLEAU ");
  for(i = 0; i < 35; i++)
    printf("-");
  printf("\n");

  for(i = 0; i < l->m+1; i++){
    for(j = 0; j < l->l; j++)
      printf("%6.3f ",l->e[i][j]);
    
    printf("\n");
  }
}

/****************************** phase1 ****************************************
 * Applied only to equality constraints if any.
 *****************************************************************************/
static int phase1(minit *l,double td){
  long m=l->m,n=l->n,p=l->p,lcol=l->l;
  double gmin;
  long i, j, k, r;

  /* Fix suggested by Messham to allow negative coeffs. in
   * equality constraints.
   */
  for(i = m - p + 1; i < m + 1; i++)
    if(l->e[i][lcol - 1] < 0)
      for(j = 0; j < lcol; j++)
	l->e[i][j] = -l->e[i][j];

  for(r = 0; r < p; r++){
    long L    = 0;
    long im   = -1;
    long im1  = -1;
    long jmin = -1;
    long jmin1= -1;
    long first= 1;
    
    /* Fix suggested by Kolm and Dahlstrand */
    /* if(e[0,j] < 0) */
    for(j = 0; j < n; j++)
      if(l->e[0][j] < -td)
	l->ind[L++] = j;
    
    while(1){

      if(L == 0){
	for(j = 0; j < n; j++)
	  l->ind[j] = j;
	L = n;
      }
      
      /* I eliminated some local storage on the assumtion that it was
	 unneccessary... this checks that assumption ont he theory that
	 it's better to break than get the wrong answer */
      for(k = 0; k < L; k++){
	for(j=k+1;j < L; j++){
	  if(l->ind[k]==l->ind[j]){
	    printf("Internal error, failed assertion\n");
	    exit(1);
	  }
	}
      }
      
      for(k = 0; k < L; k++){
	long ind=l->ind[k];
	double thmin;
	long imin=-1;
	
	for(i = m - p + 1; i < m + 1; i++)
	  if(l->chk[i] == -1){
	    /* Fix suggested by Kolm
	     * and Dahlstrand
	     */
	    /* if(e[i][ind[k]] > 0.0) */
	    if(l->e[i][ind] > td){
	      double theta = l->e[i][lcol-1] / l->e[i][ind];
	      if(imin<0 || theta < thmin){
		thmin = theta;
		imin = i;
	      }
	    }
	  }
	
	/* Fix suggested by Obradovic overrides
	 * fixes suggested by Kolm and Dahstrand
	 * as well as Messham.
	 */
	if(imin!=-1){
	  double gamma = thmin * l->e[0][ind];
	  if(jmin==-1 || gamma < gmin){
	    jmin = ind;
	    gmin = gamma;
	    im   = imin;
	  }
	}
      }
      
      if(jmin == -1){
	if(first){
	  first = 0;
	  L = 0;
	  continue;
	}else
	  im = -1;
      }
      
      if(im == im1 && jmin == jmin1){
	L = 0;
	continue;
      }
      
      /* if(debug)tab(l);	*/
      
      if(rowtrans(l,im,jmin))return(1);
      
      im1 = im;
      jmin1 = jmin;
      break;
    }
  }
  return(0);
} 

/********************************* MINIT **************************************
 * This is the main procedure. It is invoked with the various matrices,
 * after space for other arrays has been generated elsewhere
 * It returns
 * 0 if everything went OK and x, w, z as the solutions.
 * 1 if no solution existed.
 * 2 if primal had no feasible solutions.
 * 3 if primal had unbounded solutions.
 *****************************************************************************/
int minit_solve(minit *l,double **A,double *b,double *c,double td,
	  double *x,double *w,double *z){
  long i, j;
  long m=l->m,n=l->n,p=l->p,lcol=l->l;
  long imax,jm,jmin,im;
  double phimax,gmin;

  /* Generate initial tableau. */
  for(j = 0; j < n; j++)
    l->e[0][j] = -c[j];

  for(i = 0; i < m; i++)
    for(j = 0; j < n; j++)
      l->e[i+1][j] = A[i][j];

  for(j = 0; j < m; j++)
    l->e[j+1][lcol-1] = b[j];

  for(i = 0; i < m - p; i++)
    l->e[1+i][n+i] = 1.0;

  /* Now begins the actual algorithm. */
  for(i = 1; i < m+1; i++)
    l->chk[i] = -1;

  if(p)
    if(phase1(l,td))return(1);

  while(1){
    long L=0,k=0;

    /*if(debug)tab(l);	*/
    
    for(j = 0; j < lcol - 1; j++)
      if(l->e[0][j] < -td)
	l->ind[L++] = j; /* ind[L] keeps track of the columns in which
			    e[0][j] is negative */
    
    for(i=1; i<m+1; i++)
      if(l->e[i][lcol-1] < -td)
	l->ind1[k++] = i; /* ind1[k] keeps track of the rows in which
			     e[i][lcol] is negative */

    if(L == 0){
      if(k == 0) break; /* results */

      if(k == 1){
	for(j = 0; j < lcol - 1; j++)
	  if(l->e[l->ind1[0]][j] < 0){
	    printf(":R:\n");
	    imax=prophi(l,k,td,&jm,&phimax);
	    if(rowtrans(l,imax,jm))return(1);
	    break;
	  }
	if(j==lcol-1)return(2);	/* Primal problem has no feasible
                                   solutions. */
      }else{
	printf(":R:\n");
	imax=prophi(l,k,td,&jm,&phimax);
	if(rowtrans(l,imax,jm))return(1);
      }
    }else{
      if(k==0){
	if(L == 1){
	  for(i = 1; i < m + 1; i++)
	    if(l->e[i][l->ind[0]] > 0){
	      printf(":C:\n");
	      jmin=progamma(l,L,td,&im,&gmin);
	      if(rowtrans(l,im,jmin))return(1);
	      break;
	    }
	  if(i==m+1)return(3);	/* Primal problem is unbounded. */
	}else{
	  printf(":C:\n");
	  jmin=progamma(l,L,td,&im,&gmin);
	  if(rowtrans(l,im,jmin))return(1);
	}
      }else{
	
	printf(":S:\n");
	jmin=progamma(l,L,td,&im,&gmin);
	imax=prophi(l,k,td,&jm,&phimax);

	if(jmin==-1){
	  if(rowtrans(l,imax,jm))return(1);
	}else if(imax==-1){
	  if(rowtrans(l,im,jmin))return(1);
	}else if(fabs(phimax) > fabs(gmin)){
	  if(rowtrans(l,imax,jm))return(1);
	}else{
	  if(rowtrans(l,im,jmin))return(1);
	}
      }
    }
  }

  /* Output results here. */
  if(z)*z = l->e[0][lcol-1];

  if(x){
    memset(x,0,n*sizeof(double));    
    for(i = 1; i < m + 1; i++){
      if(l->chk[i] < n)
	x[l->chk[i]] = l->e[i][lcol-1];
    }
  }
  
  if(w){
    memset(w,0,m*sizeof(double));
    for(j = n; j < lcol - 1; j++)
      w[j-n] = l->e[0][j];
  }

  return(0);
}

