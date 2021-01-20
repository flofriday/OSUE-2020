import subprocess
import os
import shutil
import urllib.request
from typing import Dict
import zlib
import http.client
import time


class HttpTest:

    # The constuctor prints a simple message about this testsuite and
    # initialises all testcounters and a temporary folder
    def __init__(self):
        self._tests = 0
        self._tests_failed = 0
        self._timeout = 1
        self._create_dir()
        print("OSUE Exercise 3 Testsuite (WS 2020/2021)")
        print(
            "GitHub: https://github.com/flofriday/OSUE-2020/tree/main/http-testsuite\n"
        )

    # Increase the internal testcounter and print a simple message to stdout
    def test_passed(self):
        self._tests += 1
        print(f"✅ Test {self._tests:02d} Passed")

    # Increate the interal testscounter and failed tests counter and print a
    # simple message to stdout
    def test_failed(self):
        self._tests += 1
        self._tests_failed += 1
        print(f"🚨 Test {self._tests:02d} Failed")

    # Create a directory called __tmp to be used save output files the client
    # creates.
    def _create_dir(self):
        # Create a directory to output files
        if os.path.exists("__tmp"):
            shutil.rmtree("__tmp")
        os.makedirs("__tmp")

    # This testcase expects the program to exit with an provided exit code.
    # It is considered failing if the exitcodes are different or if the
    # process does not exit after 0.5 seconds.
    def is_returncode(self, command: str, code: int) -> bool:
        try:
            cp = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=self._timeout,
                shell=True,
            )
        except subprocess.TimeoutExpired:
            self.test_failed()
            print(
                f'"{command}" ran for more than {self._timeout}s, but was expected to return {code}'
            )
            return False

        if not cp.returncode == code:
            self.test_failed()
            print(
                f'"{command}" returned with {cp.returncode} instead of {code}'
            )
            return False

        self.test_passed()
        return True

    # This test expects the command to run for 0.5 seconds, if the commands
    # exits earlier for what ever reason this test will fail.
    def does_timeout(self, command: str) -> bool:
        try:
            cp = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=self._timeout,
                shell=True,
            )
        except subprocess.TimeoutExpired:
            self.test_passed()
            return True

        self.test_failed()
        print(
            f'"{command}" should run forever but did exit with {cp.returncode}.'
        )
        return False

    def does_leak(self, command: str) -> bool:
        valgrind_cmd = (
            "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=__tmp/valgrind-out.txt "
            + command
        )
        try:
            subprocess.run(
                valgrind_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=self._timeout * 10,
                shell=True,
            )
        except subprocess.TimeoutExpired:
            self.test_failed()
            print(
                f'"{command}" with valgrind ran for more than {self._timeout * 10}s, but was expected to execute faster.'
            )
            return False

        # Read the outputfile and delte it again
        f = open("__tmp/valgrind-out.txt", "rb")
        output = f.read().decode("utf-8")
        f.close()
        os.remove("__tmp/valgrind-out.txt")

        # Parse the outputfile
        if (
            "All heap blocks were freed -- no leaks are possibl" not in output
            or "ERROR SUMMARY: 0 errors from 0 contexts" not in output
        ):
            self.test_failed()
            print(f'"{command}" resulted in a memmory corruption/leak.')
            print("Run the following to reproduce the error:")
            print(
                f"valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose {command}"
            )
            return False

        self.test_passed()
        return True

    # Start the server to test it
    # This is not a test
    def start_server(self, command: str) -> subprocess.Popen:
        valgrind_cmd = (
            "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=__tmp/valgrind-out.txt "
            + command
        )
        p = subprocess.Popen(
            valgrind_cmd,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        time.sleep(3)
        return p

    # Stop the server
    # This tests the server for leaks and if it can handle sigterm
    def stop_server(self, process: subprocess.Popen):
        success = True
        try:
            process.terminate()
            process.wait(timeout=0.5)
            self.test_passed()
        except subprocess.TimeoutExpired:
            self.test_failed()
            print(
                "The server didn't terminate in the 0.5sec after SIGTERM was sent"
            )
            success = False

        # Parse valgrind
        f = open("__tmp/valgrind-out.txt", "rb")
        output = f.read().decode("utf-8")
        f.close()
        os.remove("__tmp/valgrind-out.txt")

        # Parse the outputfile
        if (
            "All heap blocks were freed -- no leaks are possibl" not in output
            or "ERROR SUMMARY: 0 errors from 0 contexts" not in output
        ):
            self.test_failed()
            print("The server has a memmory corruption/leak.")
            print(
                "Start your server with the following and request some files:"
            )
            print(
                "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./server -p 1339 __docroot/"
            )
            print("--- Valgrind Output ---")
            print(output)

            success = False
        else:
            self.test_passed()

        print("\n--- Server Output Start ---")
        print(process.stdout.read().decode().strip())
        print("--- Server Output End ---")
        return success

    # This test runs the command and checks afterwards if the file at path got
    # created. However, this test does not check if the file has any content
    # whatsoever. Also this test will fail if the command does not return with
    # exit code 0.
    def creates_file(self, command: str, path: str) -> bool:
        self._create_dir()
        try:
            cp = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=self._timeout,
                shell=True,
            )
        except subprocess.TimeoutExpired:
            self.test_failed()
            print(
                f'"{command}" ran for more than {self._timeout}s, but was expected to execute faster.'
            )
            return False

        if cp.returncode != 0:
            self.test_failed()
            print(f'"{command}" returned with {cp.returncode} instead of 0')
            return False

        if not os.path.exists(path):
            self.test_failed()
            print(f'"{command}" did not create file "{path}"')
            return False

        self.test_passed()
        return True

    # Run the command and afterwards compare the file saved in path with what
    # python receives in the response when requesting the url.
    # This test will also fail if the command runs for more than 0.5 seconds or
    # the command does not return with exit-code 0 or the file path points to
    # does not exist.
    def compare_output(self, command: str, path: str, url: str) -> bool:
        self._create_dir()
        try:
            cp = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=self._timeout,
                shell=True,
            )
        except subprocess.TimeoutExpired:
            self.test_failed()
            print(
                f'"{command}" ran for more than {self._timeout}s, but was expected to execute faster.'
            )
            return False

        if cp.returncode != 0:
            self.test_failed()
            print(f'"{command}" returned with {cp.returncode} instead of 0')
            return False

        if not os.path.exists(path):
            self.test_failed()
            print(f'"{command}" did not create file "{path}"')
            return False

        res = urllib.request.urlopen(url)

        body = res.read()
        file = open(path, "rb")
        output = file.read()
        if body != output:
            self.test_failed()
            print(
                f'"{command}" did not create the same output as Python 3 (with urllib).'
            )
            return False

        self.test_passed()
        return True

    # This test creates a request to url. Then it will look if there is a
    # filed in the response headers with the name and value provided to this
    # test.
    def in_response_header(
        self,
        url: str,
        name: str,
        value: str,
        method: str = "GET",
        headers: Dict[str, str] = {},
    ) -> bool:
        req = urllib.request.Request(url, method=method, headers=headers)
        try:
            res = urllib.request.urlopen(req)
            res_headers = res.headers
            res.read()
        except http.client.IncompleteRead:
            pass
        except urllib.error.HTTPError as e:
            res_headers = e.headers
        except urllib.error.URLError as e:
            self.test_failed()
            print(f"Request to {url} failed horribly. Reason: {e.reason}")
            if method != "GET":
                print(f"The request was made with HTTP-Method {method}.")
            if headers != {}:
                print(
                    f'The request was send with the additional header "{headers}"'
                )
            return False

        if name not in res_headers or value != res_headers[name]:
            self.test_failed()
            print(
                f'Response to "{url}" did not contain "{name}: {value}" in the headers.'
            )
            if method != "GET":
                print(f"The request was made with HTTP-Method {method}.")
            if headers != {}:
                print(
                    f'The request was send with the additional header "{headers}"'
                )
            if name in res_headers:
                print(f'Closest Header was: "{name}: {res_headers[name]}"')
            return False

        self.test_passed()
        return True

    # Equal to in_response_header but fails if the field is in the response
    def notin_response_header(
        self, url: str, name: str, value: str, method: str = "GET"
    ) -> bool:
        req = urllib.request.Request(url, method=method)
        try:
            res = urllib.request.urlopen(req)
            headers = res.headers
            res.read()
        except http.client.IncompleteRead:
            pass
        except urllib.error.HTTPError as e:
            headers = e.headers
        except urllib.error.URLError as e:
            self.test_failed()
            print(f"Request to {url} failed horribly. Reason: {e.reason}")
            if method != "GET":
                print(f"The request was made with HTTP-Method {method}.")
            return False

        if name in headers and value == headers[name]:
            self.test_failed()
            print(
                f'Response to "{url}" did contain "{name}: {value}" in the headers.'
            )
            if method != "GET":
                print(f"The request was made with HTTP-Method {method}.")
            return False

        self.test_passed()
        return True

    # Check if the Content-Length is equal to the number of bytes in the
    # response body
    def verify_content_length(self, url: str, headers: Dict[str, str] = {}):
        req = urllib.request.Request(url, headers=headers)
        try:
            res = urllib.request.urlopen(req)
            content_size = len(res.read())
        except http.client.IncompleteRead:
            # We test it below explicitly so we can just ignore the exception
            # here.
            pass
        except urllib.error.URLError as e:
            self.test_failed()
            print(f"The request failed: {e.reason}")
            return False

        if "Content-Length" not in res.headers:
            self.test_failed()
            print(
                f'Response to {url} did not contain the header "Content-Length"'
            )
            return False

        size = int(res.headers["Content-Length"])
        if size != content_size:
            self.test_failed()
            print(
                f'The response to "{url}" had the header "Content-Length: {content_size}" however the body sent was {size} bytes long.'
            )
            return False

        self.test_passed()
        return True

    # This test creates a request to url and then compares the response body to
    # the content of the file pointed to by path.
    def compare_response_body(
        self, url: str, path: str, headers: Dict[str, str] = {}
    ) -> bool:
        req = urllib.request.Request(url, headers=headers)
        try:
            res = urllib.request.urlopen(req)
            raw_body = res.read()
        except http.client.IncompleteRead as e:
            raw_body = e.partial
        except urllib.error.URLError as e:
            self.test_failed()
            print(
                f"The request returned with status {e.reason} instead of 200"
            )
            return False

        if (
            "Content-Encoding" in res.headers
            and "gzip" in res.headers["Content-Encoding"]
        ):
            zobj = zlib.decompressobj(zlib.MAX_WBITS | 32)
            body = zobj.decompress(raw_body)
        else:
            body = raw_body

        content = open(path, "rb").read()
        if body != content:
            self.test_failed()
            print(
                f'"Response to {url}" was not the same content as in "{path}"'
            )
            print(
                f"Response was {len(body)} bytes while original was {len(content)}"
            )
            if headers != {}:
                print(
                    f'The request was send with the additional header "{headers}"'
                )
            print(
                "NOTE: This might be because you haven't yet implemented the bonus task, binary files."
            )
            return False

        self.test_passed()
        return True

    # This test creates a request to url (optiona with the method provided) and
    # checks if the statuscode provided is equal to the status code of the
    # response. This test will fail if they are not equal.
    def is_statuscode(
        self,
        url: str,
        status: int,
        method: str = "GET",
        headers: Dict[str, str] = {},
    ) -> bool:
        req = urllib.request.Request(url, method=method, headers=headers)
        try:
            res = urllib.request.urlopen(req)
            code = res.status
            res.read()
        except http.client.IncompleteRead:
            pass
        except urllib.error.HTTPError as e:
            code = e.code
        except urllib.error.URLError as e:
            self.test_failed()
            print(f"Request to {url} failed horribly. Reason: {e.reason}")
            if headers != {}:
                print(
                    f'The request was send with the additional header "{headers}"'
                )
            if method != "GET":
                print(f"The request was made with HTTP-Method {method}.")
            return False

        if status != code:
            self.test_failed()
            print(
                f'"Response to {url}" returned with status {code} instead of {status}'
            )
            if headers != {}:
                print(
                    f'The request was send with the additional header "{headers}"'
                )
            if method != "GET":
                print(f"The request was made with HTTP-Method {method}.")
            return False

        self.test_passed()
        return True

    # Check if the command prints a certain phrase to either stdout or stderr
    def does_print(self, command: str, expected_output: str) -> bool:
        try:
            cp = subprocess.run(
                command,
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=self._timeout,
            )
        except subprocess.TimeoutExpired:
            self.test_failed()
            print(
                f'"{command}" ran for more than {self._timeout}s, but was expected to execute faster.'
            )
            return False

        output = cp.stdout.decode()
        if expected_output not in cp.stdout.decode():
            self.test_failed()
            print(f'"{command}" did not print "{expected_output}".')
            print(f"---Program Output---\n{output}")
            return False

        self.test_passed()
        return True

    # Print a very simple statistics to stdout
    def print_result(self):
        self._create_dir()
        print(f"\n{20*'-'} Statistics {20*'-'}")
        if self._tests_failed == 0:
            print(f"🎉 All {self._tests} tests passed 🎉")
        else:
            print(
                f"⚠️  {self._tests_failed} out of {self._tests} tests failed. ⚠️"
            )
