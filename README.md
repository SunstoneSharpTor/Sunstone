# Sunstone
Sunstone is an alpha-beta chess engine written in C++ that uses the
UCI protocol. It's performance is currently much better than an
above-average human's, but much worse than top chess engines.

## Building

The game is designed to be built using CMake and GCC. There are no
external dependencies, so it should be pretty easy to do.

Download the repo, then navigate to the top-level folder in a command
line/terminal and run the following commands to build the project:

```sh
cmake .
make
```