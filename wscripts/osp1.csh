#!/usr/bin/csh -f
# $Id: osp1.csh,v 1.3 1997-06-25 18:49:01 mieg Exp $
# param 1 should be the sequence name; 2,3,4,the maxScore, min max lengths 
#                                      5,6 TmMin TmMax the oligos, 7 the fasta file
./osp1.pl -m $2 -l $3,$4 -t $5 -T $6 -s $7 | nawk -f ./osp1.awk zzseq=$1 -
exit 0


# debug version of same
echo osp1.csh running >! /tmp/xxxaa
echo $1 $2 $3 $4 $5 $6 $7 >> /tmp/xxxaa
#
./osp1.pl -m $2 -l $3,$4 -t $5 -T $6 -s $7   >! /tmp/xxx2
nawk -f ./osp1.awk zzseq=$1 /tmp/xxx2 >!  /tmp/xxx3
cat /tmp/xxx3

#
