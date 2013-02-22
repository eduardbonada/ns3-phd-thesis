#!/bin/bash

#set -x

if [ $# -ne 7 ]
then
	echo "Bad Number of Arguments."
	echo "Usage: ./per_node_plots_spbls.sh <numNodes> <coldStart> <linkFailure> <nodeFailure> <fileTraffic> <outputFile> <computeTables>"
	exit 0
fi

#####################
# READING ARGUMENTS #
#####################
echo ""
echo "Input Parameters:"

echo "  numNodes:          $1"
num_nodes=$1

echo "  coldStart:         $2"
coldStart=$2
if [ $coldStart = 1 ];then scenarioName='coldStart'; fi;
 
echo "  linkFailure:       $3"
linkFailure=$3
if [ $linkFailure = 1 ];then scenarioName='linkFailure'; fi;

echo "  nodeFailure:       $4"
nodeFailure=$4
if [ $nodeFailure = 1 ];then scenarioName='nodeFailure'; fi;

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
	rm spbls_metrics_per_nodeId_CT-FRW.txt;
	rm spbls_metrics_per_nodeId_CT-LSP.txt;
	for f in `ls -tr n*`;do
	awk -v convTimeAsLastFrwInParam=1 -v convTimeAsLastLspInParam=0 -v coldStartInParam=$coldStart -v linkFailureInParam=$linkFailure -v nodeFailureInParam=$nodeFailure -f per_node_metrics_spbls.awk $f >> spbls_metrics_per_nodeId_CT-FRW.txt;
	awk -v convTimeAsLastFrwInParam=0 -v convTimeAsLastLspInParam=1 -v coldStartInParam=$coldStart -v linkFailureInParam=$linkFailure -v nodeFailureInParam=$nodeFailure -f per_node_metrics_spbls.awk $f >> spbls_metrics_per_nodeId_CT-LSP.txt;
	done;
fi



#########################
# PLOT CONVERGENCE TIME #
#########################
# compute histogram with octave
rm hist-frw.tmp;
rm hist-lsp.tmp;

binWidth=1;
echo "\
ct=load 'spbls_metrics_per_nodeId_CT-FRW.txt';\
[y,x]=hist(ct(:,4),[min(ct(:,4)):"$binWidth":max(ct(:,4))]);\
a=[x;y]';\
save hist-frw.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;
echo "\
ct=load 'spbls_metrics_per_nodeId_CT-LSP.txt';\
[y,x]=hist(ct(:,4),[min(ct(:,4)):"$binWidth":max(ct(:,4))]);\
a=[x;y]';\
save hist-lsp.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;

plotTitle='Convergence Time per node, x-y is Node location, z is CT in hops as last FRW';
echo "\
# dummy plot to compute stats
plot 'hist-frw.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'spbls_metrics_per_nodeId_CT-FRW.txt' using 4:4;\
min_ct = GPVAL_DATA_Y_MIN;\
max_ct = GPVAL_DATA_Y_MAX;\
f(x) = mean_ct;\
fit f(x) 'spbls_metrics_per_nodeId_CT-FRW.txt' using 4:4 via mean_ct;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set output 'per_node_output_plots/spbls_"$scenarioName"-01_ct-frw.png';\
set multiplot layout 1,2
set xlabel 'Node x-location';\
set ylabel 'Node y-location';\
set zlabel 'CT';\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'spbls_metrics_per_nodeId_CT-FRW.txt' using 1:2:4 with lines;
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

plotTitle='Convergence Time per node, x-y is Node location, z is CT(of node) in hops as last LSP';
echo "\
# dummy plot to compute stats
plot 'hist-lsp.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'spbls_metrics_per_nodeId_CT-LSP.txt' using 4:4;\
min_ct = GPVAL_DATA_Y_MIN;\
max_ct = GPVAL_DATA_Y_MAX;\
f(x) = mean_ct;\
fit f(x) 'spbls_metrics_per_nodeId_CT-LSP.txt' using 4:4 via mean_ct;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set output 'per_node_output_plots/spbls_"$scenarioName"-02_ct-lsp.png';\
set multiplot layout 1,2
set xlabel 'Node x-location';\
set ylabel 'Node y-location';\
set zlabel 'CT';\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'spbls_metrics_per_nodeId_CT-LSP.txt' using 1:2:4 with lines;
unset key;\
set xlabel 'CT';\
set ylabel '';\
set title 'Histogram';\
set label 1 gprintf('MAX_CT  = %g', max_ct) at min_x+(max_x-min_x)/10, max_y-(max_y-min_y)/20;\
set label 2 gprintf('MIN_CT  = %g', min_ct) at min_x+(max_x-min_x)/10, max_y-2*(max_y-min_y)/20;\
set label 3 gprintf('MEAN_CT = %g', mean_ct) at min_x+(max_x-min_x)/10, max_y-3*(max_y-min_y)/20;\
set yrange [0:*];\
plot 'hist-lsp.tmp' with boxes;\
unset multiplot;" | gnuplot;

rm hist-frw.tmp;
rm hist-lsp.tmp;


#########################
# PLOT MESSAGE OVERHEAD #
#########################
rm hist-mo.tmp;

if [ $coldStart = 1 ];then binWidth=10;  fi;
if [ $linkFailure = 1 ];then binWidth=1;  fi;
if [ $nodeFailure = 1 ];then binWidth=1;  fi;
echo "\
mo=load 'spbls_metrics_per_nodeId_CT-FRW.txt';\
[y,x]=hist(mo(:,5),[min(mo(:,5)):"$binWidth":max(mo(:,5))]);\
a=[x;y]';\
save hist-mo.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;

plotTitle='Message Overhead, x-y is Node location, z is Messages per network';
echo "\
# dummy plot to compute stats
plot 'hist-mo.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'spbls_metrics_per_nodeId_CT-FRW.txt' using 5:5;\
min_mo = GPVAL_DATA_Y_MIN;\
max_mo = GPVAL_DATA_Y_MAX;\
f(x) = mean_mo;\
fit f(x) 'spbls_metrics_per_nodeId_CT-FRW.txt' using 5:5 via mean_mo;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set output 'per_node_output_plots/spbls_"$scenarioName"-03_mo.png';\
set multiplot layout 1,2
set xlabel 'Node x-location';\
set ylabel 'Node y-location';\
set zlabel 'MO';\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'spbls_metrics_per_nodeId_CT-FRW.txt' using 1:2:5 with lines;
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


#################
# PLOT TRIGGERS #
#################
rm hist-tr.tmp;

if [ $coldStart = 1 ];then binWidth=10;  fi;
if [ $linkFailure = 1 ];then binWidth=1;  fi;
if [ $nodeFailure = 1 ];then binWidth=1;  fi;
echo "\
tr=load 'spbls_metrics_per_nodeId_CT-FRW.txt';\
[y,x]=hist(tr(:,6),[min(tr(:,6)):"$binWidth":max(tr(:,6))]);\
a=[x;y]';\
save hist-tr.tmp a;" > "octave_script.tmp"
octave "octave_script.tmp";
rm octave_script.tmp;

plotTitle='Triggers, x-y is Node location, z is Triggers per network';
echo "\
# dummy plot to compute stats
plot 'hist-tr.tmp';\
min_y = GPVAL_DATA_Y_MIN;\
max_y = GPVAL_DATA_Y_MAX;\
min_x = GPVAL_DATA_X_MIN;\
max_x = GPVAL_DATA_X_MAX;\
plot 'spbls_metrics_per_nodeId_CT-FRW.txt' using 6:6;\
min_tr = GPVAL_DATA_Y_MIN;\
max_tr = GPVAL_DATA_Y_MAX;\
f(x) = mean_tr;\
fit f(x) 'spbls_metrics_per_nodeId_CT-FRW.txt' using 6:6 via mean_tr;\
# end of dummy plot to compute stats
set terminal png size 1200,600;\
set output 'per_node_output_plots/spbls_"$scenarioName"-04_tr.png';\
set multiplot layout 1,2
set xlabel 'Node x-location';\
set ylabel 'Node y-location';\
set zlabel 'TR';\
unset key;\
set title '"$plotTitle"';\
set hidden3d;\
splot 'spbls_metrics_per_nodeId_CT-FRW.txt' using 1:2:6 with lines;
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



###################
# PLOT THROUGHPUT #
###################
binWidth=0.1
#if [ $rootFailure = 1 ];then binWidth=100; fi
./throughput_timelines.sh $filenameTraffic 0 $num_nodes $binWidth
if [ $coldStart = 1 ];then
	histLimits="0:60"
	axis="[0 50 0 70]";
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in corner');\
	xlabel('Time in hops');\
	ylabel('Messages received');\
	print('per_node_output_plots/spbls_"$scenarioName"-05_throughput-corner.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
if [ $linkFailure = 1 ] || [ $nodeFailure = 1 ];then
	histLimits="29950:1:30070"
	axis="[29990 30030 0 70]";
  xLabel="Time in hops"
  #if [ $rootFailure = 1 ];then
  #  histLimits="20:300"
  #  axis="[20 250 0 70]";
  #  xLabel="Time in 0.1 seconds"
  #fi
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in corner');\
	xlabel('"$xLabel"');\
	ylabel('Messages received');\
	print('per_node_output_plots/spbls_"$scenarioName"-05_throughput-corner.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
rm *.histdata

./throughput_timelines.sh $filenameTraffic 36 $num_nodes $binWidth
if [ $coldStart = 1 ];then
	histLimits="0:60"
	axis="[0 50 0 70]";
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in center');\
	xlabel('Time in hops');\
	ylabel('Messages received');\
	print('per_node_output_plots/spbls_"$scenarioName"-06_throughput-center.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
if [ $linkFailure = 1 ] || [ $nodeFailure = 1 ];then
  histLimits="29950:1:30070"
  axis="[29990 30030 0 70]";
  xLabel="Time in hops"
  #if [ $rootFailure = 1 ];then
  #  histLimits="20:300"
  #  axis="[20 250 0 70]";
  #  xLabel="Time in 0.1 seconds"
  #fi
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput at node in center');\
	xlabel('"$xLabel"');\
	ylabel('Messages received');\
	print('per_node_output_plots/spbls_"$scenarioName"-06_throughput-center.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
rm *.histdata

./throughput_timelines.sh $filenameTraffic -1 $num_nodes $binWidth
if [ $coldStart = 1 ];then
  histLimits="0:60"
  axis="[0 50 0 4200]";
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput in all network');\
	xlabel('Time in hops');\
	ylabel('Messages received');\
	print('per_node_output_plots/spbls_"$scenarioName"-07_throughput-all.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
if [ $linkFailure = 1 ] || [ $nodeFailure = 1 ];then
  histLimits="29950:1:30070"
  axis="[29990 30030 0 4200]";
  xLabel="Time in hops"
  #if [ $rootFailure = 1 ];then
  #  histLimits="20:300"
  #  axis="[20 250 0 4200]";
  #  xLabel="Time in 0.1 seconds"
  #fi
	echo "\
	data=load('"$filenameTraffic".histdata');\
	[y,x]=hist(data(:,1),["$histLimits"]); plot(x,y); axis("$axis");\
	title('Throughput in all network');\
	xlabel('"$xLabel"');\
	ylabel('Messages received');\
	print('per_node_output_plots/spbls_"$scenarioName"-07_throughput-all.png');" > "octave_script.tmp"
	octave "octave_script.tmp";
	rm octave_script.tmp;
fi;
rm *.histdata

################################
# MERGE ALL PNGs TO SINGLE PDF #
################################
convert per_node_output_plots/*.png per_node_output_plots/$filenameOutput


