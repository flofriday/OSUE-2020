import subprocess
import os
import shutil

class HttpTest:
    def __init__(self):
        self.tests = 0
        self.tests_failed = 0
        self._create_dir()

    def _create_dir(self):
        # Create a directory to output files
        if os.path.exists('__tmp'):
            shutil.rmtree('__tmp')
        os.makedirs('__tmp')

    # This testcase expects the program to exit with an provided exit code.
    # It is considered failing if the exitcodes are different or if the
    # process does not exit after 0.25 seconds.
    def is_returncode(self, command:str, code:int) -> bool:
        self.tests += 1
        try:
            cp = subprocess.run(command.split(" "), 
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=0.5,
            )
        except subprocess.TimeoutExpired as e:
            print('🚨 Test Failed')
            print(f'"{command}" ran for more than 0.5s, but was expected to return {code}')
            self.tests_failed += 1
            return False

        if cp.returncode is not code:
            self.tests_failed += 1
            print('🚨 Test Failed')
            print(f'"{command}" returned with {cp.returncode} instead of {code}')

        return cp.returncode is code

    # This code just checks if the right file gets created, however it does not 
    # not check if the file contains any data.
    def creates_file(self, command:str, path:str) -> bool:
        self.tests += 1
        self._create_dir()
        try:
            cp = subprocess.run(command.split(" "), 
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=0.5,
            )
        except subprocess.TimeoutExpired as e:
            print('🚨 Test Failed')
            print(f'"{command}" ran for more than 0.5s, but was expected to execute faster.')
            self.tests_failed += 1
            return False

        if cp.returncode is not 0:
            self.tests_failed += 1
            print('🚨 Test Failed')
            print(f'"{command}" returned with {cp.returncode} instead of 0')
            return False
        
        if not os.path.exists(path):
            self.tests_failed += 1
            print('🚨 Test Failed')
            print(f'"{command}" did not create file "{path}"')
            return False

        return True
 
        
    def print_result(self):
        self._create_dir()
        if self.tests_failed == 0:
            print(f"All {self.tests} tests passed 🎉")
        else:
            print(f"{20*'-'} Statistics {20*'-'}")
            print(f"{self.tests_failed} of {self.tests} tests failed.")
