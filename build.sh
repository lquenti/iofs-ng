# TODO add -Wall -Wextra -Wpedantic -Wold-style-cast -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor \
g++ -fsanitize=address -g \
  -Wall \
  -std=c++23 \
  src/*.cc \
  `pkg-config fuse3 --cflags --libs` -lcurl \
  -o iofs
