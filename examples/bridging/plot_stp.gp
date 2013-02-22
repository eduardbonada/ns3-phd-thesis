reset

set terminal png size 600,1200
data = 'output/outTable_stp.txt';
dataRaw = 'output/outTableRaw_stp.txt';
set output "output/stp.png"

set size 1,3
set origin 0,0
set multiplot layout 3,1

#1 => executions
#2 => numNodes
#7 => TxHPeriod
#8 => TxHCount

xCol = 2

############################################
# plot all init BPDU counters 

set key top left
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';
if(xCol==7) set xlabel 'TxHoldPeriod';
if(xCol==8) set xlabel 'TxHoldCount';

if(xCol==2) set xrange [ 0 : 100 ]
if(xCol==7) set xrange [ 0 : 1 ]
if(xCol==8) set xrange [ 0 : 20 ]

set ylabel 'Number of {BPDUs,Ports}';

set title "STP - BPDU counters initial convergence";
plot data using xCol:($9/$6)  with linespoints t 'Send',\
     data using xCol:($10/$6) with linespoints t 'Hold',\
     data using xCol:($11/$6) with linespoints t 'Rcvd',\
     data using xCol:($12/$6) with linespoints t 'Rcvd_Better',\
     data using xCol:($13/$6) with linespoints t 'Rcvd_Worse',\
     data using xCol:($14/$6) with linespoints t 'Rcvd_Equal'


############################################
# plot all failure BPDU counters 

set key top left
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';
if(xCol==7) set xlabel 'TxHoldPeriod';
if(xCol==8) set xlabel 'TxHoldCount';

if(xCol==2) set xrange [ 0 : 100 ]
if(xCol==7) set xrange [ 0 : 1 ]
if(xCol==8) set xrange [ 0 : 20 ]


set ylabel 'Number of {BPDUs,Ports}';

set title "STP - BPDU counters root failure";
plot data using xCol:($18/$6)  with linespoints t 'Send',\
     data using xCol:($19/$6) with linespoints t 'Hold',\
     data using xCol:($20/$6) with linespoints t 'Rcvd',\
     data using xCol:($21/$6) with linespoints t 'Rcvd_Better',\
     data using xCol:($22/$6) with linespoints t 'Rcvd_Worse',\
     data using xCol:($23/$6) with linespoints t 'Rcvd_Equal'


############################################
# plot all convergence times 

set key bottom right
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';
if(xCol==7) set xlabel 'TxHoldPeriod';
if(xCol==8) set xlabel 'TxHoldCount';

if(xCol==2) set xrange [ 0 : 100 ]
if(xCol==7) set xrange [ 0 : 1 ]
if(xCol==8) set xrange [ 0 : 20 ]


set ylabel 'Time (sec)';

set title "STP - Convergence Times";
plot dataRaw using xCol:15  with points t 'Alg. init' lt rgb "blue",\
     dataRaw using xCol:16 with points t 'Prot. init' lt rgb "blue",\
     dataRaw using xCol:28  with points t 'Alg. fail' lt rgb "green",\
     dataRaw using xCol:29 with points t 'Prot. fail' lt rgb "green"

#     data using xCol:26  with linespoints t 'Alg. fail (MAexp)' lt rgb "red",\
#     data using xCol:27 with linespoints t 'Prot. fail (MAexp)' lt rgb "red",\


unset multiplot


##############################################################################
#1 Id:  Execution id
#2 Nod: Number of nodes
#3 FlA: Node A to fail
#4 FlB: Node B to fail
#5 FrR: Node forced as root (bridge id set to 0)
#6 Prt: Number of ports (square of the number of links)
#7 HlP: TxHold period
#8 HlC: Maximum Number of BPDUs to be send per TxHold period
#9 Snd: Total Send BPDUs
#10 Hld: Hold BPDUs that had to be sent
#11 Rcv: Total Received BPDUs
#12 RBD: Better BPDU received (from Designated in RSTP)
#13 RWD: Worse BPDU received (from Designated in RSTP)
#14 RED: Equal BPDU received (from Designated in RSTP)
#15 ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure
#16 PCT1: Protocol convergence time (last port transition to forwarding) before failure
#17 Fai: Time when link fails
#18 Snd: Total Send BPDUs (after failure)
#19 Hld: Hold BPDUs that had to be sent (after failure)
#20 Rcv: Total Received BPDUs (after failure)
#21 RBD: Better BPDU received (after failure)
#22 RWD: Worse BPDU received (after failure)
#23 RED: Equal BPDU received (after failure)
#24 MAf: First expiration of Message Age
#25 MAl: Last expiration of Message Age
#26 ACT2: Algorithm convergence Time (STP: last better BPDU) after first mess age expiration
#27 PCT2: Protocol convergence time (last port transition to forwarding) after first mess age expiration
#28 ACT3: Algorithm convergence Time (STP: last better BPDU) after failure
#29 PCT3: Protocol convergence time (last port transition to forwarding) after failure




