from contextlib import contextmanager
from pathlib import Path
import select
import socket
import subprocess
import tempfile
import time


HOST = "127.0.0.1"
FALLBACK = b"Please tryagain.\n"


@contextmanager
def connect(port):
    with socket.create_connection((HOST, port), timeout=2) as sock:
        with sock.makefile("rb") as replies:
            assert replies.readline() == b"Hi ,[127.0.0.1] connect server success.\n"
            yield sock, replies


def check_time(replies):
    time.strptime(replies.readline().decode("ascii").strip(), "%a %b %d %H:%M:%S %Y")


def main():
    root = Path(__file__).resolve().parent
    with socket.socket() as port_socket:
        port_socket.bind((HOST, 0))
        port = port_socket.getsockname()[1]

    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / "server"
        subprocess.run(
            ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", f"-DSERVER_PORT={port}",
             str(root / "select_server.c"), "-o", str(exe)],
            check=True,
        )
        with subprocess.Popen(
            [str(exe)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        ) as server:
            try:
                assert select.select([server.stdout], [], [], 3)[0], "server startup timed out"
                started = server.stdout.readline()
                assert f"{HOST}:{port}" in started, f"server failed to start: {started!r}"

                with connect(port) as (a, a_replies):
                    with connect(port) as (b, b_replies):
                        b.sendall(b"time\n")
                        check_time(b_replies)

                        a.sendall(b"ti")
                        b.sendall(b"hello\n")
                        assert b_replies.readline() == FALLBACK
                        a.sendall(b"me\r\nunknown\ntime\n")
                        check_time(a_replies)
                        assert a_replies.readline() == FALLBACK
                        check_time(a_replies)

                        b.shutdown(socket.SHUT_WR)
                        assert b_replies.readline() == b""

                    with connect(port) as (c, c_replies):
                        c.sendall(b"time\n")
                        check_time(c_replies)
                        a.sendall(b"time\n")
                        check_time(a_replies)
            finally:
                if server.poll() is None:
                    server.terminate()
                server.communicate(timeout=3)
    print("PASS: multiple clients, command framing, disconnect and reconnect")


if __name__ == "__main__":
    main()
