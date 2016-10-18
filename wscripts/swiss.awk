# File wscripts/swiss.awk, postprocessing commands used by blast_search script
# $Id: swiss.awk,v 1.1 1997-03-28 12:59:54 mieg Exp $

BEGIN { state = 0 ;}
/^AC/ {seq = "SP:"$2 ; gsub(/;/,"",seq) ; printf("\nProtein %s\nDatabase SWISSPROT %s\n",seq,seq); next;}
/^\/\// {state = 0 ; printf("\n") ; next; }

/^SQ/ {printf ("Peptide %s\n\n",seq) ; state = 2 ; next ;}
{ if (state == 2) { printf("Peptide %s\n", seq) ; state = 3 ;  }
  if (state == 3) 
    { pp = $0 ; gsub(/[0-9]/,"",pp) ; gsub(/ /,"",pp) ;
      printf("%s\n", pp) ;
    }
}

