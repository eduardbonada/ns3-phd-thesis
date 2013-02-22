#!/bin/bash

#set -x

rm *.hist
rm *.histdata
rm *.tmp

if [ $# -ne 4 ]
then
	echo "Bad Number of Arguments."
	echo "Usage: ./traffic_output.sh <traces filename> <node receiver> <num nodes> <bins width in ms>"
	exit 0
fi

#####################
# READING ARGUMENTS #
#####################
echo ""
echo "Input Parameters:"

# the first argument is the filename of the traces
echo "  Traces Filename: $1"
tracesFilename=$1
# the second argument is the nodeReceiver to analyze ('-1' for merged results with all receptions)
echo "  Receiver:        $2"
nodeReceiver=$2
# the third argument is the number of nodes
echo "  Num nodes:       $3"
numNodes=$3
# the fourth argument is the bin width in ms
echo "  Bin width:       $4"
binWidth=$4




#####################
# REMOVE CHARACTERS #
#####################
echo ""
echo "Cleaning input traces file..."
sed 's/\[/ /g' $tracesFilename > "${tracesFilename}.step2.tmp"
sed 's/\]/ /g' "${tracesFilename}.step2.tmp" > "${tracesFilename}.step3.tmp"
sed 's/ns/ /g' "${tracesFilename}.step3.tmp" > "${tracesFilename}.step4.tmp"
sed 's/|/ /g' "${tracesFilename}.step4.tmp" > "${tracesFilename}.step5.tmp"
sed 's/(/ /g' "${tracesFilename}.step5.tmp" > "${tracesFilename}.step6.tmp"
sed 's/)/ /g' "${tracesFilename}.step6.tmp" > "${tracesFilename}.step7.tmp"
sed 's/SA:/0x/g' "${tracesFilename}.step7.tmp" > "${tracesFilename}.step8.tmp"
sed 's/DA:/0x/g' "${tracesFilename}.step8.tmp" > "${tracesFilename}.step9.tmp"
sed 's/Type:/ /g' "${tracesFilename}.step9.tmp" > "${tracesFilename}.step10.tmp"
sed 's/Length:/ /g' "${tracesFilename}.step10.tmp" > "${tracesFilename}.step11.tmp"
sed 's/://g' "${tracesFilename}.step11.tmp" > "${tracesFilename}.step12.tmp"
mv "${tracesFilename}.step12.tmp" "${tracesFilename}.tmp"
echo ""



echo ""
echo "Parsing cleaned traces file..."

###########################
# ANALYZE A CONCRETE NODE #
###########################
if [ $nodeReceiver != -1 ]
then
  echo ""
	echo "Computing traffic received by node ${nodeReceiver}"

	# note we convert the MACs, in decimal, to nodeIds (nodeId=MAC-1)
	# note we divide the time by 1000000 to get ms (so octave can read it)

  echo "#Time rcv length DA SA Type" > "${tracesFilename}.histdata"
  awk '/Traffic Sink/{if($4=='$nodeReceiver') {printf("%d %d %d %d %d %d\n", $1/(1000000*'${binWidth}'), $4, $7, $8-1, $9-1, $10);}}' "${tracesFilename}.tmp" >> "${tracesFilename}.histdata"

  # plot histogram with octave
  #echo "timeline=load "${tracesFilename}".histdata;\
  #      hist(timeline(:,1),[min(timeline(:,1)):"$binWidth":max(timeline(:,1))]);\
  #      pause();" > "octave_script.tmp"
  #octave "octave_script.tmp";
fi


###################################################
# ANALYZE ALL NODES TOGETHER (NETWORK THROUGHPUT) #
###################################################
if [ $nodeReceiver == -1 ]
then
  echo ""
	echo "Computing traffic received by all nodes together (network throughput)"

	# note we convert the MACs, in decimal, to nodeIds (nodeId=MAC-1)
	# note we divide the time by 1000000 to get ms (so octave can read it)

  echo "#Time rcv length DA SA Type" > "${tracesFilename}.histdata"
  awk '/Traffic Sink/{printf("%d %d %d %d %d %d\n", $1/(1000000*'${binWidth}'), $4, $7, $8-1, $9-1, $10);}' "${tracesFilename}.tmp" >> "${tracesFilename}.histdata"

  # plot histogram with octave
  #echo "timeline=load "${tracesFilename}".histdata;\
  #      hist(timeline(:,1),[min(timeline(:,1)):"$binWidth":max(timeline(:,1))]);\
  #      pause();" > "octave_script.tmp"
  #octave "octave_script.tmp";  
fi


#################################
# ANALYZE ALL NODES SEPARATEDLY #
#################################
if [ $nodeReceiver == -2 ]
then
  echo ""
	echo "Computing traffic received by all nodes..."

	# note we convert the MACs, in decimal, to nodeIds (nodeId=MAC-1)
	# note we divide the time by 1000000 to get ms (so octave can read it)
	
	# this awk scripts considers all lines regardless the receiver Id
  #awk '/Traffic Sink/{printf("%d %d %d %d %d %d\n", $1/1000000, $4, $7, $8-1, $9-1, $10);}' "${tracesFilename}.tmp" >> "${tracesFilename}.hist"
    
  # create histogram data files to print then with gnuplot with octave
  for (( nodeId=0; nodeId<$numNodes; nodeId++ ))
  do 
    echo "#Time rcv length DA SA Type" > "${tracesFilename}.n${nodeId}.histdata"
    awk '/Traffic Sink/{if($4=='$nodeId') {printf("%d %d %d %d %d %d\n", $1/(1000000*'${binWidth}'), $4, $7, $8-1, $9-1, $10);}}' "${tracesFilename}.tmp" >> "${tracesFilename}.n${nodeId}.histdata"
    echo "timeline=load "${tracesFilename}".n"${nodeId}".histdata;\
          histogram=(hist(timeline(:,1),[min(timeline(:,1)):"$binWidth":max(timeline(:,1))]))';\
          histogram3d=zeros(size(histogram,1),3);\
          histogram3d(:,1)="$nodeId";\
          histogram3d(:,2)=[min(timeline(:,1)):"$binWidth":max(timeline(:,1))];\
          histogram3d(:,3)=histogram;\
          save "${tracesFilename}".n"${nodeId}".hist histogram3d;" > "octave_script.tmp"
    octave "octave_script.tmp";
  done
  
  # plot with gnuplot
  #echo "unset key; splot \\" > "gnuplot_script.tmp";
  #for (( nodeId=0; nodeId<$numNodes; nodeId++ ))
  #do
  #  echo -e '"'${tracesFilename}'.n'${nodeId}'.hist"\' >> "gnuplot_script.tmp";
  #  if [ $nodeId == `expr $numNodes - 1` ]
  #  then
  #    echo -e ';' >> "gnuplot_script.tmp";
  #  else
  #    echo -e ',\' >> "gnuplot_script.tmp";
  #  fi
  #done
  #gnuplot -persist "gnuplot_script.tmp";

fi

rm *.hist
#rm *.histdata
rm *.tmp

echo ""
echo "Done"
echo ""
