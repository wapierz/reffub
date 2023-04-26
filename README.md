# reffub
Modern c++20/c++23 implementation of gap buffer data structure under MIT licence.

# Project description
This project contains header only implementation of gap buffer.  It uses modern approach provided by c++20 and c++23 e.g. gap is naturally represented as std::ranges::subrange, content as a std::ranges::view etc.
Thanks to the ranges/concepts library code is clean and easily extensible.

# Install/run
As was mentioned above it is header only so all you need to do is include gap_buffer.hpp into your project and that's all! However, note that some modern compiler is required (e.g. gcc13) and standard c++23 should be used.
In order to run example from main.cpp just run `./make_debug.sh && ./run_debug.sh`.

# Remarks
Some testing needs to be done in the future (consider this version as a beta one). Any improvements, suggestions or advice are always appreciated :)
