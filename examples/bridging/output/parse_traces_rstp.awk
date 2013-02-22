BEGIN{
  iC=0;   # stats index during init convergence
  iP=1;   # stats index between init convergence and failure (periodical)
  fC=2;   # stats index during failure convergence 
  fP=3;   # stats index after failure convergence 
  ind=iC;
  
}

# key input parameters that vary
/numNodes:/          		  { num_nodes=$2;} 
/forcedRootNode:/   			  { forced_root_node=$2}
/nodeAToFail:/     			  { node_a_to_fail=$2} 
/nodeBToFail:/        		  { node_b_to_fail=$2}
/txHoldPeriod:/        { tx_hold_period=$2 }
/xstpMaxBpduTxHoldPeriod:/ { max_tx_hold=$2 }
  
# general counters
/Link to port/       { num_ports++;}

# check before/after failure
/EthPointToPointChannel\(\): Failed Link/{
  ind=fC;
  time_link_failure=$1;
  time_last_transition_forwarding[fC]=$1;
  time_last_algorihtm_reconf[fC]=$1;
}

# BPDU counters
###############
/Sends BPDU/{
  num_bpdus_sent[ind]++;
}

/Sends CONF/{
  num_bpdus_sent[ind]++;
}

/Recvs BPDU/{
  num_bpdus_rcvd[ind]++;
}

/Recvs CONF/{
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

# RSTP counters (received detail)
#################
/Better from Designated/{
  num_bpdus_rcvd_better[ind]++;
  time_last_better_bpdu[ind]=$1;
  
  #update final counters at this point of potential last better bpdu received 
  num_bpdus_sent_final[ind]=num_bpdus_sent[ind];
  num_bpdus_hold_final[ind]=num_bpdus_hold[ind];
  num_bpdus_rcvd_final[ind]=num_bpdus_rcvd[ind];
  num_bpdus_rcvd_better_final[ind]=num_bpdus_rcvd_better[ind];
  num_bpdus_rcvd_worse_final[ind]=num_bpdus_rcvd_worse[ind];
  num_bpdus_rcvd_equal_final[ind]=num_bpdus_rcvd_equal[ind];
  num_bpdus_rcvd_worse_agreement_final[ind]=num_bpdus_rcvd_worse_agreement[ind];
  num_bpdus_rcvd_equal_agreement_final[ind]=num_bpdus_rcvd_equal_agreement[ind];
  num_bpdus_rcvd_discarded_agreement_final[ind]=num_bpdus_rcvd_discarded_agreement[ind];
}

/Worse from Designated/{
  num_bpdus_rcvd_worse[ind]++;

  #update final counters at this point of potential last better bpdu received 
  num_bpdus_sent_final[ind]=num_bpdus_sent[ind];
  num_bpdus_hold_final[ind]=num_bpdus_hold[ind];
  num_bpdus_rcvd_final[ind]=num_bpdus_rcvd[ind];
  num_bpdus_rcvd_better_final[ind]=num_bpdus_rcvd_better[ind];
  num_bpdus_rcvd_worse_final[ind]=num_bpdus_rcvd_worse[ind];
  num_bpdus_rcvd_equal_final[ind]=num_bpdus_rcvd_equal[ind];
  num_bpdus_rcvd_worse_agreement_final[ind]=num_bpdus_rcvd_worse_agreement[ind];
  num_bpdus_rcvd_equal_agreement_final[ind]=num_bpdus_rcvd_equal_agreement[ind];
  num_bpdus_rcvd_discarded_agreement_final[ind]=num_bpdus_rcvd_discarded_agreement[ind];
}
  
/Equal from Designated/{
  num_bpdus_rcvd_equal[ind]++;
}

/Worse from Root-Alternate with Agreement/{
  num_bpdus_rcvd_worse_agreement[ind]++;
  time_last_agreement[ind]=$1;
}

/Equal from Root-Alternate with Agreement/{
  num_bpdus_rcvd_equal_agreement[ind]++;
  time_last_agreement[ind]=$1;
}

/Discarded Agreement/{
  num_bpdus_rcvd_discarded_agreement[ind]++;
}


# COMMON Special events
#######################
/STP: FRW State/{
  time_last_transition_forwarding[ind]=$1;

  #update final counters at this point of potential last better bpdu received 
  num_bpdus_sent_final[ind]=num_bpdus_sent[ind];
  num_bpdus_hold_final[ind]=num_bpdus_hold[ind];
  num_bpdus_rcvd_final[ind]=num_bpdus_rcvd[ind];
  num_bpdus_rcvd_better_final[ind]=num_bpdus_rcvd_better[ind];
  num_bpdus_rcvd_worse_final[ind]=num_bpdus_rcvd_worse[ind];
  num_bpdus_rcvd_equal_final[ind]=num_bpdus_rcvd_equal[ind];
  num_bpdus_rcvd_worse_agreement_final[ind]=num_bpdus_rcvd_worse_agreement[ind];
  num_bpdus_rcvd_equal_agreement_final[ind]=num_bpdus_rcvd_equal_agreement[ind];
  num_bpdus_rcvd_discarded_agreement_final[ind]=num_bpdus_rcvd_discarded_agreement[ind];
}


/STP: MessAgeExp/{
  if(time_first_mess_age_exp[ind]==0) time_first_mess_age_exp[ind]=$1;
  time_last_mess_age_exp[ind]=$1;
}


END{

# select the latest among last_better_bpdu and last_mess_age_exp as last algorithm step after failure (in some cases there are no better BPDUs received...)
if(time_last_better_bpdu[fC]!=0) time_last_algorihtm_reconf[fC]=time_last_better_bpdu[fC];
if(time_last_algorihtm_reconf[fC]<time_last_mess_age_exp[fC]) time_last_algorihtm_reconf[fC]=time_last_mess_age_exp[fC];

#check if there were no MessAge expirations after failure (so direct link failure detection was activated)
if(time_first_mess_age_exp[fC]==0)
  {
    act2 = 0.0;
    pct2 = 0.0;
  }
else 
  {
    act2 = (time_last_algorihtm_reconf[fC] - time_first_mess_age_exp[fC])/1000000000;
    pct2 = (time_last_transition_forwarding[fC] - time_first_mess_age_exp[fC])/1000000000;
  }



#         "Id \tNod\tFlA\tFlB\tFrR\tPrt\tHlP  \tHlC\tSnd\tHld\tRcv\tRBD\tRWD\tRED\tRWA\tRDA\tACT1 \tPCT1 \tFai  \tSnd\tHld\tRcv\tRBD\tRWD\tRED\tRWA\tRDA\tMAf  \tMAl  \tACT2 \tPCT2 \tACT3 \tPCT3\n"
  printf ("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.0f\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.4f\t%1.4f\t%1.1f\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\n", exId, num_nodes, node_a_to_fail, node_b_to_fail, forced_root_node, num_ports, tx_hold_period, max_tx_hold, num_bpdus_sent_final[iC],num_bpdus_hold_final[iC], num_bpdus_rcvd_final[iC], num_bpdus_rcvd_better_final[iC], num_bpdus_rcvd_worse_final[iC], num_bpdus_rcvd_equal_final[iC], num_bpdus_rcvd_worse_agreement_final[iC], num_bpdus_rcvd_discarded_agreement_final[iC], time_last_better_bpdu[iC]/1000000000, time_last_transition_forwarding[iC]/1000000000, time_link_failure/1000000000, num_bpdus_sent_final[fC],num_bpdus_hold_final[fC], num_bpdus_rcvd_final[fC], num_bpdus_rcvd_better_final[fC], num_bpdus_rcvd_worse_final[fC], num_bpdus_rcvd_equal_final[fC], num_bpdus_rcvd_worse_agreement_final[fC], num_bpdus_rcvd_discarded_agreement_final[fC], time_first_mess_age_exp[fC]/1000000000, time_last_mess_age_exp[fC]/1000000000, act2, pct2, (time_last_algorihtm_reconf[fC] - time_link_failure)/1000000000, (time_last_transition_forwarding[fC] - time_link_failure)/1000000000);
}

