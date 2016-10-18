{ mm = $1 ; n = length(mm) ; mm2 = substr(mm, 6, n-11) ;
 start = $2 ; stop = $3 ; ll = $4 ; gc = $5 ; tm = $6 ; score = $7 ;
 if (mm2 != "" && score < 20)
  printf("Oligo %s\nSequence %s\nLength %s\nGC %s\nTm %s\nScore %s\nIn_sequence %s\n\n",
	mm2, mm2, ll, gc, tm, score, zzseq) ;
}
