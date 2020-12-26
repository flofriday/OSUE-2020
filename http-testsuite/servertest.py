from httptest import HttpTest
import os
import urllib.request
import subprocess
from datetime import datetime


def main():
    h = HttpTest()
    create_docroot()

    # Test the server with invalid arguments
    h.is_returncode("./server", 1)
    h.is_returncode("./server -p __docroot/", 1)
    h.is_returncode("./server -p 12 __docroot/index.html", 1)

    # Test the server with valid arguments
    h.does_timeout("./server -p 1334 __docroot")
    h.does_timeout("./server -p 1335 -i servertest.py ./__docroot/")

    # Start a server in the background and test a couple of requests
    p = start_server("./server -p 1337 __docroot")
    try:
        h.response_header_contains(
            "http://localhost:1337/", "Connection", "close"
        )

        index_size = os.path.getsize("__docroot/index.html")
        h.response_header_contains(
            "http://localhost:1337/", "Content-Length", str(index_size)
        )

        date_text = datetime.now().strftime("%a, %d %b %y %H:%M:%S CET")
        h.response_header_contains("http://localhost:1337/", "Date", date_text)

        if not h.response_header_contains(
            "http://localhost:1337/", "Content-Type", "text/html"
        ):
            print("NOTE: This is a bonus task.")

        if not h.response_header_contains(
            "http://localhost:1337/countdown.js",
            "Content-Type",
            "application/javascript",
        ):
            print("NOTE: This is a bonus task.")

        if not h.response_header_contains(
            "http://localhost:1337/solarized.css", "Content-Type", "text/css"
        ):
            print("NOTE: This is a bonus task.")

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

        h.is_statuscode("http://localhost:1337/", 200)
        h.is_statuscode("http://localhost:1337/index.html", 200)
        h.is_statuscode("http://localhost:1337/countdown.js", 200)
        h.is_statuscode("http://localhost:1337/cat.png", 200)
        h.is_statuscode("http://localhost:1337/solarized.css", 200)
        h.is_statuscode("http://localhost:1337/doesnotexist", 404)

        h.is_statuscode("http://localhost:1337/index.html", 501, method="HEAD")
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

    except Exception as e:
        stop_server(p)
        raise e

    stop_server(p)

    h.print_result()


def start_server(command: str) -> subprocess.Popen:
    return subprocess.Popen(
        command,
        shell=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def stop_server(process: subprocess.Popen):
    process.terminate()
    process.wait(timeout=0.5)


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


def download_file(url: str, path: str):
    body = urllib.request.urlopen(url).read()
    file = open(path, "wb")
    file.write(body)


main()