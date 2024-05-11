#include <iostream>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc == 1) {
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