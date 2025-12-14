g++ -fsanitize=address -g \
  -Wall -Wextra -Wpedantic -Wold-style-cast -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor \
  -std=c++23 \
  -Iinclude \
  src/*.cc \
  `pkg-config fuse3 --cflags --libs` -lcurl \
  -o iofs-ng
