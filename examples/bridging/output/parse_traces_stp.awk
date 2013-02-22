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
/txHoldPeriod/        { tx_hold_period=$2 }
/xstpMaxBpduTxHoldPeriod/ { max_tx_hold=$2 }
  
# general counters
/Link to port/       { num_ports++;}

# check before/after failure
/EthPointToPointChannel\(\): Failed Link/{
  ind=aF;
  time_link_failure=$1;
}

# BPDU counters
###############
/Sends BPDU/{
  num_bpdus_sent[ind]++;
}

/Recvs BPDU/{
  num_bpdus_rcvd[ind]++;
}

/periodical BPDU/{
  num_bpdus_sent_periodical[ind]++;
}

/Holds BPDU/{
  num_bpdus_hold[ind]++;
}

#/pending txHold/          {
#  num_bpdus_sent_txhold[ind]++;
#}

#/maxAge expiration/{
#  num_bpdus_sent_maxage[ind]++;
#}



# STP counters (received detail)
#################
/Better from Designated/{
  num_bpdus_rcvd_better[ind]++;
  time_last_better_bpdu[ind]=$1;

  #update final counters at this point of potential last better bpdu received
  num_bpdus_rcvd_final[ind]=num_bpdus_rcvd[ind];
  num_bpdus_sent_final[ind]=num_bpdus_sent[ind];
  num_bpdus_hold_final[ind]=num_bpdus_hold[ind];
  num_bpdus_rcvd_better_final[ind]=num_bpdus_rcvd_better[ind];
  num_bpdus_rcvd_worse_final[ind]=num_bpdus_rcvd_worse[ind];
  num_bpdus_rcvd_equal_final[ind]=num_bpdus_rcvd_equal[ind];
}

/Worse from Designated/{
  num_bpdus_rcvd_worse[ind]++;
}
  
/Equal from Designated/{
  num_bpdus_rcvd_equal[ind]++;
}

# STP counters (reasons of send BPDUs)
##############
#/disseminate better BPDU/{
#  num_bpdus_sent_better[ind]++;
#}

#/reply worse BPDU/{
#  num_bpdus_sent_worse[ind]++;
#}

#/disseminate equal BPDU/{
#  num_bpdus_sent_equal[ind]++;
#}


# Special events
################
/STP: FRW State/{
  time_last_transition_forwarding[ind]=$1;
}


/STP: MessAgeExp/{
  if(time_first_mess_age_exp[ind]==0) time_first_mess_age_exp[ind]=$1;
  time_last_mess_age_exp[ind]=$1;
}

END{

# select the latest among last_better_bpdu and last_mess_age_exp as last algorithm step after failure (in some cases there are no better BPDUs received...)
time_last_algorihtm_reconf[aF]=time_last_better_bpdu[aF];
if(time_last_algorihtm_reconf[aF]<time_last_mess_age_exp[aF]) time_last_algorihtm_reconf[aF]=time_last_mess_age_exp[aF];

#check if there were no MessAge expirations after failure (so direct link failure detection was activated)
if(time_first_mess_age_exp[aF]==0)
  {
    act2 = 0.0;
    pct2 = 0.0;
  }
else 
  {
    act2 = (time_last_algorihtm_reconf[aF] - time_last_mess_age_exp[aF])/1000000000;
    pct2 = (time_last_transition_forwarding[aF] - time_last_mess_age_exp[aF])/1000000000;
  }

#         "Id \tNod\tFlA\tFlB\tFrR\tPrt\tHlP  \tHlC\tSnd\tHld\tRcv\tRBD\tRWD\tRED\tACT1 \tPCT1 \tFai  \tSnd\tHld\tRcv\tRBD\tRWD\tRED\tMAf  \tMAl  \tACT2 \tPCT2 \tACT3 \tPCT3\n"
  printf ("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.1f\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.4f\t%1.4f\t%1.1f\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\n", exId, num_nodes, node_a_to_fail, node_b_to_fail, forced_root_node, num_ports, tx_hold_period, max_tx_hold, num_bpdus_sent_final[bF], num_bpdus_hold_final[bF], num_bpdus_rcvd_final[bF], num_bpdus_rcvd_better_final[bF], num_bpdus_rcvd_worse_final[bF], num_bpdus_rcvd_equal_final[bF], time_last_better_bpdu[bF]/1000000000, time_last_transition_forwarding[bF]/1000000000, time_link_failure/1000000000, num_bpdus_sent_final[aF], num_bpdus_hold_final[aF], num_bpdus_rcvd_final[aF], num_bpdus_rcvd_better_final[aF], num_bpdus_rcvd_worse_final[aF], num_bpdus_rcvd_equal_final[aF],  time_first_mess_age_exp[aF]/1000000000, time_last_mess_age_exp[aF]/1000000000, act2, pct2, (time_last_algorihtm_reconf[aF] - time_link_failure)/1000000000, (time_last_transition_forwarding[aF] - time_link_failure)/1000000000);
  
#         "Id \tNod\tFlA\tFlB\tFrR\tPrt\tTxH\tSnd\tHld\tRcv\tRBD\tRWD\tRED\tACT1 \tPCT1 \tFai  \tSnd\tHld\tRcv\tRBD\tRWD\tRED\tMAf  \tMAl  \tACT2 \tPCT2 \tACT3 \tPCT3\n"
#  printf ("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.3e\t%1.3e\t%1.1e\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.3e\t%1.3e\t%1.3e\t%1.3e\t%1.3e\t%1.3e\n", exId, num_nodes, node_a_to_fail, node_b_to_fail, forced_root_node, num_ports, max_tx_hold, num_bpdus_sent_final[bF], num_bpdus_hold_final[bF], num_bpdus_rcvd_final[bF], num_bpdus_rcvd_better_final[bF], num_bpdus_rcvd_worse_final[bF], num_bpdus_rcvd_equal_final[bF], time_last_better_bpdu[bF]/1000000000, time_last_transition_forwarding[bF]/1000000000, time_link_failure/1000000000, num_bpdus_sent_final[aF], num_bpdus_hold_final[aF], num_bpdus_rcvd_final[aF], num_bpdus_rcvd_better_final[aF], num_bpdus_rcvd_worse_final[aF], num_bpdus_rcvd_equal_final[aF],  time_first_mess_age_exp[aF]/1000000000, time_last_mess_age_exp[aF]/1000000000, act2, pct2, (time_last_algorihtm_reconf[aF] - time_link_failure)/1000000000, (time_last_transition_forwarding[aF] - time_link_failure)/1000000000);
}

