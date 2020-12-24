# http-testsuite
An attempt to write an integration testsuite for the third exercise.

## 🚨 Work in progress⚠️
At the moment there are a couple of client tests, however some edge-cases are
still missing, like if the server behaves completly wrong.
Server tests are almost non-existing.

## How to use
copy the files in this folder to your working directory and run: 
```
make all
python3 clienttest.py
python3 servertest.py
```