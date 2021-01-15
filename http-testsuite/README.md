# http-testsuite
An attempt to write an integration testsuite for the third exercise.

## ️Disclaimer ⚠️
First, there is no gurantee that passing these test will gurantee that you will
be positive. These tests are created by your fellow students and by our
interpretation of the exercise (which may be wrong). 

Having said that, there are quite a lot of test for the server and the client.

## How to use
copy the files in this folder to your working directory and run: 
```
make all
python3 clienttest.py
python3 servertest.py
```

**Note:** Please feel free to create an issue if you have more tests, ideas for 
more tests or notes on how to make the error messages more clear.

## What is tested
(Empty boxes are for tests that we want to implement, but won't as I don't have 
too much time left)

- [x] Memory corruption / leaks for **Client and Server** 

- [x] **Client** has correct arguments parsing.
- [x] **Client** parses urls correctly
- [x] **Client** prints the correct error if server responds with anything but 200
- [x] **Client** creates the right files in the right places
- [x] **Client** can download files and the content is correct (only binary download can be tested)
- [ ] **Client** sends the correct/required headers
- [ ] **Client** prints the correct error messages if the server violates the HTTP protocoll
- [ ] **Client** prints the correct error messages if the server answers with the wrong HTTP version


- [x] **Server** has correct arguments parsing.
- [x] **Server** sends correct/required headers
- [x] **Server** serves the correct file content (only binary delivery can be tested)
- [x] **Server** sends correct status code upon success
- [x] **Server** sends correct status code if file is missing 
- [x] **Server** sends correct status code if method not supported
- [x] **Server** sends correct status code if HTTP version is not supported
- [x] **Server** sends correct status code if client violates HTTP protocoll
- [x] **Server** Only sends compressed content if client can accept it
- [x] **Server** Sends compressed content
- [x] **Server** The content-length is also the amount the server sent
