#include <iostream>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <string>
#include <string.h>

using namespace std;

void wgrep(int fd, char *key);

int main(int argc, char *argv[]) {
    if (argc == 1) {
        cout << "wgrep: searchterm [file ...]" << endl;
        return 1;
    } else if (argc == 2) {
        wgrep(STDIN_FILENO, argv[1]);
        return 0;
    }

    for (int i = 2; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            cout << "wgrep: cannot open file" << endl;
            return 1;
        }

        wgrep(fd, argv[1]);      
    }
    return 0;
}

// read file and look at buffer
// iterate through buffer 
// keep a flag for if you found the string in question
// look for when the character matches the newline character, store that in linelen
// while doing this you have a writeout buffer that should end with a newline
// write writeout buffer up to linelen
// 
// repeat, writing over writeout buffer, continuing from where buffer left off
// 
// when there is no newline but still stuff to read from buffer:
// write this stuff into writeout buffer, 
// get a new buffer, continue, keeping track of how many bytes read
void wgrep(int fd, char *key) {
    char buffer[4096];
    char writeOut[4096];
    int k = 0;
    int ret;
    string findStr(key);

    while ((ret = read(fd, buffer, 4096)) > 0) {
        bool newLine = false;
        for (int j = 0; j < ret; j++) {
            writeOut[k] = buffer[j];
            k++;
            if (buffer[j] == '\n') {
                string line(writeOut);
                if (line.find(findStr) != string::npos) {
                    write(1, writeOut, k);
                }
                memset(writeOut, '\0', k);
                k = 0;
                newLine = true;
            }
        }
        if (!newLine) {
            string line(writeOut);
            if (line.find(findStr) != string::npos) {
                write(1, writeOut, k);
            }
            memset(writeOut, '\0', k);
            k = 0;
        }
    }
    if (fd != STDIN_FILENO) {
        close(fd);
    }
}