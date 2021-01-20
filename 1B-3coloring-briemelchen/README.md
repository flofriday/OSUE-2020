# 1b-3coloring-briemelchen

## Rating
**Points received:** 12/15

-3 points because I did not check if sem_wait is interrupted by a signal (like `while(sem_wait == -1) if (errno == EINTR) continue`)