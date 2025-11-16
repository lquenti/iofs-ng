gcc -Wall src/*.c `pkg-config fuse3 --cflags --libs` -lcurl -o iofs
