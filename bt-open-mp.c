/**
 * @file bt1.cpp
 * @author Raul Jimenez
 * @author Bryann Alfaro
 * @author Donaldo Garcia
 * @brief First extra implementation of brute force improving performance
 * @version 0.1
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 */
//Reference: Barlas Capitulo 5
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <omp.h>

const int BLOCK = 1000000;

/**
 * @brief Read a file into a buffer
 * @param filename
 * @return char*
*/
char *readFile(const char *filename)
{
  FILE *fp;
  char *buffer;
  long fileSize;

  // Open file for reading in binary mode
  fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    perror("Error opening file");
    return NULL;
  }

  // Get file size
  fseek(fp, 0L, SEEK_END);
  fileSize = ftell(fp);
  rewind(fp);

  // Allocate memory for buffer
  buffer = (char *)malloc(fileSize + 1);
  if (buffer == NULL)
  {
    fclose(fp);
    perror("Error allocating memory");
    return NULL;
  }

  // Read file contents into buffer
  if (fread(buffer, fileSize, 1, fp) != 1)
  {
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
void decrypt(long key, char *ciph, int len)
{
  // Set parity of key
  long k = 0;
  for (int i = 0; i < 8; ++i)
  {
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
  for (size_t i = 0; i < len; i += 8)
  {
    DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &keySchedule, DES_DECRYPT);
  }

  // Check padding
  size_t padLen = ciph[len - 1];
  if (padLen > 8)
  {
    // Error: Invalid padding
    return;
  }
  for (size_t i = len - padLen; i < len; i++)
  {
    if (ciph[i] != padLen)
    {
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
void encryptText(long key, char *ciph, int len)
{
  // Set parity of key
  long k = 0;
  for (int i = 0; i < 8; ++i)
  {
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

  // Calculate padding length
  size_t padLen = 8 - len % 8;
  if (padLen == 0)
  {
    padLen = 8;
  }

  // Add padding to message
  for (size_t i = len; i < len + padLen; i++)
  {
    ciph[i] = padLen;
  }

  // Encrypt the message using ECB mode
  for (size_t i = 0; i < len + padLen; i += 8)
  {
    DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &keySchedule, DES_ENCRYPT);
  }
}

// key word to search in decrypted text to determine if the code was broken
char search[] = "es una prueba de";

/**
 * @brief try a key to decrypt a text
 * @param key key to try
 * @param ciph text to decrypt
 * @param len length of the text
 *
 * @return int 1 if the key is correct, 0 otherwise
*/
int tryKey(long key, char *ciph, int len)
{
  char temp[len + 1]; //+1 because of the terminal character
  strcpy(temp, ciph);
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

long theKey = 3L;

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    return 1;
  }
  theKey = strtol(argv[1], NULL, 10); //getting the key from the command line

  int processSize, id;
  long upper = (1L << 56);
  long found = 0;
  MPI_Status st;
  MPI_Request req;

  MPI_Comm comm = MPI_COMM_WORLD;
  double start, end;

  char* text = readFile("text.txt");
  if (text == NULL) {
      printf("Error reading \n");
      return 0;
  }

  int flag;
  int ciphLen = strlen((char *) text);

  //encrypt the text
  char cipher[ciphLen+1];
  memcpy(cipher, text, ciphLen);
  cipher[ciphLen]=0;

  encryptText(theKey, cipher, ciphLen);
  printf("Cipher text: %s\n", cipher);
  int N,id;
  long found=-1;
  int flag=0;
  MPI_Init(&argc,&argv);
  double start=MPI_Wtime();
  MPI_Comm_rank(MPI_COMM_WORLD,&id);
  MPI_Comm_size(MPI_COMM_WORLD,&processSize);

  MPI_Irecv((void*)&found,1,MPI_LONG,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&req);
  int iterCount=0;
  long idx=0;
  while(idx<upper&&found<0)
  {
    #pragma omp  parallel for  default(none)  shared ( cipher ,  ciphLen ,  found, idx ,  id , N)
    for( long i=idx+id;i<idx+N*BLOCK;i+=N)
    {
      if (tryKey(i,(char*)cipher,ciphLen))
      {
        #pragma omp critical
        found=i;33
      }
    }
    //  communicate  termination  signal
    if (found>= 0)
    {
      for( int node=0;node<N;node++)
        MPI_Send((void*)&found,1,MPI_LONG,node,0,MPI_COMM_WORLD);
    }
        
    idx+=N*BLOCK;
  }
  //  print  out  the  result  from node 0
  if (id== 0)
  {
    MPI_Wait(&req,&st);//  in case process 0 finishes  before the key  is  found
    decrypt(found,(char*)cipher,ciphLen);
    printf( "%i  nodes  in %lf  sec  : %li %s \n",N,MPI_Wtime()-start,found,cipher);
  }
  MPI_Finalize();

  // //MPI
  // MPI_Init(&argc, &argv);
  // MPI_Comm_rank(MPI_COMM_WORLD, &id);
  // MPI_Comm_size(MPI_COMM_WORLD, &processSize);

  // start = MPI_Wtime(); //start time
  // MPI_Irecv((void*)&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &req);
  // int iterCount = 0;

  // //Iterate in step way
  // #pragma omp parallel for num_threads(4) shared(found, processSize, upper, cipher, ciphLen, iterCount, req, st, flag)
  // for( long i=id;i<upper;i+=processSize)
  // {
  //   if (tryKey(i,(char*)cipher,ciphLen))
  //   {
  //     #pragma omp critical
  //     found=i;
  //      end = MPI_Wtime();
  //     for( int node=0;node<processSize;node++)
  //       MPI_Send((void*)&found,1,MPI_LONG,node,0, MPI_COMM_WORLD);

  //     break;
  //   }
  //   if(++iterCount% 1000 == 0) //test every 1000 iterations
  //   {
  //     MPI_Test(&req,&flag,&st);
  //     if(flag)  break;
  //   }
  // }

  // if (id== 0)
  // {
  //   MPI_Wait(&req,&st); //  in case process 0 finishes  before the key  is  found
  //   decrypt(found,(char*)cipher,ciphLen);
  //   cipher[ciphLen+1]='\0';
  //   printf( "%li %s \n",found,cipher);
  //   printf("Time %f",end-start);
  // }

  // //End MPi
  // MPI_Finalize();
}
