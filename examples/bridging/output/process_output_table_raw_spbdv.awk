BEGIN{
  bF=0
  aF=1
  num_samples=0
}

//{
  #$2 == valueCol => numNodes
  #$7 == valueCol => TxHCount
  if($7 == valueCol)
    {
      num_samples = num_samples + 1
      
      exId     	 			 = 0
      num_nodes 			 = $2
      node_a_to_fail	 = $3
      node_b_to_fail 	 = $4
      forced_root_node = $5
      num_ports        = $6
      max_tx_hold      = $7
      

      num_merged_rcvd_final[bF] 		  = num_merged_rcvd_final[bF]*( (num_samples-1)/num_samples ) + $8/num_samples 
      num_bpdus_rcvd_final[bF] 				= num_bpdus_rcvd_final[bF]*( (num_samples-1)/num_samples ) + $9/num_samples
      num_bytes_rcvd_final[bF] 				= num_bytes_rcvd_final[bF]*( (num_samples-1)/num_samples ) + $10/num_samples
      num_bpdus_hold_final[bF] 				= num_bpdus_hold_final[bF]*( (num_samples-1)/num_samples ) + $11/num_samples
      num_bpdus_rcvd_better_final[bF] = num_bpdus_rcvd_better_final[bF]*( (num_samples-1)/num_samples ) + $12/num_samples
      num_bpdus_rcvd_worse_final[bF] 	= num_bpdus_rcvd_worse_final[bF]*( (num_samples-1)/num_samples ) + $13/num_samples
      num_bpdus_rcvd_equal_final[bF] 	= num_bpdus_rcvd_equal_final[bF]*( (num_samples-1)/num_samples ) + $14/num_samples     
      num_bpdus_rcvd_worse_agreement_final[bF] = num_bpdus_rcvd_worse_agreement_final[bF]*( (num_samples-1)/num_samples ) + $15/num_samples
      num_bpdus_rcvd_equal_agreement_final[bF] = num_bpdus_rcvd_equal_agreement_final[bF]*( (num_samples-1)/num_samples ) + $16/num_samples
      
      act1 = act1*( (num_samples-1)/num_samples ) + ($17*1000000000)/num_samples
      pct1 = pct1*( (num_samples-1)/num_samples ) + ($18*1000000000)/num_samples
      
      time_link_failure = $19*1000000000
      
      num_merged_rcvd_final[aF]				= num_merged_rcvd_final[aF]*( (num_samples-1)/num_samples ) + $20/num_samples
      num_bpdus_rcvd_final[aF]        = num_bpdus_rcvd_final[aF]*( (num_samples-1)/num_samples ) + $21/num_samples
      num_bytes_rcvd_final[aF]        = num_bytes_rcvd_final[aF]*( (num_samples-1)/num_samples ) + $22/num_samples      
      num_bpdus_hold_final[aF]        = num_bpdus_hold_final[aF]*( (num_samples-1)/num_samples ) + $23/num_samples
      num_bpdus_rcvd_better_final[aF]	= num_bpdus_rcvd_better_final[aF]*( (num_samples-1)/num_samples ) + $24/num_samples
      num_bpdus_rcvd_worse_final[aF]	= num_bpdus_rcvd_worse_final[aF]*( (num_samples-1)/num_samples ) + $25/num_samples
      num_bpdus_rcvd_equal_final[aF]	= num_bpdus_rcvd_equal_final[aF]*( (num_samples-1)/num_samples ) + $26/num_samples
      num_bpdus_rcvd_worse_agreement_final[aF] = num_bpdus_rcvd_worse_agreement_final[aF]*( (num_samples-1)/num_samples ) + $27/num_samples
      num_bpdus_rcvd_equal_agreement_final[aF] = num_bpdus_rcvd_equal_agreement_final[aF]*( (num_samples-1)/num_samples ) + $28/num_samples
            
      time_first_mess_age_exp[aF]     = time_first_mess_age_exp[aF]*( (num_samples-1)/num_samples ) + ($29*1000000000)/num_samples
      time_last_mess_age_exp[aF]      = time_last_mess_age_exp[aF]*( (num_samples-1)/num_samples ) + ($30*1000000000)/num_samples

      act2 = act2*( (num_samples-1)/num_samples ) + ($31*1000000000)/num_samples
      pct2 = pct2*( (num_samples-1)/num_samples ) + ($32*1000000000)/num_samples
      
      act3 = act3*( (num_samples-1)/num_samples ) + ($33*1000000000)/num_samples
      pct3 = pct3*( (num_samples-1)/num_samples ) + ($34*1000000000)/num_samples    
    }
    
}

END{

#         "Id \tNod\tFlA\tFlB\tFrR\tPrt\tHlC\tRMg  \tRcv  \tKB   \tHld  \tRBD  \tRWD  \tRED  \tRWA  \tREA  \tACT1 \tPCT1 \tFai  \tRMg  \tRcv  \tKB   \tHld  \tRBD  \tRWD  \tRED  \tRWA  \tREA  \tMAf  \tMAl  \tACT2 \tPCT2 \tACT3 \tPCT3\n"
  printf ("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.4f\t%1.4f\t%1.1f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\n", exId, num_nodes, node_a_to_fail, node_b_to_fail, forced_root_node, num_ports, max_tx_hold, num_merged_rcvd_final[bF], num_bpdus_rcvd_final[bF], num_bytes_rcvd_final[bF], num_bpdus_hold_final[bF], num_bpdus_rcvd_better_final[bF], num_bpdus_rcvd_worse_final[bF], num_bpdus_rcvd_equal_final[bF], num_bpdus_rcvd_worse_agreement_final[bF], num_bpdus_rcvd_equal_agreement_final[bF], act1/1000000000, pct1/1000000000, time_link_failure/1000000000, num_merged_rcvd_final[aF], num_bpdus_rcvd_final[aF], num_bytes_rcvd_final[aF],  num_bpdus_hold_final[aF], num_bpdus_rcvd_better_final[aF], num_bpdus_rcvd_worse_final[aF], num_bpdus_rcvd_equal_final[aF], num_bpdus_rcvd_worse_agreement_final[aF], num_bpdus_rcvd_equal_agreement_final[aF], time_first_mess_age_exp[aF]/1000000000, time_last_mess_age_exp[aF]/1000000000, act2/1000000000, pct2/1000000000, act3/1000000000, pct3/1000000000);
}






