#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>

using namespace std;
namespace fs = std::filesystem;

union number {
    int n;
    char m[sizeof(int)];
};

int main() {
    srand(time(NULL));
    cout << ::getpid() << endl;

    int filedesc = open("numbers.txt", O_RDWR | O_CREAT | O_APPEND, 0777);

    number num;
    int k = 0;
    int bytes_written;
    int offset;

#ifdef W
    for (auto i : views::iota(1, 1024*1024 + 1)) {
        num.n = rand();
        k = sizeof(num.m);
        offset = bytes_written = 0;
        while (k > 0) {
            bytes_written = write(filedesc, &(num.m) + offset, k);
            if (bytes_written < 0) {
                cout << "write fails with " << strerror(errno) << endl;
                break;
            }
            k = k - bytes_written;
            offset += bytes_written;
        }
        cout << "write : " << num.n << endl;
    }

    syncfs(filedesc);

    lseek(filedesc, 0, SEEK_SET);
#endif

#ifdef R
    fs::path example = "numbers.txt";
    fs::path p = fs::current_path() / example;
    auto sz = fs::file_size(p);
    cout << "size of file is = " << sz << " bytes" << endl;

    int bytes_read;
    while (sz > 0) {
        k = sizeof(num.m);

        offset = bytes_read = 0;

        while (k > 0) {
            bytes_read = read(filedesc, &(num.m) + offset, k);
            if (bytes_read < 0) {
                cout << "read fails with " << strerror(errno) << endl;
                break;
            }
            k = k - bytes_read;
            offset += bytes_read;
        }
        sz = sz - sizeof(num.m);
        cout << "read : " << num.n << endl;
    }
#endif
    close(filedesc);
}
