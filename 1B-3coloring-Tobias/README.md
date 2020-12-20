# 1B-3coloring-Tobias

## Rating
**Points received:** 14/15

Two Mistakes (Edge-Cases):

1. If the supervisor terminates while the buffer is full, a Deadlock *could* occur in a generator.
2. The loop in the generator was not implemented as it normally is and I would have had to post the exclWriteSemaphore when sem_wait gets interrupted by a signal to avoid possible deadlocks.
