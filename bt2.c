#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

char* readFile(const char* filename) {
    FILE* fp;
    char* buffer;
    long fileSize;

    // Open file for reading in binary mode
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Get file size
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);

    // Allocate memory for buffer
    buffer = (char*) malloc(fileSize + 1);
    if (buffer == NULL) {
        fclose(fp);
        perror("Error allocating memory");
        return NULL;
    }

    // Read file contents into buffer
    if (fread(buffer, fileSize, 1, fp) != 1) {
        fclose(fp);
        free(buffer);
        perror("Error reading file");
        return NULL;
    }

    // Null-terminate the buffer
    buffer[fileSize] = '\0';

    // Close file and return buffer
    fclose(fp);
    return buffer;
}

//descifra un texto dado una llave
void decrypt(long key, char *ciph, int len) {
    // Set parity of key
    long k = 0;
    for (int i = 0; i < 8; ++i) {
        key <<= 1;
        k += (key & (0xFE << i * 8));
    }

    // Initialize DES key
    DES_cblock desKey;
    memcpy(desKey, &k, sizeof(k));
    DES_set_odd_parity(&desKey);

    // Initialize key schedule
    DES_key_schedule keySchedule;
    DES_set_key_unchecked(&desKey, &keySchedule);

    // Decrypt the message using ECB mode
    for (size_t i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &keySchedule, DES_DECRYPT);
    }

    // Check padding
    size_t padLen = ciph[len - 1];
    if (padLen > 8) {
        // Error: Invalid padding
        return;
    }
    for (size_t i = len - padLen; i < len; i++) {
        if (ciph[i] != padLen) {
            // Error: Invalid padding
            return;
        }
    }

    // Null-terminate the plaintext
    ciph[len - padLen] = '\0';
    // printf("DECRYPTTTT A ABA JKAKAJH KJAH , %s\n", ciph);
}

//cifra un texto dado una llave
void encryptText(long key, char *ciph, int len) {
// Set parity of key
    long k = 0;
    for(int i=0; i<8; ++i){
        key <<= 1;
        k += (key & (0xFE << i*8));
    }

    // Initialize DES key
    DES_cblock desKey;
    memcpy(desKey, &k, sizeof(k));
    DES_set_odd_parity(&desKey);

    // Initialize key schedule
    DES_key_schedule keySchedule;
    DES_set_key_unchecked(&desKey, &keySchedule);

    // Calculate padding length
    size_t padLen = 8 - len % 8;
    if (padLen == 0) {
        padLen = 8;
    }

    // Add padding to message
    for (size_t i = len; i < len + padLen; i++) {
        ciph[i] = padLen;
    }

    // Encrypt the message using ECB mode
    for (size_t i = 0; i < len + padLen; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &keySchedule, DES_ENCRYPT);
    }
  // strcpy(ciph, ciphertext);
  // printf("AAAAAAAA, %s\n", ciph);
}

//palabra clave a buscar en texto descifrado para determinar si se rompio el codigo
char search[] = "es una prueba de";
int tryKey(long key, char *ciph, int len){
  char temp[len+1]; //+1 por el caracter terminal
  strcpy(temp, ciph);
  // memcpy(temp, ciph, len);
  // temp[len]=0;	//caracter terminal
  decrypt(key, temp, len);
  // printf("IMPRIMIR %s\n", temp);
  return strstr((char *)temp, search) != NULL;
}

long theKey = 3L;
//2^56 / 4 es exactamente 18014398509481983
//long theKey = 18014398509481983L;
//long theKey = 18014398509481983L +1L;

int main(int argc, char *argv[]){ //char **argv
  //printf("Plain text");
  if (argc < 2) {
    return 1;
  }
  theKey = strtol(argv[1], NULL, 10);


  int N, id;
  long upper = (1L <<56); //upper bound DES keys 2^56
  long myLower, myUpper;
  MPI_Status st;
  MPI_Request req;
  long chunk_start, chunk_end, chunk_size; //DEFINICIONES
  long start_c = 0; 
  long end_c = upper;

  //printf("Plain text");
  //printf("Cipher text: %s\n", eltexto);
  MPI_Comm comm = MPI_COMM_WORLD;
  double start, end;
  
  char* text = readFile("text.txt");
  if (text == NULL) {
      printf("Error melon \n");
      return 0;
  }
  int ciphLen = strlen(text);
  //cifrar el texto
  char cipher[ciphLen+1];
  memcpy(cipher, text, ciphLen);
  cipher[ciphLen]=0;
  //printf("Plain text");
  encryptText(theKey, cipher, ciphLen);
  printf("Cipher text: %s\n", cipher);
  //INIT MPI
  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);
  chunk_size = (end_c - start_c + 1) / N; //CHUNK SIZE
  
  // printf("Process %d of %d\n", id, N);
  long found = 0L;
  long keyFound = 0L;
  start = MPI_Wtime();
  while (!found) { // Determine the start and end of the current chunk 
  //printf("CHunk size %d",end_c);
    //printf("working hard");
    chunk_start = start_c + id * chunk_size; 
    chunk_end = chunk_start + chunk_size - 1; // Check if the current chunk is within the range of keys 
    printf("CHUNKS %ld %ld",chunk_start,chunk_end);
    if (chunk_start <= end_c) { 
      
      if (chunk_end > end_c) { 
          chunk_end = end_c; 
        } 
      //printf("chunk start %d chunk end %d",chunk_start,chunk_end);
      // Evaluate the keys in the current chunk 
      for (int key = chunk_start; key <= chunk_end; key++) 
      { 
        //printf("key %d",key);
        if (tryKey(key,cipher, ciphLen)) { 
          found = 1;
          keyFound = key;
          if (id == 0) { 
            printf("Found key: %d\n", key); 
            end = MPI_Wtime();
            } 
            break; 
            } 
        } 
    } 
    // Synchronize the processes 
    MPI_Barrier(MPI_COMM_WORLD); 
    // Determine the number of processes that have finished their chunk 
    int finished = 0; 
    MPI_Allreduce(&found, &finished, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
    // If all the processes have finished, break out of the loop 
    if (finished == N) { 
      break; 
      } 
    } 

    //wait y luego imprimir el texto
  if(id==0){
    //MPI_Wait(&req, &st);
    decrypt(keyFound, cipher, ciphLen);
    printf("Key = %li\n\n", keyFound);
    cipher[ciphLen+1]='\0';
    printf("%s\n", cipher);
    printf("Time to break the DES %f\n",end-start);
  }
  //printf("Process %d exiting\n", id);
    MPI_Finalize(); 
    } 
