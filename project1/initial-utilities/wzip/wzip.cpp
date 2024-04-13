#include <iostream>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc == 1) {
        cout << "wzip: file1 [file2 ...]" << endl;
        return 1;
    }

    char k;
    uint32_t c = 0;
    bool start = true;

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            cout << "wzip: cannot open file" << endl;
            return 1;
        }

        char buffer[4096];
        int ret;

        // iterate through every char in buffer
        // have current char k
        // keep count c of how many many in a row
        // when new char != k, write the c, then the char k
        while ((ret = read(fd, buffer, 4096)) > 0) {
            if (start) {
                k = buffer[0];  
                start = false;
            }    
            for (int j = 0; j < ret; j++) {
                if (buffer[j] != k) {
                    write(STDOUT_FILENO, &c, 4);
                    write(STDOUT_FILENO, &k, 1);
                    k = buffer[j]; 
                    c = 1;
                } else {
                    c++;
                }
            }
        }   

        close(fd);
    }
    write(STDOUT_FILENO, &c, 4);
    write(STDOUT_FILENO, &k, 1);

    return 0;
}