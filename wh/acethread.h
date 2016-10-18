#ifndef ACETHREAD_DEF
#define ACETHREAD_DEF

typdef struct AceThreadStruct *TH ;

typedef enum 
{
  ZERO=0,
  Adisk_Save_all_Mutex,
  Adisk_Bat_Mutex,
  Adisk_NewP_Mutex,
} ACE_MUTEX;

void aceThreadInit (void) ;

BOOL getMutex (TH th, int nn) ;
BOOL releaseMutex (TH th, int nn) ;

#endif /* ACETHREAD_DEF */
