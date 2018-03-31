//
// MATLAB Compiler: 6.1 (R2015b)
// Date: Fri Mar 30 12:34:53 2018
// Arguments: "-B" "macro_default" "-W" "cpplib:mlsq" "-T" "link:lib" "-d"
// "D:\Matlab2015b_install\bin\mlsq\for_testing" "-v"
// "D:\Matlab2015b_install\bin\m_lsqlin.m" 
//

#ifndef __mlsq_h
#define __mlsq_h 1

#if defined(__cplusplus) && !defined(mclmcrrt_h) && defined(__linux__)
#  pragma implementation "mclmcrrt.h"
#endif
#include "mclmcrrt.h"
#include "mclcppclass.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__SUNPRO_CC)
/* Solaris shared libraries use __global, rather than mapfiles
 * to define the API exported from a shared library. __global is
 * only necessary when building the library -- files including
 * this header file to use the library do not need the __global
 * declaration; hence the EXPORTING_<library> logic.
 */

#ifdef EXPORTING_mlsq
#define PUBLIC_mlsq_C_API __global
#else
#define PUBLIC_mlsq_C_API /* No import statement needed. */
#endif

#define LIB_mlsq_C_API PUBLIC_mlsq_C_API

#elif defined(_HPUX_SOURCE)

#ifdef EXPORTING_mlsq
#define PUBLIC_mlsq_C_API __declspec(dllexport)
#else
#define PUBLIC_mlsq_C_API __declspec(dllimport)
#endif

#define LIB_mlsq_C_API PUBLIC_mlsq_C_API


#else

#define LIB_mlsq_C_API

#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_mlsq_C_API 
#define LIB_mlsq_C_API /* No special import/export declaration */
#endif

extern LIB_mlsq_C_API 
bool MW_CALL_CONV mlsqInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_mlsq_C_API 
bool MW_CALL_CONV mlsqInitialize(void);

extern LIB_mlsq_C_API 
void MW_CALL_CONV mlsqTerminate(void);



extern LIB_mlsq_C_API 
void MW_CALL_CONV mlsqPrintStackTrace(void);

extern LIB_mlsq_C_API 
bool MW_CALL_CONV mlxM_lsqlin(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__BORLANDC__)

#ifdef EXPORTING_mlsq
#define PUBLIC_mlsq_CPP_API __declspec(dllexport)
#else
#define PUBLIC_mlsq_CPP_API __declspec(dllimport)
#endif

#define LIB_mlsq_CPP_API PUBLIC_mlsq_CPP_API

#else

#if !defined(LIB_mlsq_CPP_API)
#if defined(LIB_mlsq_C_API)
#define LIB_mlsq_CPP_API LIB_mlsq_C_API
#else
#define LIB_mlsq_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_mlsq_CPP_API void MW_CALL_CONV m_lsqlin(int nargout, mwArray& x, mwArray& resnorm, mwArray& residual, const mwArray& A, const mwArray& b);

#endif
#endif
