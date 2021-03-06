#!/bin/bash

#set -x

if [ $# -ne 7 ]
then
	echo "Bad Number of Arguments."
	echo "Usage: ./per_node_plots_rstp.sh <coldStart> <linkFailure> <nodeFailure> <rootFailure> <fileTraffic> <outputFile> <computeTables>"
	exit 0
fi

#####################
# READING ARGUMENTS #
#####################
echo ""
echo "Input Parameters:"

echo "  coldStart:         $1"
coldStart=$1
if [ $coldStart = 1 ];then scenarioName='coldStart'; fi;
 
echo "  linkFailure:       $2"
linkFailure=$2
if [ $linkFailure = 1 ];then scenarioName='linkFailure'; fi;

echo "  nodeFailure:       $3"
nodeFailure=$3
if [ $nodeFailure = 1 ];then scenarioName='nodeFailure'; fi;

echo "  rootFailure:       $4"
rootFailure=$4
if [ $rootFailure = 1 ];then scenarioName='rootFailure'; fi;

echo "  traffic filename:  $5"
filenameTraffic=$5

echo "  output filename:   $6"
filenameOutput=$6

echo "  compute tables:    $7"
computeTables=$7

echo ""


##############################
# COMPUTE TABLE WITH METRICS #
##############################
if [ $computeTables = 1 ];then
	echo ""
	echo "Computing tables:"
	rm rstp_metrics_per_nodeId_CT-FRW.txt;
	rm rstp_metrics_per_bridgeId_CT-BPDU.txt;
	x=0; y=0;
	for f in `ls -tr n*`;do
	echo " parsing " $f
	awk -v xPos=$x -v yPos=$y -v sortByNodeIdInParam=1 -v sortByBridgeIdInParam=0 -v convTimeAsLastFrwInParam=1 -v convTimeAsLastBpduInParam=0 -v coldStartInParam=$coldStart -v linkFailureInParam=$linkFailure -v nodeFailureInParam=$nodeFailure -v rootFailureInParam=$rootFailure -f per_node_metrics_rstp.awk $f >> rstp_metrics_per_nodeId_CT-FRW.txt;
	awk -v xPos=$x -v yPos=$y -v sortByNodeIdInParam=0 -v sortByBridgeIdInParam=1 -v convTimeAsLastFrwInParam=0 -v convTimeAsLastBpduInParam=1 -v coldStartInParam=$coldStart -v linkFailureInParam=$linkFailure -v nodeFailureInParam=$nodeFailure -v rootFailureInParam=$rootFailure -f per_node_metrics_rstp.awk $f >> rstp_metrics_per_bridgeId_CT-BPDU.txt;
	y=`expr $y + 1`;
	if [ $y = 8 ];then x=`expr $x + 1`; y=0; echo "" >> rstp_metrics_per_bridgeId_CT-BPDU.txt; echo "" >> rstp_metrics_per_nodeId_CT-FRW.txt; fi;
	done;
fi;

#########################
# PLOT CONVERGENCE TIME #
#########################
# compute histogram with octave
rm hist-frw.tmp;
rm hist-bpdu.tmp;

if [ $coldStart = 1 ];then binWidth=1; fi;
if [ $linkFailure = 1 ];then binWidth=1; fi;
if [ $nodeFailure = 1 ];then binWidth=10000; fi;
echo "\
ct=load 'rstp_metrics_per_nodeId_CT-FRW.txt';\
[y,x]=hist(ct(:,7),[min(ct(:,7)):"$binWidth":max(ct(:,7))]);\
a=[x;y]';\
save hist-frw.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;
echo "\
ct=load 'rstp_metrics_per_bridgeId_CT-BPDU.txt';\
[y,x]=hist(ct(:,7),[min(ct(:,7)):"$binWidth":max(ct(:,7))]);\
a=[x;y]';\
save hist-bpdu.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;

if [ $coldStart = 1 ];then plotTitle='Convergence Time per net, x-y is Root location, z is CT in hops as last FRW'; zrange='[*:*]'; fi;
if [ $linkFailure = 1 ];then plotTitle='Convergence Time per net, x-y is ???, z is CT in hops as last FRW'; zrange='[*:*]'; fi;
if [ $nodeFailure = 1 ];then plotTitle='Convergence Time per net, Root in corner, x-y is failed node, z is CT in hops as last FRW'; zrange='[*:20]'; fi;
echo "\
# dummy plot to compute stats
plot 'hist-frw.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'rstp_metrics_per_nodeId_CT-FRW.txt' using 7:7;\
min_ct = GPVAL_DATA_Y_MIN;\
max_ct = GPVAL_DATA_Y_MAX;\
f(x) = mean_ct;\
fit f(x) 'rstp_metrics_per_nodeId_CT-FRW.txt' using 7:7 via mean_ct;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set output 'per_node_output_plots/rstp_"$scenarioName"-01_ct-frw.png';\
set multiplot layout 1,2
set xlabel 'Root x-location';\
set ylabel 'Root y-location';\
set zlabel 'CT';\
set zrange "$zrange";\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'rstp_metrics_per_nodeId_CT-FRW.txt' using 1:2:7 with lines;\
unset key;\
set xlabel 'CT';\
set ylabel '';\
set title 'Histogram';\
set label 1 gprintf('MAX_CT  = %g', max_ct) at min_x+(max_x-min_x)/10, max_y-(max_y-min_y)/20;\
set label 2 gprintf('MIN_CT  = %g', min_ct) at min_x+(max_x-min_x)/10, max_y-2*(max_y-min_y)/20;\
set label 3 gprintf('MEAN_CT = %g', mean_ct) at min_x+(max_x-min_x)/10, max_y-3*(max_y-min_y)/20;\
set yrange [0:*];\
plot 'hist-frw.tmp' with boxes;\
unset multiplot;" | gnuplot;

if [ $coldStart = 1 ];then plotTitle='Convergence Time per net, x-y is Root location, z is CT in hops as last BPDU';  zrange='[*:*]'; fi;
if [ $linkFailure = 1 ];then plotTitle='Convergence Time per net, Root in corner, x-y is ???, z is CT in hops as last BPDU';  zrange='[*:*]'; fi;
if [ $nodeFailure = 1 ];then plotTitle='Convergence Time per net, Root in corner, x-y is failed node, z is CT in hops as last BPDU'; zrange='[*:20]'; fi;
echo "\
# dummy plot to compute stats
plot 'hist-bpdu.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'rstp_metrics_per_bridgeId_CT-BPDU.txt' using 7:7;\
min_ct = GPVAL_DATA_Y_MIN;\
max_ct = GPVAL_DATA_Y_MAX;\
f(x) = mean_ct;\
fit f(x) 'rstp_metrics_per_bridgeId_CT-BPDU.txt' using 7:7 via mean_ct;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set terminal png size 1200,600;\
set output 'per_node_output_plots/rstp_"$scenarioName"-02_ct-bpdu.png';\
set multiplot layout 1,2
set xlabel 'Root x-location';\
set ylabel 'Root y-location';\
set zlabel 'CT';\
set zrange "$zrange";\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'rstp_metrics_per_bridgeId_CT-BPDU.txt' using 1:2:7 with lines;
unset key;\
set xlabel 'CT';\
set ylabel '';\
set title 'Histogram';\
set label 1 gprintf('MAX_CT  = %g', max_ct) at min_x+(max_x-min_x)/10, max_y-(max_y-min_y)/20;\
set label 2 gprintf('MIN_CT  = %g', min_ct) at min_x+(max_x-min_x)/10, max_y-2*(max_y-min_y)/20;\
set label 3 gprintf('MEAN_CT = %g', mean_ct) at min_x+(max_x-min_x)/10, max_y-3*(max_y-min_y)/20;\
set yrange [0:*];\
plot 'hist-bpdu.tmp' with boxes;\
unset multiplot;" | gnuplot;

rm hist-frw.tmp;
rm hist-bpdu.tmp;


#########################
# PLOT MESSAGE OVERHEAD #
#########################
rm hist-mo.tmp;

if [ $coldStart = 1 ];then binWidth=10; fi;
if [ $linkFailure = 1 ];then binWidth=10; fi;
if [ $nodeFailure = 1 ];then binWidth=100; fi;
echo "\
mo=load 'rstp_metrics_per_nodeId_CT-FRW.txt';\
[y,x]=hist(mo(:,7+65),[min(mo(:,7+65)):"$binWidth":max(mo(:,7+65))]);\
a=[x;y]';\
save hist-mo.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;

if [ $coldStart = 1 ];then plotTitle='Message Overhead, x-y is Root location, z is Messages per network';  zrange='[*:*]'; fi;
if [ $linkFailure = 1 ];then plotTitle='Message Overhead, Root in corner, x-y is ???, z is Messages per network';  zrange='[*:*]'; fi;
if [ $nodeFailure = 1 ];then plotTitle='Message Overhead, Root in corner, x-y is failed node, z is Messages per network';  zrange='[*:500]'; fi;
echo "\
# dummy plot to compute stats
plot 'hist-mo.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'rstp_metrics_per_nodeId_CT-FRW.txt' using 7+64+1:7+64+1;\
min_mo = GPVAL_DATA_Y_MIN;\
max_mo = GPVAL_DATA_Y_MAX;\
f(x) = mean_mo;\
fit f(x) 'rstp_metrics_per_nodeId_CT-FRW.txt' using 7+64+1:7+64+1 via mean_mo;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set output 'per_node_output_plots/rstp_"$scenarioName"-03_mo.png';\
set multiplot layout 1,2
set xlabel 'Root x-location';\
set ylabel 'Root y-location';\
set zlabel 'MO';\
set zrange "$zrange";\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'rstp_metrics_per_nodeId_CT-FRW.txt' using 1:2:7+65 with lines;
unset key;\
set xlabel 'MO';\
set ylabel '';\
set title 'Histogram';\
set label 1 gprintf('MAX_MO  = %g', max_mo) at min_x+(max_x-min_x)/10, max_y-(max_y-min_y)/20;\
set label 2 gprintf('MIN_MO  = %g', min_mo) at min_x+(max_x-min_x)/10, max_y-2*(max_y-min_y)/20;\
set label 3 gprintf('MEAN_MO = %g', mean_mo) at min_x+(max_x-min_x)/10, max_y-3*(max_y-min_y)/20;\
plot 'hist-mo.tmp' with boxes;\
unset multiplot;" | gnuplot;

rm hist-mo.tmp;



###############################################
# PLOT MESSAGE OVERHEAD - per nodeId/bridgeId #
###############################################
if [ $coldStart = 1 ];then plotTitleNodeId='MO of all Root cases sorting by Node Id'; fi;
if [ $linkFailure = 1 ];then plotTitleNodeId='MO of all Link Failure cases sorting by Node Id, Root in corner'; fi;
if [ $nodeFailure = 1 ];then plotTitleNodeId='MO of all Node Failure cases sorting by Node Id, Root in corner'; fi;

if [ $coldStart = 1 ];then plotTitleBrId='MO of all Root cases sorting by Bridge Id'; fi;
if [ $linkFailure = 1 ];then plotTitleBrId='MO of all Link Failure sorting by Bridge Id, Root in corner'; fi;
if [ $nodeFailure = 1 ];then plotTitleBrId='MO of all Node Failure sorting by Bridge Id, Root in corner'; fi;

echo "\
mo=load 'rstp_metrics_per_nodeId_CT-FRW.txt';\
subplot(2,1,1);\
plot(mo(1,7+65+1:7+65+64),'o');\
hold on;\
for i=2:64\
plot(mo(i,7+65+1:7+65+64),'o');\
endfor;\
plot(mo(1,7+65+1:7+65+64),'r','linewidth',2);\
plot(mo(35,7+65+1:7+65+64),'g','linewidth',2);\
title('"$plotTitleNodeId"');\
xlabel('Node Id (red/green: corner/center root)');\
ylabel('MO');\
subplot(2,1,2);\
mo=load 'rstp_metrics_per_bridgeId_CT-BPDU.txt';\
plot(mo(1,7+65+1:7+65+64),'o');\
hold on;\
for i=2:64\
plot(mo(i,7+65+1:7+65+64),'o');\
endfor;\
title('"$plotTitleBrId"');\
xlabel('BridgeId');\
ylabel('MO');\
print('per_node_output_plots/rstp_"$scenarioName"-04_mo-perNId-BrId.png');" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;


#################
# PLOT TRIGGERS #
#################
rm hist-tr.tmp;

if [ $coldStart = 1 ];then binWidth=1; fi;
if [ $linkFailure = 1 ];then binWidth=1; fi;
if [ $nodeFailure = 1 ];then binWidth=100; fi;
echo "\
tr=load 'rstp_metrics_per_nodeId_CT-FRW.txt';\
[y,x]=hist(tr(:,7+65+65),[min(tr(:,7+65+65)):"$binWidth":max(tr(:,7+65+65))]);\
a=[x;y]';\
save hist-tr.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;

if [ $coldStart = 1 ];then plotTitle='Triggers, x-y is Root location, z is Triggers per network';  zrange='[*:*]'; fi;
if [ $linkFailure = 1 ];then plotTitle='Triggers, Root in corner, x-y is ???, z is Triggers per network';  zrange='[*:*]'; fi;
if [ $nodeFailure = 1 ];then plotTitle='Triggers, Root in corner, x-y is failed node, z is Triggers per network';  zrange='[*:50]'; fi;
echo "\
# dummy plot to compute stats
plot 'hist-tr.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'rstp_metrics_per_nodeId_CT-FRW.txt' using 7+64+1+64+1:7+64+1+64+1;\
min_tr = GPVAL_DATA_Y_MIN;\
max_tr = GPVAL_DATA_Y_MAX;\
f(x) = mean_tr;\
fit f(x) 'rstp_metrics_per_nodeId_CT-FRW.txt' using 7+64+1+64+1:7+64+1+64+1 via mean_tr;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set output 'per_node_output_plots/rstp_"$scenarioName"-05_tr.png';\
set multiplot layout 1,2
set xlabel 'Root x-location';\
set ylabel 'Root y-location';\
set zlabel 'TR';\
set zrange "$zrange";\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'rstp_metrics_per_nodeId_CT-FRW.txt' using 1:2:7+65+65 with lines;
unset key;\
set xlabel 'TR';\
set ylabel '';\
set title 'Histogram';\
set label 1 gprintf('MAX_TR  = %g', max_tr) at min_x+(max_x-min_x)/10, max_y-(max_y-min_y)/20;\
set label 2 gprintf('MIN_TR  = %g', min_tr) at min_x+(max_x-min_x)/10, max_y-2*(max_y-min_y)/20;\
set label 3 gprintf('MEAN_TR = %g', mean_tr) at min_x+(max_x-min_x)/10, max_y-3*(max_y-min_y)/20;\
plot 'hist-tr.tmp' with boxes;\
unset multiplot;" | gnuplot;

rm hist-tr.tmp;


#######################################
# PLOT TRIGGERS - per nodeId/bridgeId #
#######################################
if [ $coldStart = 1 ];then plotTitleNodeId='TR of all Root cases sorting by Node Id'; fi;
if [ $linkFailure = 1 ];then plotTitleNodeId='TR of all Root cases sorting by Node Id, Root in corner'; fi;
if [ $nodeFailure = 1 ];then plotTitleNodeId='TR of all Root cases sorting by Node Id, Root in corner'; fi;

if [ $coldStart = 1 ];then plotTitleBrId='TR of all Root cases sorting by Bridge Id'; fi;
if [ $linkFailure = 1 ];then plotTitleBrId='TR of all Root cases sorting by Bridge Id, Root in corner'; fi;
if [ $nodeFailure = 1 ];then plotTitleBrId='TR of all Root cases sorting by Bridge Id, Root in corner'; fi;

echo "\
tr=load 'rstp_metrics_per_nodeId_CT-FRW.txt';\
subplot(2,1,1);\
plot(tr(1,7+65+65+1:7+65+65+64),'o');\
hold on;\
for i=2:64\
plot(tr(i,7+65+65+1:7+65+65+64),'o');\
endfor;\
plot(tr(1,7+65+65+1:7+65+65+64),'r','linewidth',2);\
plot(tr(35,7+65+65+1:7+65+65+64),'g','linewidth',2);\
title('"$plotTitleNodeId"');\
xlabel('Node Id (red/green: corner/center root)');\
ylabel('TR');\
subplot(2,1,2);\
tr=load 'rstp_metrics_per_bridgeId_CT-BPDU.txt';\
plot(tr(1,7+65+65+1:7+65+65+64),'o');\
hold on;\
for i=2:64\
plot(tr(i,7+65+65+1:7+65+65+64),'o');\
endfor;\
title('"$plotTitleBrId"');\
xlabel('BridgeId');\
ylabel('TR');\
print('per_node_output_plots/rstp_"$scenarioName"-06_tr-perNId-BrId.png');" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;


###################
# PLOT THROUGHPUT #
###################
binWidth=0.1
if [ $rootFailure = 1 ];then binWidth=100; fi
./throughput_timelines.sh $filenameTraffic 0 64 $binWidth
if [ $coldStart = 1 ];then
	histLimits="0:60"
	axis="[0 50 0 70]";
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in corner');\
	xlabel('Time in hops');\
	ylabel('Messages received');\
	print('per_node_output_plots/rstp_"$scenarioName"-07_throughput-corner.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
if [ $linkFailure = 1 ] || [ $nodeFailure = 1 ];then
	histLimits="29950:1:30070"
	axis="[29990 30030 0 70]";
  xLabel="Time in hops"
  if [ $rootFailure = 1 ];then
    histLimits="20:400"
    axis="[20 350 0 70]";
    xLabel="Time in 0.1 seconds"
  fi
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in corner');\
	xlabel('"$xLabel"');\
	ylabel('Messages received');\
	print('per_node_output_plots/rstp_"$scenarioName"-08_throughput-corner.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
rm *.histdata

./throughput_timelines.sh $filenameTraffic 36 64 $binWidth 
if [ $coldStart = 1 ];then
	histLimits="0:60"
	axis="[0 50 0 70]";
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in center');\
	xlabel('Time in hops');\
	ylabel('Messages received');\
	print('per_node_output_plots/rstp_"$scenarioName"-09_throughput-center.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
if [ $linkFailure = 1 ] || [ $nodeFailure = 1 ];then
  histLimits="29950:1:30070"
  axis="[29990 30030 0 70]";
  xLabel="Time in hops"
  if [ $rootFailure = 1 ];then
    histLimits="20:400"
    axis="[20 350 0 70]";
    xLabel="Time in 0.1 seconds"
  fi
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in center');\
	xlabel('"$xLabel"');\
	ylabel('Messages received');\
	print('per_node_output_plots/rstp_"$scenarioName"-07_throughput-center.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
rm *.histdata

./throughput_timelines.sh $filenameTraffic -1 64 $binWidth 
if [ $coldStart = 1 ];then
  histLimits="0:60"
  axis="[0 50 0 4200]";
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput in all network');\
	xlabel('Time in hops');\
	ylabel('Messages received');\
	print('per_node_output_plots/rstp_"$scenarioName"-08_throughput-all.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
if [ $linkFailure = 1 ] || [ $nodeFailure = 1 ];then
  histLimits="29950:1:30070"
  axis="[29990 30030 0 4200]";
  xLabel="Time in hops"
  if [ $rootFailure = 1 ];then
    histLimits="20:400"
    axis="[20 350 0 4200]";
    xLabel="Time in 0.1 seconds"
  fi
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput in all network');\
	xlabel('"$xLabel"');\
	ylabel('Messages received');\
	print('per_node_output_plots/rstp_"$scenarioName"-09_throughput-all.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
rm *.histdata

################################
# MERGE ALL PNGs TO SINGLE PDF #
################################
convert per_node_output_plots/*.png per_node_output_plots/$filenameOutput


