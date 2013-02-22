for f in `ls -tr`;
do
  awk '//{if($1!="#"){print}}' $f > tmp.tmp
  rm $f; 
  mv tmp.tmp $f;
  rm tmp.tmp; 
done;
