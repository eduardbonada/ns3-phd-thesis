reset

data = 'output/outTable_stp.txt';

set terminal png
set key top left

############################################
# plot all convergence times per txHold scenario

xCol = 1
set xlabel 'Executions';

set output data."_times.png"
set title "STP - Convergence Times";

set ylabel 'Time (sec)';
plot data using xCol:14  with linespoints t 'Alg. init' lt rgb "blue",\
     data using xCol:15 with linespoints t 'Prot. init' lt rgb "blue",\
     data using xCol:25  with linespoints t 'Alg. fail (MAexp)' lt rgb "red",\
     data using xCol:26 with linespoints t 'Prot. fail (MAexp)' lt rgb "red",\
     data using xCol:27  with linespoints t 'Alg. fail' lt rgb "green",\
     data using xCol:28 with linespoints t 'Prot. fail' lt rgb "green"

     
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



