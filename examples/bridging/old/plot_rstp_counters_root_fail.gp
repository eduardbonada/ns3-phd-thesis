reset

data = 'output/outTable_rstp.txt';

set terminal png
set key top left

############################################
# plot all BPDU counters per txHold scenario

xCol = 2

if(xCol==1)
  set xlabel 'Executions';
if(xCol==2)
  set xlabel 'Number of Nodes';
if(xCol==7)
  set xlabel 'Num of BPDUs per txHold';
  
  


set output data."_counters_rootFail.png"
set title "RSTP - BPDU counters root failure";

set ylabel 'Number of {BPDUs,Ports}';
plot data using xCol:6  with linespoints t 'Number of Ports',\
     data using xCol:19  with linespoints t 'Send',\
     data using xCol:20 with linespoints t 'Hold',\
     data using xCol:21 with linespoints t 'Rcvd',\
     data using xCol:22 with linespoints t 'Rcvd_Better_Des',\
     data using xCol:23 with linespoints t 'Rcvd_Worse_Des',\
     data using xCol:24 with linespoints t 'Rcvd_Equal_Des',\
     data using xCol:25 with linespoints t 'Rcvd_Worse_R-A',\
     data using xCol:26 with linespoints t 'Rcvd_Equal_R-A'
     
##############################################################################
#1 Id:  Execution Id
#2 Nod: Number of nodes
#3 FlA: Node A to fail
#4 FlB: Node B to fail
#5 FrR: Node forced as root (bridge id set to 0)
#6 Prt: Number of ports (square of the number of links)
#7 TxH: Maximum Number of BPDUs to be send per TxHold period
#8 Snd: Total Send BPDUs
#9 Hld: Hold BPDUs that had to be sent
#10 Rcv: Total Received BPDUs
#11 RBD: Better BPDU received (from Designated in RSTP)
#12 RWD: Worse BPDU received (from Designated in RSTP)
#13 RED: Equal BPDU received (from Designated in RSTP)
#14 REA: Worse BPDU with Agreement received (only RSTP, from Root/Alternate)
#15 WA: Equal BPDU with Agreement received (only RSTP, from Root/Alternate)
#16 ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure
#17 PCT1: Protocol convergence time (last port transition to forwarding) before failure
#18 Fai: Time when link fails
#19 Snd: Total Send BPDUs (after failure)
#20 Hld: Total Received BPDUs (after failure)
#21 Rcv: Total Received BPDUs (after failure)
#22 RBD: Better BPDU received from Designated (after failure)
#23 RWD: Worse BPDU received from Designated (after failure)
#24 RED: Equal BPDU received from Designated (after failure)
#25 REA: Worse BPDU with Agreement received from Root/Alternate (after failure)
#26 RWA: Equal BPDU with Agreement received from Root/Alternate (after failure)
#27 MAf: First expiration of Message Age
#28 MAl: Last expiration of Message Age
#29 ACT2: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after last mess age expiration
#30 PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration
#31 ACT3: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after failure
#32 PCT3: Protocol convergence time (last port transition to forwarding) after failure



