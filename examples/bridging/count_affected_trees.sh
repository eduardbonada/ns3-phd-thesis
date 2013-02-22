#!/bin/bash

#set -x

# count how many trees process BPDUs after the failure (count in how many trees there is at least one recieved BPDU about that tree)

for f in `ls -tr output/n*`
do
  echo $f
  
	# parse the output trace file and select only the lines after the failure that trace a received BPDU
	awk '/BEGIN/{f=0;}/SPBDV: Periodical BPDUs/{f=0;}/Failed Link between nodes/{f=1}/SPBDV: Recvs BPDU \[R:/{if(f==1) print}' $f > tmp.txt

	# add spaces after '[R:' and before '|C:' to easily select which tree the BPDU refers to
	sed -i 's/R:/R: /g' tmp.txt
	sed -i 's/|C:/ |C:/g' tmp.txt

	# count how many different trees are affected, by counting how many different Roots appear in sent BPDUs
	awk 'BEGIN{for(i=0;i<64;i++)affTree[i]=0; count=0;} //{if(affTree[$7]==0)count=count+1; affTree[$7]=1;} END{print count;}' tmp.txt
  
  rm tmp.txt
done;


