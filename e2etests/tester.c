#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

int test_empty_directory(const char *mount_dir) {
    printf("Running Test 1: Empty directory check...\n");
    DIR *dir = opendir(mount_dir);
    if (!dir) {
        perror("opendir failed");
        return -1;
    }

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            count++;
        }
    }
    closedir(dir);

    if (count != 0) {
        fprintf(stderr, "Directory not empty, found %d items\n", count);
        return -1;
    }
    return 0;
}

int test_file_lifecycle(const char *mount_dir) {
    printf("Running Test 2: File lifecycle and stat...\n");
    char path[1024];
    snprintf(path, sizeof(path), "%s/test2.txt", mount_dir);
    
    int fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("open with O_CREAT failed");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat failed");
        close(fd);
        return -1;
    }
    close(fd);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open without O_CREAT failed");
        return -1;
    }
    close(fd);

    if (unlink(path) != 0) {
        perror("unlink failed");
        return -1;
    }

    if (stat(path, &st) == 0 || errno != ENOENT) {
        fprintf(stderr, "stat should have failed with ENOENT\n");
        return -1;
    }

    return 0;
}

int test_multiple_files(const char *mount_dir) {
    printf("Running Test 3: Multiple files and readdir...\n");
    char path[1024];
    
    for (int i = 0; i < 10; i++) {
        snprintf(path, sizeof(path), "%s/file%d.txt", mount_dir, i);
        int fd = open(path, O_CREAT | O_WRONLY, 0644);
        if (fd < 0) {
            perror("create multiple files failed");
            return -1;
        }
        close(fd);
    }

    DIR *dir = opendir(mount_dir);
    if (!dir) {
        perror("opendir failed");
        return -1;
    }

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            count++;
        }
    }
    closedir(dir);

    if (count != 10) {
        fprintf(stderr, "readdir should see exactly 10 files, found %d\n", count);
        return -1;
    }

    for (int i = 0; i < 10; i++) {
        snprintf(path, sizeof(path), "%s/file%d.txt", mount_dir, i);
        if (unlink(path) != 0) {
            perror("unlink multiple files failed");
            return -1;
        }
    }

    return 0;
}

int test_subdirectories(const char *mount_dir) {
    printf("Running Test 4: Subdirectories...\n");
    char dirpath[1024];
    snprintf(dirpath, sizeof(dirpath), "%s/testdir", mount_dir);
    
    if (mkdir(dirpath, 0755) != 0) {
        perror("mkdir failed");
        return -1;
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/testdir/file.txt", mount_dir);
    int fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("create file in testdir failed");
        return -1;
    }
    close(fd);

    if (unlink(path) != 0) {
        perror("unlink file in testdir failed");
        return -1;
    }
    
    if (rmdir(dirpath) != 0) {
        perror("rmdir failed");
        return -1;
    }

    return 0;
}

int test_read_write_truncate(const char *mount_dir) {
    printf("Running Test 5: Read, write, and truncation...\n");
    char path[1024];
    snprintf(path, sizeof(path), "%s/test5.txt", mount_dir);
    
    // Create & stat
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("create test5.txt failed");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat test5.txt failed");
        close(fd);
        return -1;
    }

    if (st.st_size != 0) {
        fprintf(stderr, "file size should be 0, got %lld\n", (long long)st.st_size);
        close(fd);
        return -1;
    }
    
    // Write & stat
    if (write(fd, "foobar", 6) != 6) {
        perror("write to test5.txt failed");
        close(fd);
        return -1;
    }

    if (fstat(fd, &st) != 0) {
        perror("fstat test5.txt after write failed");
        close(fd);
        return -1;
    }

    if (st.st_size != 6) {
        fprintf(stderr, "file size should be 6 after write, got %lld\n", (long long)st.st_size);
        close(fd);
        return -1;
    }
    close(fd);

    // Reopen & read
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("reopen test5.txt for reading failed");
        return -1;
    }

    char buf[16] = {0};
    if (read(fd, buf, sizeof(buf)) != 6) {
        perror("read from test5.txt failed");
        close(fd);
        return -1;
    }

    if (strcmp(buf, "foobar") != 0) {
        fprintf(stderr, "read content mismatch. Expected 'foobar', got '%s'\n", buf);
        close(fd);
        return -1;
    }
    close(fd);

    // Reopen with O_TRUNC
    fd = open(path, O_RDWR | O_TRUNC);
    if (fd < 0) {
        perror("reopen test5.txt with O_TRUNC failed");
        return -1;
    }

    if (fstat(fd, &st) != 0) {
        perror("fstat test5.txt after O_TRUNC failed");
        close(fd);
        return -1;
    }

    if (st.st_size != 0) {
        fprintf(stderr, "file size should be 0 after truncate, got %lld\n", (long long)st.st_size);
        close(fd);
        return -1;
    }
    
    // Read should be empty
    int bytes_read = read(fd, buf, sizeof(buf));
    if (bytes_read != 0) {
        fprintf(stderr, "read after truncate should return 0 bytes, got %d\n", bytes_read);
        close(fd);
        return -1;
    }
    close(fd);

    if (unlink(path) != 0) {
        perror("cleanup test5.txt failed");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <fake_mount_path> <real_backend_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *mount_dir = argv[1];
    printf("Starting C-level E2E tests on mount: %s\n\n", mount_dir);

    if (test_empty_directory(mount_dir) != 0) return EXIT_FAILURE;
    if (test_file_lifecycle(mount_dir) != 0) return EXIT_FAILURE;
    if (test_multiple_files(mount_dir) != 0) return EXIT_FAILURE;
    if (test_subdirectories(mount_dir) != 0) return EXIT_FAILURE;
    if (test_read_write_truncate(mount_dir) != 0) return EXIT_FAILURE;

    printf("\nAll C E2E tests passed successfully!\n");
    return EXIT_SUCCESS;
}
