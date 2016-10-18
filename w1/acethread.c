#include "regular.h"
#include "acethread.h"

static Array mutexArray = 0 ;
typdef struct AceThreadStruct 
{ Array mArray ; } ;


BOOL getMutex (TH th, int nn)
{
  pthread_mutex_t *mm = 0 ;
  int n1, n2 ;

  if (!th) return TRUE ;

  if (mutexArray)
    messcrash ("getMutex called before threadMutexInit") ;
  if (nn >= arrayMax(mutexArray))
    messcrash ("getMutex call on excessive nn=%d", nn) ;
  
  if (!th->mArray)
    th->mArray = arrayCreate (12, int) ;
  n1 = arrayMax(th->mArray) ;
  n2 = n1 ? array(th->mArray, n1 - 1, int) : 0 ;
  if (nn < n2)
    messcrash ("Requesting a lower mutex") ;
  if (nn > n2)
    {
      array(th->mArray, arrayMax(th->mArray), int) = nn ;
      
      mm = arrayp (mutexArray, nn, pthread_mutex_t) ;
      if (!mm)
	pthread_mutex_init (mm, 0) ;
      pthread_mutex_lock (mm) ;
    }
  
  return TRUE ;
}

BOOL releaseMutex (TH th, int nn)
{
  int n1, n2 ;

  if (!th) return TRUE ;

  if (mutexArray)
    messcrash ("releaseMutex called before threadMutexInit") ;
  if (nn >= arrayMax(mutexArray))
    messcrash ("releaseMutex call on excessive nn=%d", nn) ;
  
  if (!th->mArray)
    th->mArray = arrayCreate (12, int) ;
  n1 = th->mArray ? arrayMax(th->mArray) : 0 ;
  n2 = n1 ? array(th->mArray, n1 - 1, int) : 0 ;
  if (nn > n2)
    messcrash ("Releasing an unlocked mutex") ;
  if (nn < n2) 
    messcrash ("Releasing in disorder") ;
  arrayMax(th->mArray)-- ;

  mm = arrayp (mutexArray, nn, pthread_mutex_t) ;
  if (!mm)
    messcrash ("releasing a non initialised mutex") ;
  pthread_mutex_unlock (mm) ;
  
  return TRUE ;
}

static void threadMutextInit (void)
{
  if (mutexArray)
    messcrash ("Double call to threadMutextInit") ;

  mutexArray = arrayCreate (64, pthread_mutex_t) ;
  array (mutexArray, LASTMUTEX, pthread_mutex_t) = 0 ;
}

void aceThreadInit (void)
{
  threadMutextInit () ;
}
