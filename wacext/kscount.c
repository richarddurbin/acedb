#include "../wac/ac.h"

int main (int argc, char *argv[])
{
  char *err = 0 ;
  char *target = "a:annie:2000101" ;
  AC_DB db = 0 ;
  AC_ITER iter ;
  AC_OBJ k ;
  AC_KEYSET ks ;

  if (argc > 1)
    target = argv[1] ;
  db = ac_open (target , &err) ;
  if (err)
    printf ("Error message %s", err) ;
  if (!db)
    messcrash ("could not open %s", target) ;

  iter = ac_query_iter (db, 0, "find keyset __*", 0, 0) ;
  while ((k = ac_next (iter)))
    {
      ks = ac_query_keyset (db, messprintf("Find keyset %s; expand", ac_name(k)), 0, 0) ;
      printf ("%s\t%d\n", ac_name(k), ac_keyset_count (ks)) ;
      ac_free (ks) ;
      ac_free (k) ;      
    }
  ac_free (iter) ;
  ac_close (db) ;
}
