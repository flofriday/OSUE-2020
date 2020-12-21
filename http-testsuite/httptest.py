import subprocess

class HttpTest:
    def __init__(self):
        self.tests = 0
        self.tests_failed = 0

    def is_returncode(self, command:str, code:int) -> bool:
        self.tests += 1
        cp = subprocess.run(command.split(" "), stdout=subprocess.DEVNULL)
        if cp.returncode is not code:
            self.tests_failed += 1
            print(f'"{command}" returned with {cp.returncode} instead of {code}')
        return cp.returncode is code
        
    def print_result(self):
        if self.tests_failed == 0:
            print(f"All {self.tests} tests passed 🎉")
        else:
            print(f"{self.tests_failed} of {self.tests} tests failed.")
