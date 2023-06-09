# 🌐 Proyecto paralela - Brute Force

This project tries to find the key to decrypt a text from a file using brute force. The algorithm to make the encryption en decryption process is DES.

## 🛠 Installation requirements

1. The program needs the computer to have installed successfully the `openssl` library for C.

## 🚀 Running the proyect

1. To run the proyect we just need to run the command

```shell
  mpicc <file_name>.cpp -o <executable_file_name> -lssl  -lcrypto
  mpirun -np <number_of_processes> <key>
``○`
```

In which `file_name` is the name of the file user wants to compile, It can be `naive` or `first_mpi` or `second_mpi`.
And `executable_file_name` is the name user want to save the executable file.

## Contributors

| Raul Angel J. | Donaldo Garcia | Bryann Alfaro |
| :---: |:---:| :---:|
| [![Raul Angel](https://avatars0.githubusercontent.com/u/46568595?s=200&u=c1481289dc10f8babb1bdd0853e0bcf82a213d26&v=4)](https://github.com/raulangelj)    | [![Donaldo Garcia](https://avatars1.githubusercontent.com/u/54748964?s=200&u=5e617360d13f87fa6d62022e81bab94ebf50c4e3&v=4)](https://github.com/donaldosebas) | [![Bryann Alfaro](https://avatars0.githubusercontent.com/u/46506166?s=200&u=c1481289dc10f8babb1bdd0853e0bcf82a213d26&v=4)](https://github.com/bryannalfaro) |
| <a href="https://github.com/raulangelj" target="_blank">`@raulangelj`</a> | <a href="https://github.com/donaldosebas" target="_blank">`@donaldosebas`</a> | <a href="https://github.com/bryannalfaro" target="_blank">`@bryannalfaro`</a> |