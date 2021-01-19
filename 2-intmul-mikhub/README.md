# 2-intmul-mikhub

## Rating
**Points received:** 25/20

## About this solution + bonus

The management of child processes and pipes is elegantly done (at least in my opinion) in my solution using arrays of structs. The adding of intermediate results is not done by any hex-shifting as in other solutions, but by considering only the relevant digits in a for-loop (see function `read_from_children_and_print_combined_result`). It may be hard to understand at first, but it works perfectly and I think it is an interesting alternative approach that I have not seen in other solutions this way.

By calling `./intmul -t`, in addition to the result, the calling tree is printed out (Bonus Task).
