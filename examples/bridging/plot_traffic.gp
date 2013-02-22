reset

set terminal png size 600,400
data = 'output/outTable_traffic.txt';
dataRaw = 'output/outTableRaw_traffic.txt';
set output "output/traffic.png"

#set size 1,3
#set origin 0,0
#set multiplot layout 3,1

#1 => numNodes
#...

xCol = 1


##############################################################################
# plot ratios 

set key top left
set grid

if(xCol==1) set xlabel 'Number of Nodes';

if(xCol==1) set xrange [ 0 : 100 ]

if(xCol==1) set yrange [ 0 : 1 ]

set ylabel 'Ratio';

set title "Intervals Ratios";
plot dataRaw using xCol:23  with points t 'R4/3' lt rgb "blue",\
     data using xCol:23  with lines t 'R4/3' lt rgb "blue",\
     dataRaw using xCol:24 with points t 'R5/3' lt rgb "red",\
     data using xCol:24 with lines t 'R5/3' lt rgb "red"

#unset multiplot

#
##############################################################################
