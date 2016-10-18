/* Adapter code for acedb on windows.
   First arg is (win style) path to acedb binary (xace or tace)
   Second arg is (win style) path to .adb file, which has the format:
   [ACEDB]
   Path=c:/maydb
   extract path from this.
   Convert both path to binary and path to db into POSIX form
   and call acedb.
   If a third arg is given, this is (win style) path to rxvt.
   If given call acedb as a sub-process of rxvt t for tace or xace console. */


#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/cygwin.h>
#include <windows.h>

char rxvt[MAX_PATH];
char ace[MAX_PATH];
char db[MAX_PATH];
char dbdir_posix[MAX_PATH];
char install_dir[MAX_PATH];

char *rxvtargv[] = { 
  "rxvt",
  "-fn",
  NULL,
  "-sl",
  "10000",
  "-T",
  "Acedb",
  "-e",
  NULL,
  NULL,
  NULL
};

char *bareargv[] = { 
  NULL,
  NULL,
  NULL
};

void getInstallDir(void)
{ 
  HKEY software, acedb_org, acedb, four_x;
  
  *install_dir = 0;

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		   "SOFTWARE", 0L, 
		   KEY_EXECUTE,
		   &software) == ERROR_SUCCESS)
    {
      if (RegOpenKeyEx(software,
		       "acedb.org", 0L, 
		       KEY_EXECUTE,
		       &acedb_org) == ERROR_SUCCESS)
	{ 
	  if (RegOpenKeyEx(acedb_org,
			   "Acedb", 0L, 
			   KEY_EXECUTE,
			   &acedb) == ERROR_SUCCESS)
	    { 
	      if (RegOpenKeyEx(acedb,
			       "4.x", 0L, 
			       KEY_EXECUTE,
			       &four_x) == ERROR_SUCCESS)
		{ 
		  DWORD buffsiz = 255;
		  char buff[256];
		  if (RegQueryValueEx(four_x,
				      "InstallPath",
				      NULL,
				      NULL,
				      buff,
				      &buffsiz) == ERROR_SUCCESS)
		    cygwin_conv_to_full_posix_path(buff, install_dir);
		  RegCloseKey(four_x);
		}
	      RegCloseKey(acedb);
	    }
	  RegCloseKey(acedb_org);
	}
      RegCloseKey(software);
    }
}	

int main(int argc, char **argv)
{
  FILE *f;
  char *dbpath = (char *)malloc(MAX_PATH);
  char *cp2;
  int c;

  if (argc<2)
    exit(1);
  
  /* We need to set the env variable SH_PATH to the location
     of the Acedb copy of sh.exe so that system() calls work.
     It's installed in <install_dir>/bin.
     We get the install_dir from the registry */

  getInstallDir();
  strcat (install_dir, "/bin/sh");

  setenv("SH_PATH", install_dir, 1);


  /* Third arg is optional path to rxvt */
  if (argc == 4)
    cygwin_conv_to_full_posix_path(argv[3], rxvt);

  /* First arg is acedb binary */
  cygwin_conv_to_full_posix_path(argv[1], ace);
  /* second is .adb file */
  if (argc == 2)
    /* No second arg, get xace to prompt */
    strcpy(dbdir_posix, "-promptdb");
  else
    {
      cygwin_conv_to_full_posix_path(argv[2], db);
      
      /* open .adb */
      f = fopen(db, "r");
      if (!f)
	{
	  perror("Open failed: ");
      exit(1);
	}
      
      /* read into dbpath */
      cp2 = dbpath;
      while ((c = fgetc(f)) != EOF)
	*cp2++ = c;
      fclose(f);
      *cp2 = 0;
      
  
      /* Remove everything up to first '=' */
      while (*dbpath && *dbpath != '=')
	dbpath++;
      
      if (*dbpath)
	dbpath++;
      
      /* remove spaces between = and path */
      while (*dbpath && *dbpath == ' ')
	dbpath++;
      
      /* remove trailing newline */
      cp2 = dbpath;
      while (*cp2)
	{ 
	  if (*cp2 == '\n')
	    *cp2 = 0;
	  else
	cp2++;
	}
  
      /* Convert to posix */
      cygwin_conv_to_full_posix_path(dbpath, dbdir_posix);
    }
  
  /* Call ace or rxvt */
  if (argc < 4)
    { 
      bareargv[0] = ace;
      bareargv[1] = dbdir_posix;
      execv(ace,  &bareargv);
    }
  else
    {
      DWORD version = GetVersion();
      /* Pick different fonts for rxvt, depending in version.
	 Win 95 doesn't have Lucida Console, so we have to use FixedSys */
      if ( version < 0x80000000)
	rxvtargv[2] = "Lucida Console-12";
      else
	rxvtargv[2] = "FixedSys";	  
      rxvtargv[8] = ace;
      rxvtargv[9] = dbdir_posix;
      execv(rxvt, &rxvtargv);
    }
  
  perror("Exec failed: ");
  return 1;

}





