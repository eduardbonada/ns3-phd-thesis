

clear all;

C=1;
ratio_CM=2;
ratio_ME=2;

M=C*ratio_CM;
E=M*ratio_ME;
N=C+M+E;
costMatrix=ones(N,N)*999999;

for k=0:C-1
  for i=0:C-1
    costMatrix(k+1,i+1)=1;
  endfor
endfor

core_side=0;
m=C;
while (m<C+M-1)
  for k=0:ratio_CM-1
    costMatrix(m+k+1,mod(core_side   ,C)+1)=1;
    costMatrix(m+k+1,mod(core_side+1 ,C)+1)=1;
  endfor
  m=m+ratio_CM;
  if(C>2)
    core_side=core_side+1;
  endif
endwhile

#e=C+M;
#next_metro_node=C;
#while (e<C+M+E-1)
#endwhile
for e=C+M:N-1
  costMatrix(e +1, mod(e-M  ,M)+C +1)=1;
  costMatrix(e +1, mod(e-M+1,M)+C +1)=1;
endfor

for i=1:N
  for j=1:N
    if(i==j)
      costMatrix(i,j)=0;
    endif
    if(costMatrix(i,j)==1)
      costMatrix(j,i)=1;
    endif
    #if(costMatrix(i,j)==999999)
    #endif
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

