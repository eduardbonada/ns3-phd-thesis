

clear all;

meshlevel="light";
N=40;
costMatrix=ones(N,N)*999999;




if(strcmp(meshlevel,"light"))
  C=N/2;
  E=N-C;

  numTrunksTotal = ceil(2*(log2(C)-2)); # ring trunks + extra trunks
  numTrunksExtra = numTrunksTotal - 2; # not in the ring

  gap = floor( (C-1-numTrunksExtra)/(numTrunksExtra+1) );
endif

if(strcmp(meshlevel,"heavy"))
  C=sqrt(2*N);
  E=N-C;

  numTrunksTotal = ceil((C-1)/2); # ring trunks + extra trunks
  numTrunksExtra = numTrunksTotal - 2; # not in the ring

  gap = floor( (C-1-numTrunksExtra)/(numTrunksExtra+1) );
endif

if(strcmp(meshlevel,"full"))
  C=sqrt(2*N);
  E=N-C;
  
  numTrunksTotal = C-1; # ring trunks + extra trunks
endif

if( strcmp(meshlevel,"light") || strcmp(meshlevel,"heavy") )
  
  for i=0:C-1
    costMatrix(i+1,mod(i+1,C)+1)=1;
  endfor

  for k=0:C-1
    for i=1:numTrunksExtra
      costMatrix(k+1,mod(k+i*(gap+1),C)+1)=1;
    endfor
  endfor

  #set to ones the connections between ring and edges
  for i=C:N-1
    costMatrix(i+1,mod(i-C,C)+1)=1;
    costMatrix(i+1, mod(i-C+1,C)+1)=1;
  endfor
  
endif

if(strcmp(meshlevel,"full"))

  for k=0:C-1
    for i=0:C-1
      costMatrix(k+1,i+1)=1;
    endfor
  endfor

  #set to ones the connections between ring and edges
  for i=C:N-1
    costMatrix(i+1,mod(i-C,C)+1)=1;
    costMatrix(i+1, mod(i-C+1,C)+1)=1;
  endfor

endif

for i=1:N
  for j=1:N
    if(i==j)
      costMatrix(i,j)=0;
    endif
    if(costMatrix(i,j)==1)
      costMatrix(j,i)=1;
    endif
    if(costMatrix(i,j)==999999)
      costMatrix(i,j)=0;
    endif
  endfor
endfor

sum(sum(costMatrix))/2

save '/home/eduard/matrix.txt' costMatrix

for i=1:N
  for j=1:N
    if(costMatrix(i,j)==1)
      printf("%d--%d; ",i,j);
    endif
  endfor
  printf("\n");
endfor

