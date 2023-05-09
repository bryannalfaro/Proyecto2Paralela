//bruteforceNaive.c
//Tambien cifra un texto cualquiera con un key arbitrario.
//OJO: asegurarse que la palabra a buscar sea lo suficientemente grande
//  evitando falsas soluciones ya que sera muy improbable que tal palabra suceda de
//  forma pseudoaleatoria en el descifrado.
//>> mpicc bruteforce.c -o desBrute
//>> mpirun -np <N> desBrute

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

char* read_file(const char* filename) {
    FILE* fp;
    char* buffer;
    long file_size;

    // Open file for reading in binary mode
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Get file size
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    // Allocate memory for buffer
    buffer = (char*) malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(fp);
        perror("Error allocating memory");
        return NULL;
    }

    // Read file contents into buffer
    if (fread(buffer, file_size, 1, fp) != 1) {
        fclose(fp);
        free(buffer);
        perror("Error reading file");
        return NULL;
    }

    // Null-terminate the buffer
    buffer[file_size] = '\0';

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
    DES_cblock des_key;
    memcpy(des_key, &k, sizeof(k));
    DES_set_odd_parity(&des_key);

    // Initialize key schedule
    DES_key_schedule key_schedule;
    DES_set_key_unchecked(&des_key, &key_schedule);

    // Decrypt the message using ECB mode
    for (size_t i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &key_schedule, DES_DECRYPT);
    }

    // Check padding
    size_t pad_len = ciph[len - 1];
    if (pad_len > 8) {
        // Error: Invalid padding
        return;
    }
    for (size_t i = len - pad_len; i < len; i++) {
        if (ciph[i] != pad_len) {
            // Error: Invalid padding
            return;
        }
    }

    // Null-terminate the plaintext
    ciph[len - pad_len] = '\0';
    // printf("DECRYPTTTT A ABA JKAKAJH KJAH , %s\n", ciph);
}

//cifra un texto dado una llave
void encrypt_h(long key, char *ciph, int len) {
// Set parity of key
    long k = 0;
    for(int i=0; i<8; ++i){
        key <<= 1;
        k += (key & (0xFE << i*8));
    }

    // Initialize DES key
    DES_cblock des_key;
    memcpy(des_key, &k, sizeof(k));
    DES_set_odd_parity(&des_key);

    // Initialize key schedule
    DES_key_schedule key_schedule;
    DES_set_key_unchecked(&des_key, &key_schedule);

    // Calculate padding length
    size_t pad_len = 8 - len % 8;
    if (pad_len == 0) {
        pad_len = 8;
    }

    // Add padding to message
    for (size_t i = len; i < len + pad_len; i++) {
        ciph[i] = pad_len;
    }

    // Encrypt the message using ECB mode
    for (size_t i = 0; i < len + pad_len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &key_schedule, DES_ENCRYPT);
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

long the_key = 3L;
//2^56 / 4 es exactamente 18014398509481983
//long the_key = 18014398509481983L;
//long the_key = 18014398509481983L +1L;

int main(int argc, char *argv[]){ //char **argv
  //printf("Plain text");
  if (argc < 2) {
    return 1;
  }
  the_key = strtol(argv[1], NULL, 10);


  int N, id;
  long upper = (1L <<56); //upper bound DES keys 2^56
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;
  //printf("Plain text");
  //printf("Cipher text: %s\n", eltexto);
  MPI_Comm comm = MPI_COMM_WORLD;
  double start, end;

  char* text = read_file("text.txt");
  if (text == NULL) {
      printf("Error melon \n");
      return 0;
  }
  int ciphlen = strlen(text);
  //cifrar el texto
  char cipher[ciphlen+1];
  memcpy(cipher, text, ciphlen);
  cipher[ciphlen]=0;
  //printf("Plain text");
  encrypt_h(the_key, cipher, ciphlen);
  printf("Cipher text: %s\n", cipher);
  //INIT MPI
  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);
  // printf("Process %d of %d\n", id, N);
  long found = 0L;
  int ready = 0;

  //distribuir trabajo de forma naive
  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) -1;
  if(id == N-1){
    //compensar residuo
    myupper = upper;
  }
  printf("Process %d lower %ld upper %ld\n", id, mylower, myupper);

  //non blocking receive, revisar en el for si alguien ya encontro
  start = MPI_Wtime();
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  for(long i = mylower; i<myupper; ++i){
    MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
    if(ready)
      break;  //ya encontraron, salir

    if(tryKey(i, cipher, ciphlen)){
      found = i;
      printf("Process %d found the key\n", id);
      end = MPI_Wtime();
      for(int node=0; node<N; node++){
        MPI_Send(&found, 1, MPI_LONG, node, 0, comm); //avisar a otros
      }
      break;
    }

  }
  

  //wait y luego imprimir el texto
  if(id==0){
    MPI_Wait(&req, &st);
    decrypt(found, cipher, ciphlen);
    printf("Key = %li\n\n", found);
    cipher[ciphlen+1]='\0';
    printf("%s\n", cipher);
    printf("Time to break the DES %f\n",end-start);
  }
  printf("Process %d exiting\n", id);

  //FIN entorno MPI
  MPI_Finalize();
}
