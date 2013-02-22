# print timeline of BPDUs about root 0 (whch becomes old root after root failure)
awk '/RSTP: Recvs BPDU \[R:0/{print $1," 1"}' output/n25_grid4_rstp_txHld1_100_rnd_r0_srX_f0-all_0.txt | sed 's/ns/ /' > rootInfoTimeline.txt
  
# print timeline of BPDUs
awk '/RSTP: Recvs BPDU /{print $1," 1"}' output/n25_grid4_rstp_txHld1_100_rnd_r0_srX_f0-all_0.txt | sed 's/ns/ /' > bpdusTimeline.txt

# plot histogram with octave
load bpdusTimeline.txt
histBpdu=hist(bpdusTimeline,1000);
plot(histBpdu); axis([0 1000 0 5000])

# print timleline of received traffic messages in sinks
awk '/Receives packet/{ printf("%s\t1\n",$1);}' output/n4_grid4_spbDv_txHld1000000_1000000_rnd_rX_srX_f2-3_0.txt | sed 's/ns/ /' > trafficTimeline.txt


# plot histogram with octave
load trafficTimeline.txt
histTraffic=hist(trafficTimeline(:,1),1000);
plot(histTraffic);
axis([450 550 0 1000]);
