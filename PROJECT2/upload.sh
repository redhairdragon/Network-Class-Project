#~/bin/bash
a=`ls *.cpp *.h Makefile`
gcloud compute scp $a homework:~
