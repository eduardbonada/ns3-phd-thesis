reset

set terminal png size 600,1200
dataRaw = 'output/outTableRaw_spbIsIs.txt';
data = 'output/outTable_spbIsIs.txt';
set output "output/spb_isis.png"

set size 1,3
set origin 0,0
set multiplot layout 3,1

xCol = 2

############################################
# plot convergence times 

set key top left
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';

if(xCol==2) set xrange [ 0 : 64 ]

set ylabel 'Time (sec)';
set title "SPBLS - Convergence Times";
plot dataRaw using xCol:18  with points t 'Cold Start(LSP)' lt rgb "blue",\
     dataRaw using xCol:19  with points t 'Cold Start(FRW)' lt rgb "blue",\
     dataRaw using xCol:30  with points t 'Failure(LSP)' lt rgb "green",\
     dataRaw using xCol:31  with points t 'Failure(FRW)' lt rgb "green"


############################################     
# plot MC & CC

set key bottom right
set grid

if(xCol==1) set xlabel 'Executions';
if(xCol==2) set xlabel 'Number of Nodes';

if(xCol==2) set xrange [ 0 : 64 ]

set ylabel 'Number of LSPs per node';
set title "SPBLS - Message Counters Cold start";
plot dataRaw using xCol:($9/$2) with points t   'Recvd',\
     dataRaw using xCol:($11/$2) with points t  'Hold',\
     dataRaw using xCol:($10/$2) with points t  'Triggers'

set ylabel 'Number of LSPs per node';
set title "SPBLS - Message Counters Failure";
plot dataRaw using xCol:($22/$2) with points t  'Recvd',\
     dataRaw using xCol:($23/$2) with points t  'Hold',\
     dataRaw using xCol:($24/$2) with points t  'Triggers'
    

unset multiplot


##############################################################################
#Id	Nod	FlA	FlB	FrR	Prt	TxH	SdL	RvL	HdL	Lbe	Lwo	Leq	SdB	RvB	KB 	Cmp	ACT1	PCT1	Fai	SdL	RvL	HdL	Lbe	Lwo	Leq	SdB	RvB	KB 	ACT2	PCT2
#1  2   3   4   5   6	  7		8		9		10	11	12	13	14	15	16	17	18		19		20	21	22	23	24	25	26	27	28	29	30		31
