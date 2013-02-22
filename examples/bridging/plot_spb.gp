reset

set terminal png size 600,1200
data = 'output/outTableRaw_spbIsIs.txt';
set output "output/spb.png"

set size 1,3
set origin 0,0
set multiplot layout 3,1

xCol = 2

############################################
# plot all LSP-BPDU init counters 

set key top left
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';

set ylabel 'Number of {LSPs,BPDUs,Ports}';

set title "SPB - Message counters initial convergence";
plot data using xCol:($11/$6) with linespoints t 'Recvd LSPs',\
     data using xCol:($12/$6) with linespoints t 'Rcvd_HighSeq',\
     data using xCol:($13/$6) with linespoints t 'Rcvd_LowSeq',\
     data using xCol:($14/$6) with linespoints t 'Rcvd_EqSeq'


############################################
# plot all LSP-BPDU reconfiguration counters 

set key top left
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';

set ylabel 'Number of {LSPs,BPDUs,Ports}';

set title "SPB - Message counters failure recovery";
plot data using xCol:($20/$6) with linespoints t 'Recvd LSPs',\
     data using xCol:($21/$6) with linespoints t 'Rcvd_HighSeq',\
     data using xCol:($22/$6) with linespoints t 'Rcvd_LowSeq',\
     data using xCol:($23/$6) with linespoints t 'Rcvd_EqSeq'


############################################
# plot convergence times 

set key top left
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';

set ylabel 'Time (sec)';
set title "STP - Convergence Times";
plot data using xCol:16  with linespoints t 'Alg. init' lt rgb "blue",\
     data using xCol:17 with linespoints t 'Prot. init' lt rgb "blue",\
     data using xCol:24  with linespoints t 'Alg. fail' lt rgb "green",\
     data using xCol:25 with linespoints t 'Prot. fail' lt rgb "green"

unset multiplot


##############################################################################
#1 Id:  Execution Id
#2 Nod: Number of nodes
#3 FlA: Node A to fail
#4 FlB: Node B to fail
#5 FrR: Node forced as root (bridge id set to 0)
#6 Prt: Number of ports (square of the number of links)
#7 TxH: Maximum Number of BPDUs to be send per TxHold period
#8 SdH: Total Send HELLOs
#9 RvH: Total Received HELLOs
#10 SdL: Total Send LSPs
#11 RvL: Total Received LSPs
#12 Lbe: Received LSPs carrying a higher sequence number
#13 Lwo: Received LSPs carrying a lower sequence number
#14 Leq: Received LSPs carrying an equal sequence number
#15 SdB: Total Send BPDUs
#16 RvB: Total Received BPDUs
#17 BCo: Received BPDUs carrying a contract
#18 BAg: Received BPDUs carrying an agreement
#19 Cmp: Number of topology computations
#20 ACT1: Time of last topology computation
#21 PCT1: Time of last transition to FRW
#22 Fai: Time when link fails
#23 SdL: Total Send LSPs (after failure)
#24 RvL: Total Received LSPs (after failure)
#25 Lbe: Received LSPs carrying a higher sequence number (after failure)
#26 Lwo: Received LSPs carrying a lower sequence number (after failure)
#27 Leq: Received LSPs carrying an equal sequence number (after failure)
#28 SdB: Total Send BPDUs (after failure)
#29 RvB: Total Received BPDUs (after failure)
#30 BCo: Received BPDUs carrying a contract (after failure)
#31 BAg: Received BPDUs carrying an agreement (after failure)
#32 ACT2: Time of last topology computation (after failure)
#33 PCT2: Time of last transition to FRW (after failure)



