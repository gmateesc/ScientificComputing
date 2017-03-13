#ifndef _LOG_H_
#define _LOG_H_

/* for main.c */
#define fprintf_ //

/* for sff.c  */
#define fprintf_s //

/* for match.c  */
#define fprintf_m //

#define fprintf_x fprintf


#ifdef DEBUG2
  #define fprintf2_ fprintf
#else
  #define fprintf2_ //
#endif

#endif

