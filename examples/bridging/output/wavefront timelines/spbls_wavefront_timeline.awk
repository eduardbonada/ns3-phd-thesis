# Creation of the wavefront timeline using a single execution file
# parses the traces file and look for Received LSPs
# creates an output file with a table: col1=>nodeId col2=>time

# use from output folder:
# awk -f spbls_wavefront_timeline.awk n64_grid4_rstp_txHld1_1000000_rnd_r0_srX_f0-all_0.txt > spbls_wavefront_timeline.txt

BEGIN{
  cold_start=0;
  failure=1;
  
  if(cold_start==1) capture=1;
  if(failure==1) capture=0;
}

# key input parameters that vary
/numNodes/          		  { num_nodes=$2} 
/nodeAToFail/     			  { node_a_to_fail=$2} 
/nodeBToFail/        		  { node_b_to_fail=$2}

# check before/after failure
/EthPointToPointChannel\(\): Failed Link/{
  if(failure==1) capture=1;
  time_link_failure=$1;
}

# Match nodeId<=>bridgeId
#########################
/Installing SPB-LS with bridge Id/{
  split($1,a,"[");
  split(a[2],b,"]");

  bridgeId2nodeId[$8]=b[1]
  nodeId2bridgeId[b[1]]=$8
}  

# LSP counters
###############
/Recvs LSP/{
  if(capture==1)
    {
      split($6,a,"|");
      split(a[1],b,":");
  
      if(cold_start==1) print b[2],"\t",$1
      if(failure==1) print b[2],"\t",$1-time_link_failure
    }
}


END{
}

