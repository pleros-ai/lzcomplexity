#! /bin/bash

# Data folders
# /Users/efren_aragon/Documents/Work/Investigacion/Data/data_symmetric_two_state_process
# /Users/efren_aragon/Documents/Work/Investigacion/Data/data_even_process
search_dir=$1
file_list=`ls $search_dir/*.dat`

echo "Runing lzcomplexity..."
for entry in $file_list
do
   ./build/lzcomplexity -p 10 -j 10 -e a "$entry"
   echo -ne '.'
done
echo -ne '\n'

# Progress bar:
# echo -ne '######                    (33%)\r'
# echo -ne '#############             (66%)\r'
# echo -ne '#######################   (100%)\r'