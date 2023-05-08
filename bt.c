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

    // Remove padding from message
    size_t pad_len = ciph[len - 1];
    for (size_t i = len - pad_len; i < len; i++) {
        if (ciph[i] != pad_len) {
            // Error: Invalid padding
            return;
        }
    }

    // Null-terminate the plaintext
    ciph[len - pad_len] = '\0';
    printf("DECRYPTTTT A ABA JKAKAJH KJAH , %s\n", ciph);
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
  printf("AAAAAAAA, %s\n", ciph);
}

//palabra clave a buscar en texto descifrado para determinar si se rompio el codigo
char search[] = "donaldo";
int tryKey(long key, char *ciph, int len){
  char temp[len+1]; //+1 por el caracter terminal
  strcpy(temp, ciph);
  // memcpy(temp, ciph, len);
  // temp[len]=0;	//caracter terminal
  decrypt(key, temp, len);
  printf("IMPRIMIR %s\n", temp);
  return strstr((char *)temp, search) != NULL;
}

char eltexto[] = "Hola Raul, hola bryann, hola donaldo, ya me quiero ir a dormir";

long the_key = 3L;
//2^56 / 4 es exactamente 18014398509481983
//long the_key = 18014398509481983L;
//long the_key = 18014398509481983L +1L;

int main(int argc, char *argv[]){ //char **argv
  //printf("Plain text");
  int N, id;
  long upper = 400L; //upper bound DES keys 2^56
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;
  //printf("Plain text");
  int ciphlen = strlen(eltexto);
  //printf("Cipher text: %s\n", eltexto);
  MPI_Comm comm = MPI_COMM_WORLD;

  //cifrar el texto
  char cipher[ciphlen+1];
  memcpy(cipher, eltexto, ciphlen);
  cipher[ciphlen]=0;
  //printf("Plain text");
  encrypt_h(the_key, cipher, ciphlen);
  printf("Cipher text: %s\n", cipher);
  //INIT MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);
  printf("Process %d of %d\n", id, N);

  //si es el id 0 desencriptar el texto
  if(id==0){
    //printf("Plain text: %s\n", eltexto);
    //print cipher
    //printf("Cipher text inside: %s\n", cipher);
    // decrypt(the_key, cipher, ciphlen);
    printf("\nAACipher text----------: %s\n", cipher);
    printf("TRYING KEY %d \n", tryKey(the_key, cipher, ciphlen));
    printf("\nAACipher text: %s\n", cipher);
  }

  printf("Process %d exiting\n", id);

  //FIN entorno MPI
  MPI_Finalize();
}
