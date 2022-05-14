/**
 * @file ext_merge_sort.cpp
 * @author Debashish Deka (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-05-06
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <ranges>

using namespace std;
namespace fs = std::filesystem;

int direct_to_disk = 0; // O_DIRECT | O_SYNC;

// const int chunk = 1024 * 128;  // in terms of integer numbers
// const int chunk = 1024 * 64;  // in terms of integer numbers
// const int chunk = 1024 * 32;  // in terms of integer numbers
// const int chunk = 1024 * 16;  // in terms of integer numbers
// const int chunk = 1024 * 8;  // in terms of integer numbers
const int chunk = 1024 * 4;  // in terms of integer numbers
// const int chunk = 1024 * 2;  // in terms of integer numbers

/**
 * @brief
 *
 */
union number {
    int n;
    char d[sizeof(int)];
};

number x[chunk];
number y[chunk];
number z[chunk];

/**
 * @brief
 *
 * @param fd
 * @param buffer
 * @param bytes_to_read
 * @return true
 * @return false
 */
bool read_data(int fd, char* buffer, int bytes_to_read, int offset) {
    int ret = true;
    int buf_offset, bytes_read;
    buf_offset = bytes_read = 0;
    while (bytes_to_read > 0) {
        bytes_read =
            pread(fd, buffer + buf_offset, bytes_to_read, offset + buf_offset);
        if (bytes_read < 0) {
            LOG(ERROR) << "read fails with " << strerror(errno);
            ret = false;
        }
        buf_offset += bytes_read;
        bytes_to_read -= bytes_read;
    }
    return ret;
}

/**
 *
 * @brief
 *
 * @param fd
 * @param buffer
 * @param bytes_to_write
 * @return true
 * @return false
 */
bool write_data(int fd, char* buffer, int bytes_to_write, int offset) {
    int ret = true;
    int buf_offset, bytes_written;
    buf_offset = bytes_written = 0;
    while (bytes_to_write > 0) {
        bytes_written = pwrite(fd, buffer + buf_offset, bytes_to_write,
                               offset + buf_offset);
        if (bytes_written < 0) {
            LOG(ERROR) << "write fails with " << strerror(errno);
            ret = false;
        }
        buf_offset += bytes_written;
        bytes_to_write -= bytes_written;
    }
    return ret;
}

void print_data(int fd, int l, int r) {
    #ifdef P
    CHECK_LT(l, r);
    int total_bytes_to_read = r - l;
    int offset = l;
    int buffer_size = chunk * sizeof(number::n);
    int bytes_read = 0;
    while (total_bytes_to_read > 0) {
        bytes_read = min(total_bytes_to_read, buffer_size);
        CHECK(read_data(fd, x[0].d, bytes_read, offset));
        offset += bytes_read;
        total_bytes_to_read -= bytes_read;
        for (const auto& k : x) {
            LOG(INFO) << k.n;
        }
    }
    #endif
}

/**
 * @brief
 *
 * @param filedesc
 * @param sz
 */
void first_pass(int filedesc, int sz) {
    number num{0};

    int total_numbers = sz / sizeof(number::n);

    int bytes_to_read = 0;
    int bytes_to_write = 0;

    int cnt = 0;
    int offset = 0;
    while (total_numbers > 0) {
        bytes_to_read = chunk * sizeof(num.n);

        offset = cnt * chunk * sizeof(num.n);

        CHECK(read_data(filedesc, x[0].d, bytes_to_read, offset));

        sort(begin(x), end(x),
             [](const number& a, const number& b) { return a.n < b.n; });

        bytes_to_write = chunk * sizeof(num.d);

        CHECK(write_data(filedesc, x[0].d, bytes_to_write, offset));

        total_numbers -= chunk;
        cnt++;
        LOG(INFO) << "cnt = " << cnt << " rw : [" << offset << ","
                  << offset + chunk * sizeof(num.d) << ")";
    }
}

/**
 * @brief
 *
 * @param filedesc
 * @param offset
 * @param sz
 * @param chunkSize
 */
void ext_merge(int in_fd, int out_fd, int left, int right, int buffer_size) {
    CHECK_LE(left, right);
    int sz = right - left;
    // if the sz is already sorted no need to proceed
    if (sz <= buffer_size) return;

    int mid = (left + right) / 2;
    LOG(INFO) << "entry : left = " << left << " right = " << right
              << " mid = " << mid;

    LOG(INFO) << "calling : [" << left << "," << mid << "]";
    ext_merge(in_fd, out_fd, left, mid, buffer_size);  //[left,mid)

    LOG(INFO) << "calling : [" << mid << "," << right << "]";
    ext_merge(in_fd, out_fd, mid, right, buffer_size);  // [mid,right)

    int len_of_left = mid - left;
    int len_of_right = right - mid;

    LOG(INFO) << "len_of_left = " << len_of_left
              << " len_of_right = " << len_of_right;

    int loffset = left;
    int roffset = mid;
    int write_offset = left;

    auto get_next_number =
        [in_fd, out_fd, len_of_left, len_of_right, items_on_left_buffer = 0,
         items_on_right_buffer = 0, loffset, roffset, buffer_size]<int side>(
            int idx) mutable -> pair<bool, decltype(number::n)> {
        constexpr int LEFT = 0;
        constexpr int RIGHT = 1;
        using T = decltype(number::n);
        T ret;
        if constexpr (side == LEFT) {
            int lidx = idx % chunk;
            if (idx < items_on_left_buffer) {
                return {true, x[lidx].n};
            } else {
                // time to read next batch of numbers from the left side
                int bytes_to_read_from_left = min(len_of_left, buffer_size);
                CHECK(
                    read_data(in_fd, x[0].d, bytes_to_read_from_left, loffset));
                loffset += bytes_to_read_from_left;
                items_on_left_buffer += bytes_to_read_from_left / sizeof(T);
                len_of_left -= bytes_to_read_from_left;

                // read is done, now return 1 number from it
                return {true, x[lidx].n};
            }
        } else {
            int ridx = idx % chunk;
            if (idx < items_on_right_buffer) {
                return {true, y[ridx].n};
            } else {
                // time to read next batch of numbers from the right side
                int bytes_to_read_from_right = min(len_of_right, buffer_size);
                CHECK(read_data(in_fd, y[0].d, bytes_to_read_from_right,
                                roffset));
                roffset += bytes_to_read_from_right;
                items_on_right_buffer += bytes_to_read_from_right / sizeof(T);
                len_of_right -= bytes_to_read_from_right;

                // read is done, now return 1 number from it
                return {true, y[ridx].n};
            }
        }
    };

    constexpr int LEFT = 0;
    constexpr int RIGHT = 1;

    int i = 0, j = 0, k = 0;

    function<void(decltype(number::n))> write_to_output_buffer =
        [buffer_size, out_fd, &write_offset,
         &k](decltype(number::n) value) mutable {
            z[k++].n = value;
            if (k == chunk) {
                // time to write to disk
                CHECK(write_data(out_fd, z[0].d, buffer_size, write_offset));
                write_offset += buffer_size;
                k = 0;
            }
        };

    int items_on_left = len_of_left / sizeof(number::n);
    int items_on_right = len_of_right / sizeof(number::n);

    LOG(INFO) << "items_on_left = " << items_on_left
              << " items_on_right = " << items_on_right;

    // actual merge algorithm starts here
    while (i < items_on_left && j < items_on_right) {
        auto lv = get_next_number.operator()<LEFT>(i).second;
        auto rv = get_next_number.operator()<RIGHT>(j).second;
        if (lv < rv) {
            write_to_output_buffer(lv);
            i++;
        } else {
            write_to_output_buffer(rv);
            j++;
        }
    }

    while (i < items_on_left) {
        auto lv = get_next_number.operator()<LEFT>(i).second;
        write_to_output_buffer(lv);
        i++;
    }

    while (j < items_on_right) {
        auto rv = get_next_number.operator()<RIGHT>(j).second;
        write_to_output_buffer(rv);
        j++;
    }

    CHECK_EQ(write_offset, right);

    // copy from out_fd to in_fd
    int total_bytes_to_copy = right - left;
    int offset = left;
    while (total_bytes_to_copy > 0) {
        int bytes_to_rw = min(buffer_size, total_bytes_to_copy);
        CHECK(read_data(out_fd, x[0].d, bytes_to_rw, offset));
        CHECK(write_data(in_fd, x[0].d, bytes_to_rw, offset));
        offset += bytes_to_rw;
        total_bytes_to_copy -= bytes_to_rw;
    }

    CHECK_EQ(offset, right);
    print_data(in_fd, left, right);
    LOG(INFO) << "return : left = " << left << " right = " << right;
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char* argv[]) {
    FLAGS_logtostdout = 1;
    FLAGS_minloglevel = 2;
    google::InitGoogleLogging(argv[0]);

    fs::path example = "numbers.txt";
    fs::path p = fs::current_path() / example;
    auto sz = fs::file_size(p);

    LOG(INFO) << "size of file is = " << sz << " bytes";

    int in_fd = open("numbers.txt", O_RDWR | O_CREAT | direct_to_disk, 0777);
    int out_fd =
        open("sorted_numbers.txt", O_RDWR | O_CREAT | direct_to_disk, 0777);

    first_pass(in_fd, sz);

    print_data(in_fd, 0, sz);

    ext_merge(in_fd, out_fd, 0, sz, chunk * sizeof(number::n));

    close(in_fd);
    close(out_fd);
}
