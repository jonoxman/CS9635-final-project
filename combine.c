#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
//#include <cilk/cilk_api.h>


int* read_line(char *map, int line){

}

/*  Usage: combine *input file name*
    Input file format: n classes of m vectors v_i_j, arranged as follows: 
    Line 1: n, m, c_1, c_2, ..., c_n: integers separated by columns, where:
    n: the number of classes of v_i
    m: the size of each class
    c_i: the length of the vectors in class i
    Subsequent lines: m lines of c_1 ints separated by ", ", each denoting m vectors separated by new lines. 
    Then, m lines of length c_2, and so on. 
*/ 
int main(int argc, char *argv[]) {

    if (argc != 2){
        perror("Usage: combine [input file name]\n");
        return 1;
    }
    char* filename = argv[1];
    FILE* fp = fopen(filename, "r");
    if (fp == NULL){
        perror("Error opening file\n");
        return 1;
    }
    int fd = fileno(fp);

    int n;
    int m;
    fscanf(fp, "%d, %d", &n, &m);
    int lengths[n];
    int total_length = 0;
    for (int i = 0; i < n; i++){
        fscanf(fp, ", %d", &lengths[i]);
        total_length += lengths[i];
    }
    fscanf(fp, "\n"); //skip trailing newline

    long curr = ftell(fp);
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    printf("The data after the file header is %d characters long.\n", sz);
    fseek(fp, curr, SEEK_SET);
    char* data = mmap(NULL, sz, PROT_READ, MAP_SHARED, fd, curr);
    madvise(data, sz, MADV_RANDOM);

    int nworkers = 16; //__cilkrts_get_nworkers();
    

    // TODO: Split the file into as many chunks as there are workers and preprocess each chunk individually

    long linestarts[n*m]; // Array storing the location of each line in the memory
    linestarts[0] = ftell(fp);
    int idx = 1;
    for (int i = 0; i < sz; i++){ // Parse the filein
        if (data[i] == '\n' && i + 1 < sz){ // Parse
            linestarts[idx] = data + i + 1; // 
        }
    }

    fclose(fp);

    /*char temp[10];
    for (int i = 0; i < 10; i++){
        fscanf(fp, "%c", &temp[i]);
        printf("%c", temp[i]);
    }*/
    /*long eol;
    long len;
    curr = ftell(fp);
    char c;
    c = '0';
    while(c != '\n'){ // Read in raw string of characters corresponding to one line of the file
        c = fgetc(fp);
        printf("%c", c);
    }
    fscanf(fp, "\n"); 
    */
    //int *curr_inputs = malloc(sizeof(int)*)
    

    /*eol = ftell(fp);
    len = eol - curr;
    fseek(fp, curr, SEEK_SET);
    char line[len];
    fgets(line, len, fp);
    for (int i = 0; i < len; i++){
        printf("%c", line[i]);
    }*/

    /*for (int x = 0; x < n; x++){ // One iteration per class of v_i
        for(int y = 0; y < m; y++){ // One iteration per specific v_i

        }
    }*/

    /*long curr = ftell(fp);
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    printf("The data after the file header is %d characters long.\n", sz);
    fseek(fp, curr, SEEK_SET);*/


    /*printf("n: %d\n", n);
    printf("m: %d\n", m);
    for (int i = 0; i < n; i++){
        printf("c_%d: %d\n", i, lengths[i]);
    }
    */
    printf("\n");
    return 0;
}