#include <iostream>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // uint32_t c = 38;
        // char k = 'a';
        // write(STDOUT_FILENO, &c, 4);
        // write(STDOUT_FILENO, &k, 1);
        cout << "wunzip: file1 [file2 ...]" << endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            cout << "wunzip: cannot open file" << endl;
            return 1;
        }

        char k;
        uint32_t c = 0;

        // ok you might need to read a little differently here
        // idk what the while condition is yet but
        // repeatedly:
        // read 4 bytes into a uint32 pointer c
        // read 1 byte into a char pointer k
        // then forloop write 1 char k c times
        while ((read(fd, &c, 4)) > 0) {  
            read(fd, &k, 1);
            for (u_int32_t j = 0; j < c; j++) {
                write(STDOUT_FILENO, &k, 1);
            }       
        }   

        close(fd);
    }

    return 0;
}