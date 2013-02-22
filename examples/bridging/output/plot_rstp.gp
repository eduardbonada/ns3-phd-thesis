reset

set terminal png size 600,1200
data = 'output/outTable_rstp.txt';
dataRaw = 'output/outTableRaw_rstp.txt';
set output "output/rstp.png"

set size 1,3
set origin 0,0
set multiplot layout 3,1

#1 => executions
#2 => numNodes
#7 => TxHPeriod
#8 => TxHCount

xCol = 2

if(xCol==2) set xrange [ 0 : 70 ]
if(xCol==7) set xrange [ 0 : 1 ]
if(xCol==8) set xrange [ 0 : 40 ]

set key top left
set grid

############################################
# plot all convergence times 

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';
if(xCol==7) set xlabel 'TxHoldPeriod';
if(xCol==8) set xlabel 'TxHoldCount';

set ylabel 'Time (sec)';

set title "RSTP - Convergence Times";
plot dataRaw using xCol:17  with points t 'ACT cold start' lt rgb "blue",\
     dataRaw using xCol:18  with points t 'PCT cold start' lt rgb "blue",\
     dataRaw using xCol:32  with points t 'ACT failure' lt rgb "green",\
     dataRaw using xCol:33  with points t 'PCT failure' lt rgb "green"
     
############################################
# plot all init BPDU counters 

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';
if(xCol==7) set xlabel 'TxHoldPeriod';
if(xCol==8) set xlabel 'TxHoldCount';

set ylabel 'Number of Messages';

set title "RSTP - BPDU counters initial convergence";
plot dataRaw using xCol:($11/$2) with points t 'Rcvd',\
     dataRaw using xCol:($10/$2) with points t 'Hold',\
     dataRaw using xCol:($12/$2) with points t 'Better'

############################################
# plot all failure BPDU counters 

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';
if(xCol==7) set xlabel 'TxHoldPeriod';
if(xCol==8) set xlabel 'TxHoldCount';
 
set ylabel 'Number of {BPDUs,Ports}';

set title "RSTP - BPDU counters after failure";
plot dataRaw using xCol:($22/$2)  with points t 'Rcvd',\
     dataRaw using xCol:($21/$2) with points t 'Hold',\
     dataRaw using xCol:($23/$2) with points t 'Better'



unset multiplot

##############################################################################
#1 Id:  Execution Id
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
#15 REA: Worse BPDU with Agreement received (only RSTP, from Root/Alternate)
#16 WA: Equal BPDU with Agreement received (only RSTP, from Root/Alternate)
#17 ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure
#18 PCT1: Protocol convergence time (last port transition to forwarding) before failure
#19 Fai: Time when link fails
#20 Snd: Total Send BPDUs (after failure)
#21 Hld: Total Received BPDUs (after failure)
#22 Rcv: Total Received BPDUs (after failure)
#23 RBD: Better BPDU received from Designated (after failure)
#24 RWD: Worse BPDU received from Designated (after failure)
#25 RED: Equal BPDU received from Designated (after failure)
#26 REA: Worse BPDU with Agreement received from Root/Alternate (after failure)
#27 RWA: Equal BPDU with Agreement received from Root/Alternate (after failure)
#28 MAf: First expiration of Message Age
#29 MAl: Last expiration of Message Age
#30 ACT2: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after last mess age expiration
#31 PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration
#32 ACT3: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after failure
#33 PCT3: Protocol convergence time (last port transition to forwarding) after failure




