#ifndef vTXT_H_DEF
#define  vTXT_H_DEF

#include "regular.h"
#include "array.h"

typedef Stack vTXT ;
vTXT vtxtCreate (void) ;
vTXT vtxtHandleCreate (STORE_HANDLE h) ;
#define vtxtDestroy(_vtxt) stackDestroy(_vtxt)

BOOL  vtxtClear (vTXT blkp) ; /* clears the content */
char *vtxtPtr (vTXT blkp) ; /* the content */
char *vtxtAt (vTXT blkp, int pos) ; /* the content strating at pos*/
int vtxtLen (vTXT blkp) ; /* the current length */
int vtxtPrint (vTXT s, char *txt) ; /* unformatted, uninterpreted, appends to what is already there */
int vtxtPrintf (vTXT s, char * formatDescription, ...) ; /* here an above, return an int valid for vtxtAt */
char *vtxtPrintWrapped (vTXT s, char *text, int lineLength) ; 
int vtxtReplaceString (vTXT vtxt, char *a, char *b) ; /* changes a into b, returns the number of replaced strings */
int vtxtRemoveBetween (char *txt, char *begin, char *end) ; /* removes all occurences of begin...end, returns the number of removed strings */

vTXT vtxtGetURL (char *urlToGet, int timeOut) ; /* get the content of the url page, in libtcpfn.a */

void vtextUpperCase (char *blkp) ; /* acts on a normal char buffer, uppers all letters */
void vtextLowerCase (char *blkp) ; /* acts on a normal char buffer, lowers all letters */
void vtextCollapseSpaces (char *buf) ; /* change \n\t into space, collapse multiple spaces into single one */
void vtextReplaceSymbol (char *blkp, char a, char b) ; /* change a into b in the whole of buf */


#endif
