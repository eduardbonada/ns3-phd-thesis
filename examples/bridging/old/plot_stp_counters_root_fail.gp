reset

data = 'output/outTable_stp.txt';

set terminal png
set key top left

############################################
# plot all BPDU counters per txHold scenario

xCol = 1
set xlabel 'Executions';

set output data."_counters_rootFail.png"
set title "STP - BPDU counters root failure";

set ylabel 'Number of {BPDUs,Ports}';
plot data using xCol:6  with linespoints t 'Number of Ports',\
     data using xCol:17  with linespoints t 'Send',\
     data using xCol:18 with linespoints t 'Hold',\
     data using xCol:19 with linespoints t 'Rcvd',\
     data using xCol:20 with linespoints t 'Rcvd_Better',\
     data using xCol:21 with linespoints t 'Rcvd_Worse',\
     data using xCol:22 with linespoints t 'Rcvd_Equal'
     
##############################################################################
#1 Id:  Execution id
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
#14 ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure
#15 PCT1: Protocol convergence time (last port transition to forwarding) before failure
#16 Fai: Time when link fails
#17 Snd: Total Send BPDUs (after failure)
#18 Rcv: Total Received BPDUs (after failure)
#19 RBD: Better BPDU received (after failure)
#20 RWD: Worse BPDU received (after failure)
#21 RED: Equal BPDU received (after failure)
#22 MAf: First expiration of Message Age
#23 MAl: Last expiration of Message Age
#24 ACT2: Algorithm convergence Time (STP: last better BPDU) after first mess age expiration
#25 PCT2: Protocol convergence time (last port transition to forwarding) after first mess age expiration
#26 ACT3: Algorithm convergence Time (STP: last better BPDU) after failure
#27 PCT3: Protocol convergence time (last port transition to forwarding) after failure



