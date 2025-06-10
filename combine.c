#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
//#include <cilk/cilk_api.h>

/*  
    Process the data stored in the input file, and write the processed data to the output file.

    Usage: combine [input file name] [output file name]

    Input file format: n classes of m vectors v_i_j, arranged as follows: 
    Line 1: n, m, c_1, c_2, ..., c_n: integers separated by columns, where:
    n: the number of classes of v_i
    m: the size of each class
    c_i: the length of the vectors in class i
    Subsequent lines: m lines of c_1 ints separated by ", ", each denoting m vectors separated by new lines. 
    Then, m lines of length c_2, and so on. 

    Output file format: m lines of C ints separated by ", ", where C is the sum of all of the c_i in the input file
*/ 
int main(int argc, char *argv[]) {
    if (argc != 3){
        printf("Usage: combine [input file name] [output file name]\n");
        exit(1);
    }
    char* filename = argv[1];
    char* outname = argv[2];
    FILE* fp = fopen(filename, "r");
    if (fp == NULL){
        perror("Error opening input file\n");
        return 1;
    }
    int fd = fileno(fp);

    int n;
    int m;
    fscanf(fp, "%d, %d", &n, &m);
    int lengths[n];
    int total_length = 0; // The length of the large vectors produced after combining
    for (int i = 0; i < n; i++){
        fscanf(fp, ", %d", &lengths[i]);
        total_length += lengths[i];
    }
    fscanf(fp, "\n"); //skip trailing newline
    long data_start = ftell(fp);
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp); // The number of characters in the file
    printf("The file is %d characters long.\n", sz);
    fseek(fp, data_start, SEEK_SET);
    //printf("%ld\n", sysconf(_SC_PAGE_SIZE));
    char* content = mmap(NULL, sz, PROT_READ, MAP_SHARED, fd, 0);
    madvise(content, sz*sizeof(char), MADV_SEQUENTIAL); // We will first process the data sequentially to scan the file
    fclose(fp);
    int nworkers = 16; //__cilkrts_get_nworkers(); TODO: Put this into a global constant
    long linestarts[n*m + 1]; // Array storing the location of each line in the memory.
    // First entry: start of the lines. Last entry: The location of the EOF.
    int idx = 0;
    //printf("%p\n", data);
    //perror("\n");
    for (int i = 0; i < sz; i++){ // Parse the input data. TODO: Parallelize this later
        if (content[i] == '\n' && i + 1 < sz){ // Found the end of the current line
            linestarts[idx] = i + 1; // Store the location of the start of the next line 
            idx += 1;
        }
    }
    linestarts[n*m] = sz + 1;
    /* for (int i = 0; i <= n*m; i++){
        printf("%ld\n", linestarts[i]);
    }  */
        for (int i = 0; i < n*m; i++) {
    printf("Line %d starts at %ld, ends at %ld, length: %ld\n", i, linestarts[i], linestarts[i+1], linestarts[i+1] - linestarts[i]);
}
    madvise(content, sz*sizeof(char), MADV_RANDOM); // Next we are reading the data in parallel, so no clear pattern to exploit
    
    fp = fopen(outname, "w+");
    if (fp == NULL){
        perror("Error opening output file\n");
        return 1;
    }
    fd = fileno(fp);

    /* Each line of the output consists of n vectors arranged on one line.
    Individual vectors in the input files are separated by "\n".
    In the output, these vectors on the same line are instead separated by ", ".
    This yields an extra character per vector per line, minus one for the last vector.
    There are m lines in the output file in total. 
    Therefore, relative to the input data, there are (n - 1) * m extra characters in the output file.
    */
    int out_sz = sz - data_start + ((n - 1) * m); // Calculated as above
    ftruncate(fd, out_sz * sizeof(char)); // Make sure the mmap is big enough to contain all data

    char *output = mmap(NULL, out_sz, PROT_WRITE, MAP_SHARED, fd, 0);
    madvise(output, out_sz*sizeof(char), MADV_RANDOM); // We write data in parallel, so no clear pattern to exploit
    
    long out_linestarts[m + 1]; // The positions of the line starts in the output file
    memset(out_linestarts, 0, (m + 1) * sizeof(long)); // Default positions are -1 since we add +1 too much in the loop
    idx = 1;
    for (int i = 0; i < m; i++){ // For each line in the output file. This doesn't need to be parallelized, it's cheap, but could be done with prefix sum we covered in class
        out_linestarts[idx] = out_linestarts[idx - 1];
        for (int j = 0; j < n; j++){ // For each line in the input file which wil go to that output line
            int line_number = i + (m * j);
            out_linestarts[idx] += (linestarts[line_number + 1] - linestarts[line_number]) - 1;
            //out_linestarts[idx] += 1; // Replace newline with ", "
        }
        out_linestarts[idx] += 2 * (n - 1); // Space for ", "
        idx += 1;
    }
    for (int i = 0; i < m; i++) {
    printf("Line %d starts at %ld, ends at %ld, length: %ld\n", i, out_linestarts[i], out_linestarts[i+1], out_linestarts[i+1] - out_linestarts[i]);
}
/* 
    for (int i = 0; i <= m; i++){
        printf("%ld\n", out_linestarts[i]);
    } 
 */
    for (int i = 0; i < m; i++){ // For each number of vector. TODO: Parallelize this
        int pos = 0; // Variable to track current position in line
        for (int j = 0; j < n; j++){ // For each class of vector. This is done serially (order is important here)
            int line_num = i + j*m; // The line number in the input file to read from
            int len = (linestarts[line_num + 1]) - (linestarts[line_num]); // Number of characters to copy
            printf("pos: %d, line_num: %d, len: %d\n", pos, line_num, len);

            memcpy(output + pos + out_linestarts[i], content + linestarts[line_num], (len - 1) * sizeof(char));
            //printf("%.10s\n", content + linestarts[line_num]);
            pos += len - 1;
            if (j < n - 1){ // Still more vectors to adjoin
                output[pos + out_linestarts[i]] = ',';
                output[pos + out_linestarts[i] + 1] = ' ';
                pos += 2;
            }
            else{ // This was the last vector, end of the line
                output[pos + out_linestarts[i]] = '\n';
                pos += 1;
            }
            pos += 1;
        }
    }

                //printf("%s\n", content + linestarts[line_num]);
/*             int k = 0;
            char c = content[k + linestarts[line_num]];
            while(k < len){
                printf("%c", c);
                k += 1;
                c = content[k + linestarts[line_num]];
            } */
    /* */
    //perror("nux");

    printf("\n");
    fclose(fp);
    return 0;
}