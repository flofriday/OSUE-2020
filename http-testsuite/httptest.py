import subprocess
import os
import shutil
import urllib.request


class HttpTest:
    def __init__(self):
        self.tests = 0
        self.tests_failed = 0
        self._create_dir()

    def _create_dir(self):
        # Create a directory to output files
        if os.path.exists("__tmp"):
            shutil.rmtree("__tmp")
        os.makedirs("__tmp")

    # This testcase expects the program to exit with an provided exit code.
    # It is considered failing if the exitcodes are different or if the
    # process does not exit after 0.25 seconds.
    def is_returncode(self, command: str, code: int) -> bool:
        self.tests += 1
        try:
            cp = subprocess.run(
                command,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=0.5,
                shell=True,
            )
        except subprocess.TimeoutExpired as e:
            print(f"🚨 Test {self.tests} Failed")
            print(
                f'"{command}" ran for more than 0.5s, but was expected to return {code}'
            )
            self.tests_failed += 1
            return False

        if cp.returncode is not code:
            self.tests_failed += 1
            print(f"🚨 Test {self.tests} Failed")
            print(f'"{command}" returned with {cp.returncode} instead of {code}')
            return False

        print(f"✅ Test {self.tests} Passed")
        return True

    # This code just checks if the right file gets created, however it does not
    # not check if the file contains any data.
    def creates_file(self, command: str, path: str) -> bool:
        self.tests += 1
        self._create_dir()
        try:
            cp = subprocess.run(
                command,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=0.5,
                shell=True,
            )
        except subprocess.TimeoutExpired as e:
            print(f"🚨 Test {self.tests} Failed")
            print(
                f'"{command}" ran for more than 0.5s, but was expected to execute faster.'
            )
            self.tests_failed += 1
            return False

        if cp.returncode is not 0:
            self.tests_failed += 1
            print(f"🚨 Test {self.tests} Failed")
            print(f'"{command}" returned with {cp.returncode} instead of 0')
            return False

        if not os.path.exists(path):
            self.tests_failed += 1
            print(f"🚨 Test {self.tests} Failed")
            print(f'"{command}" did not create file "{path}"')
            return False

        print(f"✅ Test {self.tests} Passed")
        return True

    # Run the command and afterwards compare the file saved in path with what
    # python receives in the response when requesting the url
    def compare_output(self, command: str, path: str, url: str) -> bool:
        self.tests += 1
        self._create_dir()

        try:
            cp = subprocess.run(
                command,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=0.5,
                shell=True,
            )
        except subprocess.TimeoutExpired as e:
            print(f"🚨 Test {self.tests} Failed")
            print(
                f'"{command}" ran for more than 0.5s, but was expected to execute faster.'
            )
            self.tests_failed += 1
            return False

        if cp.returncode is not 0:
            self.tests_failed += 1
            print(f"🚨 Test {self.tests} Failed")
            print(f'"{command}" returned with {cp.returncode} instead of 0')
            return False

        if not os.path.exists(path):
            self.tests_failed += 1
            print(f"🚨 Test {self.tests} Failed")
            print(f'"{command}" did not create file "{path}"')
            return False

        body = urllib.request.urlopen(url).read()
        file = open(path, "rb")
        output = file.read()
        if body != output:
            self.tests_failed += 1
            print(f"🚨 Test {self.tests} Failed")
            print(f'"{command}" did not create the same output as python.')
            return False

        print(f"✅ Test {self.tests} Passed")
        return True

    # Check if the command prints a certain phrase to either stdout or stderr
    def does_print(self, command: str, output: str) -> bool:
        self.tests += 1

        try:
            cp = subprocess.run(
                command,
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=0.5,
            )
        except subprocess.TimeoutExpired as e:
            print(f"🚨 Test {self.tests} Failed")
            print(
                f'"{command}" ran for more than 0.5s, but was expected to execute faster.'
            )
            self.tests_failed += 1
            return False

        if not output in cp.stdout.decode():
            print(f"🚨 Test {self.tests} Failed")
            print(f'"{command}" did not print "{output}".')
            self.tests_failed += 1
            return False

        print(f"✅ Test {self.tests} Passed")
        return True

    def print_result(self):
        self._create_dir()
        print(f"{20*'-'} Statistics {20*'-'}")
        if self.tests_failed == 0:
            print(f"🎉 All {self.tests} tests passed 🎉")
        else:
            print(f"⚠️ {self.tests_failed} of {self.tests} tests failed. ⚠️")
