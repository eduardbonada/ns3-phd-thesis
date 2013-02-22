BEGIN{
  bF=0
  aF=1
  num_samples=0
}

//{
  #$2 == valueCol => numNodes
  #$7 == valueCol => TxHPeriod
  #$8 == valueCol => TxHCount
  if($8 == valueCol)
    {
      num_samples = num_samples + 1
      
      exId     	 			 = 0
      num_nodes 			 = $2
      node_a_to_fail	 = $3
      node_b_to_fail 	 = $4
      forced_root_node = $5
      num_ports        = $6
      tx_hold_period   = $7
      max_tx_hold      = $8
      
      num_bpdus_sent_final[bF] 				= num_bpdus_sent_final[bF]*( (num_samples-1)/num_samples ) + $9/num_samples 
      num_bpdus_hold_final[bF] 				= num_bpdus_hold_final[bF]*( (num_samples-1)/num_samples ) + $10/num_samples
      num_bpdus_rcvd_final[bF] 				= num_bpdus_rcvd_final[bF]*( (num_samples-1)/num_samples ) + $11/num_samples
      num_bpdus_rcvd_better_final[bF] = num_bpdus_rcvd_better_final[bF]*( (num_samples-1)/num_samples ) + $12/num_samples
      num_bpdus_rcvd_worse_final[bF] 	= num_bpdus_rcvd_worse_final[bF]*( (num_samples-1)/num_samples ) + $13/num_samples
      num_bpdus_rcvd_equal_final[bF] 	= num_bpdus_rcvd_equal_final[bF]*( (num_samples-1)/num_samples ) + $14/num_samples     
      num_bpdus_rcvd_worse_agreement_final[bF] = num_bpdus_rcvd_worse_agreement_final[bF]*( (num_samples-1)/num_samples ) + $15/num_samples
      num_bpdus_rcvd_equal_agreement_final[bF] = num_bpdus_rcvd_equal_agreement_final[bF]*( (num_samples-1)/num_samples ) + $16/num_samples
      
      act1 = act1*( (num_samples-1)/num_samples ) + ($17*1000000000)/num_samples
      pct1 = pct1*( (num_samples-1)/num_samples ) + ($18*1000000000)/num_samples
      
      time_link_failure = $19*1000000000
      
      num_bpdus_sent_final[aF]				= num_bpdus_sent_final[aF]*( (num_samples-1)/num_samples ) + $20/num_samples
      num_bpdus_hold_final[aF]        = num_bpdus_hold_final[aF]*( (num_samples-1)/num_samples ) + $21/num_samples
      num_bpdus_rcvd_final[aF]        = num_bpdus_rcvd_final[aF]*( (num_samples-1)/num_samples ) + $22/num_samples
      num_bpdus_rcvd_better_final[aF]	= num_bpdus_rcvd_better_final[aF]*( (num_samples-1)/num_samples ) + $23/num_samples
      num_bpdus_rcvd_worse_final[aF]	= num_bpdus_rcvd_worse_final[aF]*( (num_samples-1)/num_samples ) + $24/num_samples
      num_bpdus_rcvd_equal_final[aF]	= num_bpdus_rcvd_equal_final[aF]*( (num_samples-1)/num_samples ) + $25/num_samples
      num_bpdus_rcvd_worse_agreement_final[aF] = num_bpdus_rcvd_worse_agreement_final[aF]*( (num_samples-1)/num_samples ) + $26/num_samples
      num_bpdus_rcvd_equal_agreement_final[aF] = num_bpdus_rcvd_equal_agreement_final[aF]*( (num_samples-1)/num_samples ) + $27/num_samples
            
      time_first_mess_age_exp[aF]     = time_first_mess_age_exp[aF]*( (num_samples-1)/num_samples ) + ($28*1000000000)/num_samples
      time_last_mess_age_exp[aF]      = time_last_mess_age_exp[aF]*( (num_samples-1)/num_samples ) + ($29*1000000000)/num_samples

      act2 = act2*( (num_samples-1)/num_samples ) + ($30*1000000000)/num_samples
      pct2 = pct2*( (num_samples-1)/num_samples ) + ($31*1000000000)/num_samples
      
      act3 = act3*( (num_samples-1)/num_samples ) + ($32*1000000000)/num_samples
      pct3 = pct3*( (num_samples-1)/num_samples ) + ($33*1000000000)/num_samples    
    }
    
}

END{

#         "Id \tNod\tFlA\tFlB\tFrR\tPrt\tHlP  \tHlC\tSnd  \tHld  \tRcv  \tRBD  \tRWD  \tRED  \tRWA  \tREA  \tACT1 \tPCT1 \tFai  \tSnd  \tHld  \tRcv  \tRBD  \tRWD  \tRED  \tRWA  \tREA  \tMAf  \tMAl  \tACT2 \tPCT2 \tACT3 \tPCT3\n"
  printf ("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.2f\t%2d\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.4f\t%1.4f\t%1.1f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\n", exId, num_nodes, node_a_to_fail, node_b_to_fail, forced_root_node, num_ports, tx_hold_period, max_tx_hold, num_bpdus_sent_final[bF], num_bpdus_hold_final[bF], num_bpdus_rcvd_final[bF], num_bpdus_rcvd_better_final[bF], num_bpdus_rcvd_worse_final[bF], num_bpdus_rcvd_equal_final[bF], num_bpdus_rcvd_worse_agreement_final[bF], num_bpdus_rcvd_equal_agreement_final[bF], act1/1000000000, pct1/1000000000, time_link_failure/1000000000, num_bpdus_sent_final[aF], num_bpdus_hold_final[aF], num_bpdus_rcvd_final[aF], num_bpdus_rcvd_better_final[aF], num_bpdus_rcvd_worse_final[aF], num_bpdus_rcvd_equal_final[aF], num_bpdus_rcvd_worse_agreement_final[aF], num_bpdus_rcvd_equal_agreement_final[aF], time_first_mess_age_exp[aF]/1000000000, time_last_mess_age_exp[aF]/1000000000, act2/1000000000, pct2/1000000000, act3/1000000000, pct3/1000000000);
}


#1 Id:  Execution Id
#2 Nod: Number of nodes
#3 FlA: Node A to fail
#4 FlB: Node B to fail
#5 FrR: Node forced as root (bridge id set to 0)
#6 Prt: Number of ports (square of the number of links)
#7 HlP: TxHold period
#8 HlC: Maximum Number of BPDUs to be send per TxHold period
#9 Snd: Total Send BPDUs
#10 Hld: Hold BPDUs that had to be sent
#11 Rcv: Total Received BPDUs
#12 RBD: Better BPDU received (from Designated in RSTP)
#13 RWD: Worse BPDU received (from Designated in RSTP)
#14 RED: Equal BPDU received (from Designated in RSTP)
#15 REA: Worse BPDU with Agreement received (only RSTP, from Root/Alternate)
#16 RWA: Equal BPDU with Agreement received (only RSTP, from Root/Alternate)
#17 ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure
#18 PCT1: Protocol convergence time (last port transition to forwarding) before failure
#19 Fai: Time when link fails
#20 Snd: Total Send BPDUs (after failure)
#21 Hld: Total Received BPDUs (after failure)
#22 Rcv: Total Received BPDUs (after failure)
#23 RBD: Better BPDU received from Designated (after failure)
#24 RWD: Worse BPDU received from Designated (after failure)
#25 RED: Equal BPDU received from Designated (after failure)
#26 REA: Worse BPDU with Agreement received from Root/Alternate (after failure)
#27 RWA: Equal BPDU with Agreement received from Root/Alternate (after failure)
#28 MAf: First expiration of Message Age
#29 MAl: Last expiration of Message Age
#30 ACT2: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after last mess age expiration
#31 PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration
#32 ACT3: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after failure
#33 PCT3: Protocol convergence time (last port transition to forwarding) after failure



