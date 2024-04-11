if command -v pip &> /dev/null
then
   pip install .
elif command -v pip3 &> /dev/null
then
   pip3 install .
fi