from httptest import HttpTest

def main():
    h = HttpTest()
    h.is_returncode("./client http://pan.vmars.tuwien.ac.at/osue/", 0)
    h.is_returncode("./client -p 80 http://pan.vmars.tuwien.ac.at/osue/", 0)

    # Missing http scheme
    h.is_returncode("./client pan.vmars.tuwien.ac.at/osue/", 1)

    # Missing arguments for the flags
    h.is_returncode("./client -p http://pan.vmars.tuwien.ac.at/osue/", 1)
    h.is_returncode("./client -o http://pan.vmars.tuwien.ac.at/osue/", 1)
    h.is_returncode("./client -d http://pan.vmars.tuwien.ac.at/osue/", 1)
    h.print_result()
main()