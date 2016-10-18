# MakeAceDB.com.
# Lines starting with # are ignored by MakeAcedb so you can comment
# out bits as you feel appropriate.
# Note that if you specify a wrm file, it will override that found in 
# the wspec directory.

#userid=rnc
#groupid=acedb
grpwr=0
rdwr=0
tace=/nfs/team71/acedb/rnc/code/acedb/bin.ALPHA_5/tace
db=newdb
wspec=test-db/wspec
ace=test-db/database.orig.ace
#ace=test-db/database.new.ace
#wrm=test-db/wspecsub/cachesize.wrm
#wrm=test-db/wspecsub/server.wrm
