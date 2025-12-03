gcc -fsanitize=address -g -Wall src/*.c `pkg-config fuse3 --cflags --libs` -lcurl -o iofs
