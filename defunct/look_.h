/*  Last edited: Nov 18 18:38 1991 (rd) */

/* $Id: look_.h,v 1.1 1994-01-20 21:57:30 acedb Exp $ */
 
#ifndef DEFINE_LOOK__h
#define DEFINE_LOOK__h
 
#ifdef DEFINE_LOOK_h
This in an error,
look_.h must not be included after look.h,
ortherwise LOOK is defined twice.
#endif
 
#ifndef DEFINE_BS_h
typedef void *OBJ;
#endif
 
#ifndef DEF_ARRAY_H
typedef void * Array;
#endif
 
typedef struct LOOKSTUFF* LOOK;
typedef struct LOOKSTUFF
  {
    int   magic;       /* ==LOOKMAGIC*/
    KEY   key;
    OBJ   objet;
   void   (*Destroy)  (LOOK look) ;
   void   (*Draw)     (LOOK look) ;                /* redraw routine */
   void   (*Menu)     (LOOK look, KEY k) ;         /* k is chosen menu key */
   void   (*Pick)     (LOOK look, int k) ;         /* k is pick box number */
    int   type,         /* TREE or MAP */
          redraw,       /* Boolean */
          activebox ;
   void  *treeflip;     /* current flip node. NB Richard's change here */
    int   mapflip;      /* maps have a different flipper simply based on box
                           numbers */
    Array content,      /* co



