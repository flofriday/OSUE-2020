# 1B-3coloring-flofriday

## Rating
**Points received:** 15/15

Tutor said that C-code should not exceed the 80-character limit.

## About this solution
My solution has two implementations to solve this problem. By default the faster
one is built with `make all`. However, since this is not the algorithm that is
described in the instructions, I also implemented the specified algorithm which
can be built with `make all-slow`. In the C code the implementations are
switched with the `SLOW_ALGO` flag.

Also the argument-parsing in the generator is not the best.