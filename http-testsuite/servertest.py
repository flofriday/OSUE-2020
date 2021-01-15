from httptest import HttpTest
import os
import urllib.request
import subprocess
import time
import http.client


def main():
    # Create a test object
    h = HttpTest()

    # Download some files from https://pan.vmars.tuwien.ac.at/osue/ to use in
    # the tests.
    create_docroot()

    # Test the server with invalid arguments
    h.is_returncode("./server", 1)
    h.is_returncode("./server -p __docroot/", 1)
    h.is_returncode("./server -p 1331 -p 1332 __docroot/", 1)
    h.is_returncode("./server -p 12 __docroot/index.html", 1)

    # Test the server with valid arguments
    h.does_timeout("./server -p 1334 __docroot")
    h.does_timeout("./server -p 1335 -i servertest.py ./__docroot/")

    # Start a server in the background and test a couple of requests
    p = h.start_server("./server -p 1337 __docroot")
    try:

        gzip_header = {"Accept-Encoding": "gzip"}

        # Check response headers upon success
        h.in_response_header("http://localhost:1337/", "Connection", "close")

        index_size = os.path.getsize("__docroot/index.html")
        h.in_response_header(
            "http://localhost:1337/", "Content-Length", str(index_size)
        )

        date_text = time.strftime("%a, %d %b %y %T %Z", time.gmtime())
        h.in_response_header("http://localhost:1337/", "Date", date_text)

        if not h.in_response_header(
            "http://localhost:1337/", "Content-Type", "text/html"
        ):
            print("NOTE: This is a bonus task.")

        if not h.in_response_header(
            "http://localhost:1337/countdown.js",
            "Content-Type",
            "application/javascript",
        ):
            print("NOTE: This is a bonus task.")

        if not h.in_response_header(
            "http://localhost:1337/solarized.css", "Content-Type", "text/css"
        ):
            print("NOTE: This is a bonus task.")

        # Check if the response header contains the gzip if we request gzip
        if not h.in_response_header(
            "http://localhost:1337/",
            "Content-Encoding",
            "gzip",
            headers=gzip_header,
        ):
            print("NOTE: This is a bonus task.")

        # If the client doesn't tell the server that it understands gzip the
        # server cannot answer with gzip
        if not h.notin_response_header(
            "http://localhost:1337/", "Content-Encoding", "gzip"
        ):
            print(
                "NOTE: The server cannot send the client gzip data if the"
                + "client didn't tell the server that it understands gzip"
            )

        # Check headers of failing requests
        h.in_response_header(
            "http://localhost:1337/doesnotexist", "Connection", "close"
        )
        h.in_response_header(
            "http://localhost:1337/", "Connection", "close", method="POST"
        )

        # Check if response is the same as the file
        h.compare_response_body(
            "http://localhost:1337/", "__docroot/index.html"
        )
        h.compare_response_body(
            "http://localhost:1337/index.html", "__docroot/index.html"
        )
        h.compare_response_body(
            "http://localhost:1337/countdown.js", "__docroot/countdown.js"
        )
        h.compare_response_body(
            "http://localhost:1337/cat.png", "__docroot/cat.png"
        )
        h.compare_response_body(
            "http://localhost:1337/solarized.css", "__docroot/solarized.css"
        )

        h.compare_response_body(
            "http://localhost:1337/",
            "__docroot/index.html",
            headers=gzip_header,
        )
        h.compare_response_body(
            "http://localhost:1337/index.html",
            "__docroot/index.html",
            headers=gzip_header,
        )
        h.compare_response_body(
            "http://localhost:1337/countdown.js",
            "__docroot/countdown.js",
            headers=gzip_header,
        )
        h.compare_response_body(
            "http://localhost:1337/cat.png",
            "__docroot/cat.png",
            headers=gzip_header,
        )
        h.compare_response_body(
            "http://localhost:1337/solarized.css",
            "__docroot/solarized.css",
            headers=gzip_header,
        )

        # Check the content-lenght
        h.verify_content_length("http://localhost:1337/")
        h.verify_content_length("http://localhost:1337/countdown.js")
        h.verify_content_length("http://localhost:1337/cat.png")
        h.verify_content_length("http://localhost:1337/solarized.css")
        h.verify_content_length("http://localhost:1337/", headers=gzip_header)
        h.verify_content_length(
            "http://localhost:1337/countdown.js", headers=gzip_header
        )
        h.verify_content_length(
            "http://localhost:1337/cat.png", headers=gzip_header
        )
        h.verify_content_length(
            "http://localhost:1337/solarized.css", headers=gzip_header
        )

        # Check Status codes upon success
        h.is_statuscode("http://localhost:1337/", 200)
        h.is_statuscode("http://localhost:1337/index.html", 200)
        h.is_statuscode("http://localhost:1337/countdown.js", 200)
        h.is_statuscode("http://localhost:1337/cat.png", 200)
        h.is_statuscode("http://localhost:1337/solarized.css", 200)
        h.is_statuscode("http://localhost:1337/", 200, headers=gzip_header)
        h.is_statuscode(
            "http://localhost:1337/index.html", 200, headers=gzip_header
        )
        h.is_statuscode(
            "http://localhost:1337/countdown.js", 200, headers=gzip_header
        )
        h.is_statuscode(
            "http://localhost:1337/cat.png", 200, headers=gzip_header
        )
        h.is_statuscode(
            "http://localhost:1337/solarized.css", 200, headers=gzip_header
        )

        # Check status codes of failed requests
        h.is_statuscode(
            "http://localhost:1337/doesnotexist", 404, method="GET"
        )
        h.is_statuscode(
            "http://localhost:1337/doesnotexist",
            404,
            method="GET",
            headers=gzip_header,
        )
        h.is_statuscode("http://localhost:1337/index.html", 501, method="HEAD")
        h.is_statuscode(
            "http://localhost:1337/index.html",
            501,
            method="HEAD",
            headers=gzip_header,
        )
        h.is_statuscode("http://localhost:1337/index.html", 501, method="POST")
        h.is_statuscode("http://localhost:1337/index.html", 501, method="PUT")
        h.is_statuscode(
            "http://localhost:1337/index.html", 501, method="DELETE"
        )
        h.is_statuscode(
            "http://localhost:1337/index.html", 501, method="CONNECT"
        )
        h.is_statuscode(
            "http://localhost:1337/index.html", 501, method="OPTIONS"
        )
        h.is_statuscode(
            "http://localhost:1337/index.html", 501, method="TRACE"
        )
        h.is_statuscode(
            "http://localhost:1337/index.html", 501, method="PATCH"
        )

        # Now lets simulate a client so bad we have to write it ourself.
        # Oh boy/girl this will be fun 😈
        first = "GET / HTTP/1.0"
        res = send_bad_request("localhost:1337", first)
        if res.status == 400:
            h.test_passed()
        else:
            h.test_failed()
            print(
                f'A request with "{first}" responded with status {res.status} instead of 400'
            )

        first = "GET / HTTP/1.3"
        res = send_bad_request("localhost:1337", first)
        if res.status == 400:
            h.test_passed()
        else:
            h.test_failed()
            print(
                f'A request with "{first}" responded with status {res.status} instead of 400'
            )

        first = "GET HTTP/1.1"
        res = send_bad_request("localhost:1337", first)
        if res.status == 400:
            h.test_passed()
        else:
            h.test_failed()
            print(
                f'A request with "{first}" responded with status {res.status} instead of 400'
            )

        first = "GET / HTTP/1.1 supersecrethiddenfieldthatshouldnotbeaccepted"
        res = send_bad_request("localhost:1337", first)
        if res.status == 400:
            h.test_passed()
        else:
            h.test_failed()
            print(
                f'A request with "{first}" responded with status {res.status} instead of 400'
            )

    except Exception as e:
        h.stop_server(p)
        raise e

    h.stop_server(p)

    # Print the statistics and results
    h.print_result()


# Send a really bad request to the server.
# The caller can specify what the first file of the request is and so we can
# test how the server reacts to misbehaving clients.
def send_bad_request(host: str, first_line: str) -> http.client.HTTPResponse:
    h1 = http.client.HTTPConnection(host)
    h1.connect()

    # WARNING: THIS IS BAD CODE!
    # I just do this because I know what Python version is running on the
    # server and because I don't have enough time, but this should never be
    # done in any production codebase.
    # Ok here we are messing with the internal state of an object in a way
    # that we shouldn't but it is so much easier than writing response parsing
    # own our own.
    h1._HTTPConnection__state = "Request-started"
    h1._method = "GET"
    h1._output(h1._encode_request(first_line))

    h1.endheaders()
    return h1.getresponse()


# Create folder with files for the server to serve in the tests
def create_docroot():
    if not os.path.exists("__docroot"):
        os.makedirs("__docroot")

    if not os.path.exists("__docroot/index.html"):
        download_file(
            "http://pan.vmars.tuwien.ac.at/osue/", "__docroot/index.html"
        )

    if not os.path.exists("__docroot/countdown.js"):
        download_file(
            "http://pan.vmars.tuwien.ac.at/osue/countdown.js",
            "__docroot/countdown.js",
        )

    if not os.path.exists("__docroot/cat.png"):
        download_file(
            "http://pan.vmars.tuwien.ac.at/osue/cat.png", "__docroot/cat.png"
        )

    if not os.path.exists("__docroot/solarized.css"):
        download_file(
            "https://thomasf.github.io/solarized-css/solarized-light.css",
            "__docroot/solarized.css",
        )


# Just a simple helper to download a file and save it to a path.
def download_file(url: str, path: str):
    body = urllib.request.urlopen(url).read()
    file = open(path, "wb")
    file.write(body)


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
