from httptest import HttpTest


def main():
    h = HttpTest()

    # Working examples
    h.is_returncode("./client http://pan.vmars.tuwien.ac.at/osue/", 0)
    h.is_returncode(
        "./client http://pan.vmars.tuwien.ac.at/osue/countdown.js", 0
    )
    h.is_returncode("./client http://pan.vmars.tuwien.ac.at/osue/cat.png", 0)
    h.is_returncode("./client http://neverssl.com", 0)
    h.is_returncode("./client http://www.nonhttps.com/", 0)
    h.is_returncode("./client -p 80 http://pan.vmars.tuwien.ac.at/osue/", 0)

    # Missing http scheme
    h.is_returncode("./client pan.vmars.tuwien.ac.at/osue/", 1)

    # Missing arguments for the flags
    h.is_returncode("./client -p http://pan.vmars.tuwien.ac.at/osue/", 1)
    h.is_returncode("./client -o http://pan.vmars.tuwien.ac.at/osue/", 1)
    h.is_returncode("./client -d http://pan.vmars.tuwien.ac.at/osue/", 1)

    # Statuscodes that are not 200 OK
    h.is_returncode(
        "./client http://pan.vmars.tuwien.ac.at/osue/does-not-exist", 3
    )
    h.does_print(
        "./client http://pan.vmars.tuwien.ac.at/osue/does-not-exist",
        "404 Not Found",
    )

    # Conflicting arguments
    h.is_returncode(
        "./client -d __tmp -o __tmp/index.html http://pan.vmars.tuwien.ac.at/osue/",
        1,
    )
    h.is_returncode(
        "./client -d __tmp -d __tmp/ http://pan.vmars.tuwien.ac.at/osue/",
        1,
    )
    h.is_returncode(
        "./client -o __tmp/index.html -o __tmp/index.html http://pan.vmars.tuwien.ac.at/osue/",
        1,
    )

    # Check if the client creates the right filenames
    h.creates_file(
        "./client -o __tmp/index.html http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/index.html",
    )
    h.creates_file(
        "./client -o __tmp/dog.txt http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/dog.txt",
    )
    h.creates_file(
        "./client -o __tmp/dog.txt http://pan.vmars.tuwien.ac.at/osue/countdown.js",
        "__tmp/dog.txt",
    )
    h.creates_file(
        "./client -d __tmp http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/index.html",
    )
    h.creates_file(
        "./client -d __tmp/ http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/index.html",
    )
    h.creates_file(
        "./client -d __tmp/ http://pan.vmars.tuwien.ac.at/osue/countdown.js",
        "__tmp/countdown.js",
    )
    h.creates_file(
        "./client -d __tmp/ http://pan.vmars.tuwien.ac.at/osue/cat.png",
        "__tmp/cat.png",
    )

    # Compare responses
    h.compare_output(
        "./client -o __tmp/index.html http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/index.html",
        "http://pan.vmars.tuwien.ac.at/osue/",
    )
    h.compare_output(
        "./client -o __tmp/dog.txt http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/dog.txt",
        "http://pan.vmars.tuwien.ac.at/osue/",
    )
    h.compare_output(
        "./client -o __tmp/dog.txt http://pan.vmars.tuwien.ac.at/osue/countdown.js",
        "__tmp/dog.txt",
        "http://pan.vmars.tuwien.ac.at/osue/countdown.js",
    )
    h.compare_output(
        "./client -d __tmp http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/index.html",
        "http://pan.vmars.tuwien.ac.at/osue/",
    )
    h.compare_output(
        "./client -d __tmp/ http://pan.vmars.tuwien.ac.at/osue/",
        "__tmp/index.html",
        "http://pan.vmars.tuwien.ac.at/osue/",
    )
    h.compare_output(
        "./client -d __tmp/ http://pan.vmars.tuwien.ac.at/osue/countdown.js",
        "__tmp/countdown.js",
        "http://pan.vmars.tuwien.ac.at/osue/countdown.js",
    )
    if not h.compare_output(
        "./client -d __tmp/ http://pan.vmars.tuwien.ac.at/osue/cat.png",
        "__tmp/cat.png",
        "http://pan.vmars.tuwien.ac.at/osue/cat.png",
    ):
        print(
            "NOTE: this test might fail because you didn't implement binary reading in your client which is a bonus exercise."
        )

    # Print the statistics at the end
    h.print_result()


main()
