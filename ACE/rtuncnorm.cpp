/*
 * sexp_macros.h - helper macros for SEXPs
 *
 * Collection of useful macros to handle S expressions. Most of these
 * are used to unpack arguments passed in via the .Call() or
 * .External() interface.
 */
#if !defined(__SEXP_MACROS_H__)
#define __SEXP_MACROS_H__

#include <R.h>
#include <Rcpp.h>
#include <Rdefines.h>
#include <Rmath.h>
#include <Rinternals.h>
#include <R_ext/Applic.h>
#include <float.h>
#include <R_ext/Lapack.h>
#include <R_ext/BLAS.h>


#define CHECK_ARG_IS_REAL_MATRIX(A)					\
if (!isReal(A) || !isMatrix(A))					    \
  error("Argument '" #A "' is not a real matrix.");

#define CHECK_ARG_IS_REAL_VECTOR(A)					\
if (!isReal(A) || !isVector(A))					    \
  error("Argument '" #A "' is not a real vector.");

#define CHECK_ARG_IS_INT_VECTOR(A)					\
if (!isInteger(A) || !isVector(A))					\
  error("Argument '" #A "' is not an integer vector.");

/*
* Unpack a real matrix stored in SEXP S. 
*/
#define UNPACK_REAL_MATRIX(S, D, N, K)          \
CHECK_ARG_IS_REAL_MATRIX(S);		                  \
double *D = REAL(S);			                         \
const R_len_t N = nrows(S);			                  \
const R_len_t K = ncols(S);

/*
* Unpack a real vector stored in SEXP S.
*/
#define UNPACK_REAL_VECTOR(S, D, N)             \
CHECK_ARG_IS_REAL_VECTOR(S);		                  \
double *D = REAL(S);			                         \
const R_len_t N = length(S);                   

/*
* Unpack a single real stored in SEXP S.
*/
#define UNPACK_REAL(S, D)			  \
CHECK_ARG_IS_REAL_VECTOR(S);		\
double D = REAL(S)[0];			     \

/*
 * Unpack an integer vector stored in SEXP S.
 */
#define UNPACK_INT_VECTOR(S, I, N)             \
CHECK_ARG_IS_INT_VECTOR(S);		                  \
int *I = INTEGER(S);		                         \
const R_len_t N = length(S);                   

/*
 * Unpack a single integer stored in SEXP S.
 */
#define UNPACK_INT(S, I)			   \
CHECK_ARG_IS_INT_VECTOR(S);			\
int I = INTEGER(S)[0];			     \

#endif
#define ALLOC_REAL_VECTOR(S, D, N)		         \
SEXP S;					                                 \
PROTECT(S = allocVector(REALSXP, N));	       \
double *D = REAL(S);

#ifndef MAX
#define MAX(A, B) ((A>B)?(A):(B))
#endif



#ifdef DEBUG
#define SAMPLER_DEBUG(N, A, B) Rprintf("%8s(%f, %f)\n", N, A, B)
#else
#define SAMPLER_DEBUG(N, A, B) 
#endif

static const double t1 = 0.15;
static const double t2 = 2.18;
static const double t3 = 0.725;
static const double t4 = 0.45;

/* Exponential rejection sampling (a,inf) */
// [[Rcpp::export]]
static R_INLINE double ers_a_inf(double a) {
  SAMPLER_DEBUG("ers_a_inf", a, R_PosInf);
  const double ainv = 1.0 / a;
  double x, rho;
  do{
    x = R::rexp(ainv) + a; /* rexp works with 1/lambda */
rho = exp(-0.5 * pow((x - a), 2));
  } while (R::runif(0, 1) > rho);
  return x;
}

/* Exponential rejection sampling (a,b) */
static R_INLINE double ers_a_b(double a, double b) {
  SAMPLER_DEBUG("ers_a_b", a, b);
  const double ainv = 1.0 / a;
  double x, rho;
  do{
    x = R::rexp(ainv) + a; /* rexp works with 1/lambda */
rho = exp(-0.5 * pow((x-a), 2));
  } while (R::runif(0, 1) > rho || x > b);
  return x;
}

/* Normal rejection sampling (a,b) */
static R_INLINE double nrs_a_b(double a, double b){
  SAMPLER_DEBUG("nrs_a_b", a, b);
  double x = -DBL_MAX;
  while(x < a || x > b){
    x = R::rnorm(0, 1);
  }
  return x;
}

/* Normal rejection sampling (a,inf) */
static R_INLINE double nrs_a_inf(double a){
  SAMPLER_DEBUG("nrs_a_inf", a, R_PosInf);
  double x = -DBL_MAX;
  while(x < a){
    x = R::rnorm(0, 1);
  }
  return x;
}

/* Half-normal rejection sampling */
double hnrs_a_b(double a, double b){
  SAMPLER_DEBUG("hnrs_a_b", a, b);    
  double x = a - 1.0;
  while(x < a || x > b) {
    x = R::rnorm(0, 1);
    x = fabs(x);
  }
  return x;
}

/* Uniform rejection sampling */
static R_INLINE double urs_a_b(double a, double b){
  SAMPLER_DEBUG("urs_a_b", a, b);
  const double phi_a = R::dnorm(a, 0.0, 1.0, FALSE);
  double x = 0.0;//, u = 0.0;
  
  /* Upper bound of normal density on [a, b] */
  const double ub = a < 0 && b > 0 ? M_1_SQRT_2PI : phi_a;
  do{
    x = R::runif(a, b);
  } while (R::runif(0, 1) * ub > R::dnorm(x,0,1,0));
  return x;
}

/* Previously this was refered to as type 1 sampling: */
static inline double r_lefttruncnorm(double a, double mean, double sd) {
  const double alpha = (a - mean) / sd;
  if (alpha < t4) {
    return mean + sd * nrs_a_inf(alpha);
  } else {
    return mean + sd * ers_a_inf(alpha);
  }
}

static R_INLINE double r_righttruncnorm(double b, double mean, double sd) {
  const double beta = (b - mean) / sd;
  /* Exploit symmetry: */
  return mean - sd * r_lefttruncnorm(-beta, 0.0, 1.0);
}

static R_INLINE double r_truncnorm(double a, double b, double mean, double sd) {
  const double alpha = (a - mean) / sd;
  const double beta = (b - mean) / sd;
  const double phi_a = R::dnorm(alpha, 0.0, 1.0, FALSE);
  const double phi_b = R::dnorm(beta, 0.0, 1.0, FALSE);
  if (beta <= alpha) {
    return NA_REAL;
  } else if (alpha <= 0 && 0 <= beta) { /* 2 */
  if (phi_a <= t1 || phi_b <= t1) { /* 2 (a) */
  return mean + sd * nrs_a_b(alpha, beta);
  } else { /* 2 (b) */
  return mean + sd * urs_a_b(alpha, beta);
  }
  } else if (alpha > 0) { /* 3 */
  if (phi_a / phi_b <= t2) { /* 3 (a) */
  return mean + sd * urs_a_b(alpha, beta);
  } else {
    if (alpha < t3) { /* 3 (b) */                
  return mean + sd * hnrs_a_b(alpha, beta);
    } else { /* 3 (c) */
  return mean + sd * ers_a_b(alpha, beta);
    }
  }
  } else { /* 3s */
  if (phi_b / phi_a <= t2) { /* 3s (a) */
  return mean - sd * urs_a_b(-beta, -alpha);
  } else {
    if (beta > -t3) { /* 3s (b) */
  return mean - sd * hnrs_a_b(-beta, -alpha);
    } else { /* 3s (c) */
  return mean - sd * ers_a_b(-beta, -alpha);
    }
  }
  }
}

//[[Rcpp::export]]
SEXP do_rtruncnorm(SEXP s_n, SEXP s_a, SEXP s_b, SEXP s_mean, SEXP s_sd) {
  R_len_t i, nn;
  UNPACK_INT(s_n, n);
  if (NA_INTEGER == n)
    error("n is NA - aborting.");
  UNPACK_REAL_VECTOR(s_a   , a   , n_a);
  UNPACK_REAL_VECTOR(s_b   , b   , n_b);
  UNPACK_REAL_VECTOR(s_mean, mean, n_mean);
  UNPACK_REAL_VECTOR(s_sd  , sd  , n_sd);
  
  nn = MAX(n, MAX(MAX(n_a, n_b), MAX(n_mean, n_sd)));
  ALLOC_REAL_VECTOR(s_ret, ret, nn);
  
  GetRNGstate();
  for (i = 0; i < nn; ++i) {
    const double ca = a[i % n_a];
    const double cb = b[i % n_b];
    const double cmean = mean[i % n_mean];
    const double csd = sd[i % n_sd];
    
    if (R_FINITE(ca) && R_FINITE(cb)) {
      ret[i] = r_truncnorm(ca, cb, cmean, csd);
    } else if (R_NegInf == ca && R_FINITE(cb)) {
      ret[i] = r_righttruncnorm(cb, cmean, csd);
    } else if (R_FINITE(ca) && R_PosInf == cb) {
      ret[i] = r_lefttruncnorm(ca, cmean, csd);
    } else if (R_NegInf == ca && R_PosInf == cb) {
      ret[i] = R::rnorm(cmean, csd);
    } else {
      ret[i] = NA_REAL;
    }
    R_CheckUserInterrupt();
  }
  PutRNGstate();
  UNPROTECT(1); /* s_ret */
  return s_ret;
}