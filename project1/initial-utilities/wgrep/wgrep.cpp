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

void wgrep(int fd, char *key) {
    char buffer[4096];
    int ret;
    string findStr(key);
    string leftover = "";

    while ((ret = read(fd, buffer, 4096)) > 0) {
        long unsigned int nl_id;
        string buffstr(buffer);
        buffstr = leftover.append(buffstr);
        while ((nl_id = buffstr.find_first_of('\n')) != string::npos) {
            string line = buffstr.substr(0, nl_id + 1);
            if (line.find(findStr) != string::npos) {
                write(STDOUT_FILENO, line.c_str(), line.size());
            }
            buffstr = buffstr.substr(nl_id + 1);
        }
        leftover = buffstr;        
    }

    if (fd != STDIN_FILENO) {
        close(fd);
    }
}