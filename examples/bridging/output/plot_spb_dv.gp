reset

set terminal png size 600,1200
data = 'output/outTable_rstp.txt';
dataRaw = 'output/outTableRaw_spbDv.txt';
set output "output/spb_dv.png"

set size 1,3
set origin 0,0
set multiplot layout 3,1

#1 => executions
#2 => numNodes
#7 => TxHCount

xCol = 2

############################################
# plot convergence times 

set key top right
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';
if(xCol==7) set xlabel 'txHC (txHP=txHC*10ms)';
if(xCol==2) set xrange [ 0 : 64 ]

set ylabel 'Time (sec)';
set title "SPBDV - Convergence Times";
plot dataRaw using xCol:18  with points t 'Cold Start' lt rgb "blue",\
     dataRaw using xCol:34  with points t 'Failure' lt rgb "green"


############################################
# plot MC & CC 

set key top right
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes'; 
if(xCol==7) set xlabel 'txHC (txHP=txHC*10ms)';
if(xCol==2) set xrange [ 0 : 64 ]

set ylabel 'Number of BPDUs per node';
set title "SPBDV - BPDU counters Cold Start";
plot dataRaw using xCol:($9/$2) with points t  'Rcvd',\
     dataRaw using xCol:($11/$2) with points t 'Hold',\
     dataRaw using xCol:($12/$2) with points t 'Triggers'

set ylabel 'Number of BPDUs per node';
set title "SPBDV - BPDU counters Failure";
plot dataRaw using xCol:($20/$2) with points t  'Rcvd',\
     dataRaw using xCol:($23/$2) with points t 'Hold',\
     dataRaw using xCol:($24/$2) with points t 'Triggers'

unset multiplot


#Id	Nod	FlA	FlB	FrR	Prt	HlC	RMg	Rcv	KB 	Hld	RBD	RWD	RED	RWA	REA	ACT1	PCT1	Fai	RMg	Rcv	KB 	Hld	RBD	RWD	RED	RWA	REA	MAf	MAl	ACT2	PCT2	ACT3	PCT3
#1	2		3		4		5		6		7		8		9		10	11	12	13	14	15	16	17		18		19	20	21	22	23	24	25	26	27	28	29	30	31		32		33		34



