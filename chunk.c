/**
 * @file bt1.cpp
 * @author Raul Jimenez
 * @author Bryann Alfaro
 * @author Donaldo Garcia
 * @brief implementation using chunks
 * @version 0.1
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

/**
 * @brief Read a file into a buffer
 * @param filename
 * @return char*
*/
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

/**
 * @brief decrypt a text given a key
 * @param key key to decrypt
 * @param ciph text to decrypt
 * @param len length of the text
 *
 * @return void
*/
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
}

/**
 * @brief encrypt a text given a key
 * @param key key to encrypt
 * @param ciph text to encrypt
 * @param len length of the text
 *
 * @return void
*/
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
}

// key word to search in decrypted text to determine if the code was broken
char search[] = "es una prueba de";

/**
 * @brief try a key on a text
 * @param key key to try
 * @param ciph text to try
 * @param len length of the text
 *
 * @return 1 if the key was found, 0 otherwise
*/
int tryKey(long key, char *ciph, int len){
  char temp[len+1]; //+1 because of the terminal character
  strcpy(temp, ciph);
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

long theKey = 3L;

int main(int argc, char *argv[]){ //char **argv
  if (argc < 2) {
    return 1;
  }
  theKey = strtol(argv[1], NULL, 10);


  int processSize, id;
  long upper = (1L <<56); //upper bound DES keys 2^56
  long myLower, myUpper;
  MPI_Status st;
  MPI_Request req;
  long chunk_start, chunk_end, chunk_size; //Definitions of chunks
  long start_c = 0;
  long end_c = upper;

  MPI_Comm comm = MPI_COMM_WORLD;
  double start, end;

  char* text = readFile("text.txt");
  if (text == NULL) {
      printf("Error reading \n");
      return 0;
  }
  int ciphLen = strlen(text);

  //encypt the text
  char cipher[ciphLen+1];
  memcpy(cipher, text, ciphLen);
  cipher[ciphLen]=0;

  encryptText(theKey, cipher, ciphLen);
  printf("Cipher text: %s\n", cipher);
  //INIT MPI
  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &processSize);
  MPI_Comm_rank(comm, &id);
  chunk_size = (end_c - start_c + 1) / processSize; //CHUNK SIZE

  long found = 0L;
  long keyFound = 0L;
  start = MPI_Wtime();
  while (!found) { // Determine the start and end of the current chunk
    chunk_start = start_c + id * chunk_size;
    chunk_end = chunk_start + chunk_size - 1; // Check if the current chunk is within the range of keys
    printf("CHUNKS %ld %ld",chunk_start,chunk_end);
    if (chunk_start <= end_c) {

      if (chunk_end > end_c) {
          chunk_end = end_c;
        }
      // Evaluate the keys in the current chunk
      for (int key = chunk_start; key <= chunk_end; key++)
      {
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
    if (finished == processSize) {
      break;
      }
    }

    //wait
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
