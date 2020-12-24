from httptest import HttpTest


def main():
    h = HttpTest()

    # These calls are not allowed and shold therefore fail
    h.is_returncode("./server", 1)
    h.is_returncode("./server -p 12 323", 1)
    h.is_returncode("./server -m 1", 1)
    h.print_result()


main()
