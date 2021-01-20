from httptest import HttpTest


def main():
    # A quick fis as pan.vmars.tuwein.ac.at/osue/ is down since almost a week
    # base_url = "http://localhost"
    base_url = "http://pan.vmars.tuwien.ac.at/osue"
    index_url = f"{base_url}/"
    countdown_url = f"{base_url}/countdown.js"
    cat_url = f"{base_url}/cat.png"
    port = 80
    base2_url = f"http://pan.vmars.tuwien.ac.at:{port}/osue"
    index2_url = f"{base2_url}/"
    countdown2_url = f"{base2_url}/countdown.js"
    cat2_url = f"{base2_url}/cat.png"

    # Initialize the testsuite
    h = HttpTest()

    # Working examples
    h.is_returncode(f"./client -p {port} {index_url}", 0)
    h.is_returncode(f"./client -p {port} {countdown_url}", 0)
    h.is_returncode(f"./client -p {port} {cat_url}", 0)
    h.is_returncode("./client http://neverssl.com", 0)
    h.is_returncode("./client http://www.nonhttps.com/", 0)
    h.is_returncode(f"./client -p {port} {index_url}", 0)

    # Missing http scheme
    h.is_returncode(f"./client -p {port} localhost/", 1)

    # Missing arguments for the flags
    h.is_returncode(f"./client -p {index_url}", 1)
    h.is_returncode(f"./client -o {index_url}", 1)
    h.is_returncode(f"./client -d {index_url}", 1)

    # Statuscodes that are not 200 OK
    h.is_returncode(f"./client -p {port} {base_url}/does-not-exist", 3)
    h.does_print(
        f"./client -p {port} {base_url}/does-not-exist",
        "404 Not Found",
    )

    # Conflicting arguments
    h.is_returncode(
        f"./client -p {port} -d __tmp -o __tmp/index.html {index_url}",
        1,
    )
    h.is_returncode(
        f"./client -p {port} -d __tmp -d __tmp/ {index_url}",
        1,
    )
    h.is_returncode(
        f"./client -p {port} -o __tmp/index.html -o __tmp/index.html {index_url}",
        1,
    )

    # Check if the client creates the right filenames
    h.creates_file(
        f"./client -p {port} -o __tmp/index.html {index_url}",
        "__tmp/index.html",
    )
    h.creates_file(
        f"./client -p {port} -o __tmp/dog.txt {index_url}",
        "__tmp/dog.txt",
    )
    h.creates_file(
        f"./client -p {port} -o __tmp/dog.txt {countdown_url}",
        "__tmp/dog.txt",
    )
    h.creates_file(
        f"./client -p {port} -d __tmp {index_url}",
        "__tmp/index.html",
    )
    h.creates_file(
        f"./client -p {port} -d __tmp/ {index_url}",
        "__tmp/index.html",
    )
    h.creates_file(
        f"./client -p {port} -d __tmp/ {countdown_url}",
        "__tmp/countdown.js",
    )
    h.creates_file(
        f"./client -p {port} -d __tmp/ {cat_url}",
        "__tmp/cat.png",
    )

    # Compare responses
    h.compare_output(
        f"./client -p {port} -o __tmp/index.html {index_url}",
        "__tmp/index.html",
        index2_url,
    )
    h.compare_output(
        f"./client -p {port} -o __tmp/dog.txt {index_url}",
        "__tmp/dog.txt",
        index2_url,
    )
    h.compare_output(
        f"./client -p {port} -o __tmp/dog.txt {countdown_url}",
        "__tmp/dog.txt",
        countdown2_url,
    )
    h.compare_output(
        f"./client -p {port} -d __tmp {index_url}",
        "__tmp/index.html",
        index2_url,
    )
    h.compare_output(
        f"./client -p {port} -d __tmp/ {countdown_url}",
        "__tmp/countdown.js",
        countdown2_url,
    )
    if not h.compare_output(
        f"./client -p {port} -d __tmp/ {cat_url}",
        "__tmp/cat.png",
        cat2_url,
    ):
        print(
            "NOTE: this test might fail because you didn't implement binary reading in your client which is a bonus exercise."
        )

    # Test for memoryleaks with valgrind
    h.does_leak(f"./client -p {port} {index_url}")
    h.does_leak(f"./client -p {port} {countdown_url}")
    h.does_leak(f"./client -p {port} {cat_url}")
    h.does_leak(
        "./client http://neverssl.com",
    )
    h.does_leak("./client http://www.nonhttps.com/")
    h.does_leak(f"./client -p {port} {index_url}")
    h.does_leak(f"./client -p {port} localhost/")
    h.does_leak(f"./client -p {index_url}")
    h.does_leak(f"./client -o {index_url}")
    h.does_leak(f"./client -d {index_url}")
    h.does_leak(f"./client -p {port} {base_url}/does-not-exist")
    h.does_leak(f"./client -p {port} -o __tmp/index.html {index_url}")
    h.does_leak(f"./client -p {port} -o __tmp/dog.txt {index_url}")
    h.does_leak(f"./client -p {port} -o __tmp/dog.txt {countdown_url}")
    h.does_leak(f"./client -p {port} -d __tmp {index_url}")
    h.does_leak(f"./client -p {port} -d __tmp/ {countdown_url}")
    h.does_leak(f"./client -p {port} -d __tmp/ {cat_url}")

    # Print the statistics at the end
    h.print_result()


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(
            """
                      ___  _   _   ___  _  __
                     | __|| | | | / __|| |/ /
                     | _| | |_| || (__ |   < 
                     |_|   \___/  \___||_|\_\\

                    🔥 The Testsuite crashed! 🔥

Okay, this shouldn't have happend and is my (the testsuite dev) fault.
However, there is probably some error in your code that triggered the crash.

Here is what you do:
1) Read the Python Traceback below and double check your code for bugs.
2) Write a Issue on GitHub (include the Traceback):
   https://github.com/flofriday/OSUE-2020/issues/new



--- TRACEBACK BELOW ---
        """
        )
        raise e
