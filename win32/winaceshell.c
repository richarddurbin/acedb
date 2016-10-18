/* Wrapper for use in winscripts scripts. */

/* Function:
   1) Convert arg1 from POSIX path to windows path
   2) Pass to windows shell for opening  with remaining args unchanged.
   3) If the $ACEDB environment variable is set, set the current
      directory to that before the call.

   This allows, (for instance) the currently registered picture-displaying
   application to be used for pictures simply by opening the jpegs or gifs
   directly.
*/
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cygwin.h>
#include <windows.h>

int main(int argc, char **argv)
{
  FILE *f;
  char *path = (char *)malloc(MAX_PATH);
  char *cwd = (char *)malloc(MAX_PATH);
  char *args;
  int i, tot;
  
  if (argc < 2)
    exit (1);
  
  for (tot = 0,i = 2; i< argc ; i++)
    tot += strlen(argv[i])+1;
  
  args = (char *)malloc(tot);
  *args  = 0;

  for (tot = 0,i = 2; i< argc ; i++)
    {
      strcat(args, argv[i]);
      strcat(args, " ");
    }

  cygwin_conv_to_full_win32_path(argv[1], path);

  if (getenv ("ACEDB"))
    cygwin_conv_to_full_win32_path(getenv("ACEDB"), cwd);
  else
    cwd = 0;

  ShellExecute(0, "open", path, args, cwd, 0);

  return 1;


}





