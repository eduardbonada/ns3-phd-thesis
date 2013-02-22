BEGIN{
  bF=0;   # stats index before failure
  aF=1;   # stats index after failure
  ind=bF;
  
}

# key input parameters that vary
/numNodes/          		  { num_nodes=$2} 
/forcedRootNode/   			  { forced_root_node=$2}
/nodeAToFail/     			  { node_a_to_fail=$2} 
/nodeBToFail/        		  { node_b_to_fail=$2}
/xstpMaxBpduTxHoldPeriod/ { max_tx_hold=-1 }
/txHoldPeriod/            { tx_hold_period=$2 }
  
# general counters
/Link to port/       { num_ports++;}

# check before/after failure
/EthPointToPointChannel\(\): Failed Link/{
  ind=aF;
  time_link_failure=$1;
}

# Messages counters
###############
/Sends HELLO/{
  num_hellos_sent[ind]++;
  num_bytes_hello_sent[ind] = num_bytes_hello_sent[ind] + $8;
}

/Recvs HELLO/{
  num_hellos_rcvd[ind]++;
  num_bytes_hello_rcvd[ind] = num_bytes_hello_rcvd[ind] + $8;
}

/Sends LSP/{
  num_lsps_sent[ind]++;
  num_bytes_lsp_sent[ind] = num_bytes_lsp_sent[ind] + $9;
}

/Recvs LSP/{
  num_lsps_rcvd[ind]++;
  num_bytes_lsp_rcvd[ind] = num_bytes_lsp_rcvd[ind] + $9;
}

/Holds LSP/{
  num_lsps_held[ind]++;
}

/Higher Sequence Number/{
  num_better_lsps_rcvd[ind]++;
  time_last_LSP[ind]=$1;   
}

/Lower Sequence Number/{
  num_worse_lsps_rcvd[ind]++;
}

/Same Sequence Number/{
  num_same_lsps_rcvd[ind]++;
}

/Sends TAP BPDU/{
  num_bpdus_sent[ind]++;
  num_bytes_bpdu_sent[ind] = num_bytes_bpdu_sent[ind] + $8;
}

/Recvs TAP BPDU/{
  num_bpdus_rcvd[ind]++;
  num_bytes_bpdu_rcvd[ind] = num_bytes_bpdu_rcvd[ind] + $8;
}

# COMMON Special events
#######################
/Compute trees/{
  num_computations[ind]++;
}

/All nodes have the same LSP-DB/{
  time_all_nodes_same_lspdb[ind]=$1; 
}

/FRW State/{
  time_last_transition_forwarding[ind]=$1;
}

END{
#         "Id \tNod\tFlA\tFlB\tFrR\tPrt\tTxH  \tSdL\tRvL\tHdL\tLbe\tLwo\tLeq\tSdB\tRvB\tKB \tCmp\tACT1 \tPCT1 \tFai  \tSdL\tRvL\tHdL\tLbe\tLwo\tLeq\tSdB\tRvB\tKB \tACT2\tPCT2\n"
  printf ("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%0.3f\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.1e\t%1.1e\t%1.1e\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.1e\t%1.1e\n", exId, num_nodes, node_a_to_fail, node_b_to_fail, forced_root_node, num_ports, tx_hold_period, num_lsps_sent[bF], num_lsps_rcvd[bF], num_lsps_held[bF], num_better_lsps_rcvd[bF], num_worse_lsps_rcvd[bF], num_same_lsps_rcvd[bF], num_bpdus_sent[bF], num_bpdus_rcvd[bF], (num_bytes_hello_rcvd[bF] + num_bytes_lsp_rcvd[bF] + num_bytes_bpdu_sent[bF])/1024, num_computations[bF], time_last_LSP[bF]/1000000000, time_last_transition_forwarding[bF]/1000000000, time_link_failure/1000000000, num_lsps_sent[aF], num_lsps_rcvd[aF], num_lsps_held[aF], num_better_lsps_rcvd[aF], num_worse_lsps_rcvd[aF], num_same_lsps_rcvd[aF],num_bpdus_sent[aF], num_bpdus_rcvd[aF], (num_bytes_hello_rcvd[aF] + num_bytes_lsp_rcvd[aF] + num_bytes_bpdu_sent[aF])/1024, time_last_LSP[aF]/1000000000 - time_link_failure/1000000000, time_last_transition_forwarding[aF]/1000000000 - time_link_failure/1000000000);
}

