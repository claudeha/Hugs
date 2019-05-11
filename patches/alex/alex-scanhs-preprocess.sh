grep ^alex_action_ Scan.hs|grep -v andBegin|grep -v "alex_action_3 ="|while read line;
do
sed -i "/$line/d" Scan.hs
fname=$(echo $line|awk '{print $1}')
fcont=$(echo $line|awk '{print $3}')
sed -i "s/($fname)/($fcont)/g" Scan.hs
done 
