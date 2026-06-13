Compile with:
gcc -std=gnu99 -pthread -o line_processor main.c

Run with input redirection:
./line_processor < input1.txt

Run with input and output redirection:
./line_processor < input1.txt > output1.txt
