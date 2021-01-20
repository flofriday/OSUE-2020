# 3-http-Tobias

## Rating
**Points received:** 22/20

Only first bonus Task implemented.

## About this solution (optional)
The Tutor had nothing to complain about but there are actually quite some things that I would have done differently if I had more time.
First of all a lot of Code was copy-pasted from client to server to save some time. Ideally I would have introduced a common module like in my first Excersie.
Another thing worth mentioning is, that the whole requests and responses are send in one go. Likewise all files are written and read in one go.
It would be better to implement bonus Task two (handling binary files) and handle the respective data not all at once.
For example the server could already send parts of a requested file to the client while still reading the file. 