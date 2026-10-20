#!/usr/bin/env python3
"""Exercise the real LLDB transport without requiring Intel translation.

Pass --intel to test the same operations on an x86-64 program instead.
"""
import argparse
import json
import os
from pathlib import Path
import select
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent
BRIDGE = ROOT / "source/src/sasm/BUILD/macos"


class Client:
    def __init__(self, executable, source, input_file):
        script = ("script import sys; sys.dont_write_bytecode=True; sys.path.insert(0, "
                  + repr(str(BRIDGE)) + "); import lldb_bridge; lldb_bridge.run(lldb.debugger, "
                  + ", ".join(repr(str(p)) for p in (executable, source.parent, input_file, source)) + ")")
        self.process = subprocess.Popen(
            ["/usr/bin/xcrun", "lldb", "--no-lldbinit", "--no-use-colors", "--batch", "-o", script],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.pending = b""
        self.events = []
        self.output = ""
        self.diagnostics = ""

    def send(self, command, **args):
        self.process.stdin.write(json.dumps(dict(command=command, **args)).encode() + b"\n")
        self.process.stdin.flush()

    def wait(self, event):
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline:
            for index, message in enumerate(self.events):
                if message["event"] == "error":
                    raise AssertionError(message["text"])
                if message["event"] == event:
                    return self.events.pop(index)
            if not select.select([self.process.stdout], [], [], 0.1)[0]:
                continue
            data = os.read(self.process.stdout.fileno(), 65536)
            if not data:
                raise AssertionError("Debugger exited before " + event + "\n" + self.diagnostics)
            self.pending += data
            while b"\n" in self.pending:
                line, self.pending = self.pending.split(b"\n", 1)
                text = line.decode(errors="replace")
                if text.startswith("SASM:"):
                    message = json.loads(text[5:])
                    if message["event"] == "output":
                        self.output += message["text"]
                    self.events.append(message)
                else:
                    self.diagnostics += text + "\n"
        raise AssertionError("Timed out waiting for " + event + "\n" + self.diagnostics)

    def close(self):
        if self.process.poll() is None:
            self.send("quit")
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()


def test_nasm(directory):
    source = directory / "program.asm"
    source.write_text('%include "io64.inc"\nsection .data\nvalue dq 41\n'
                      'section .text\nglobal CMAIN\nCMAIN:\n'
                      '    inc qword [rel value]\n    GET_DEC 8, rax\n'
                      '    add rax, [rel value]\n    PRINT_DEC 8, rax\n'
                      '    NEWLINE\n    xor eax, eax\n    ret\n')
    input_file = directory / "input.txt"
    input_file.write_text("123\n")
    nasm = shutil.which("nasm") or "/opt/homebrew/bin/nasm"
    support = BRIDGE.parent / "Linux/share/sasm"
    obj = directory / "program.o"
    macro = directory / "macro.o"
    executable = directory / "program"
    subprocess.run([nasm, "-f", "macho64", "-g", "-F", "dwarf",
                    "-I", str(support / "include") + "/", "-I", str(BRIDGE / "include") + "/",
                    "-p", str(BRIDGE / "include/macos.inc"), str(source), "-o", str(obj)], check=True)
    subprocess.run(["/usr/bin/clang", "-arch", "x86_64", "-g", "-c",
                    str(support / "NASM/macro.c"), "-o", str(macro)], check=True)
    link = subprocess.run(["/usr/bin/clang", "-arch", "x86_64", "-g", str(obj), str(macro),
                           "-o", str(executable)], check=True, capture_output=True, text=True)
    assert "no platform load command" not in link.stderr, link.stderr
    client = Client(executable, source, input_file)
    try:
        assert client.wait("stopped")["line"] == 7
        client.send("memory", text="p (long long)(value)")
        assert client.wait("memory")["value"] == "41"
        client.send("step")
        assert client.wait("stopped")["line"] == 8
        client.send("breakpoint", line=10, added=True)
        client.send("continue")
        assert client.wait("stopped")["line"] == 10
        client.send("memory", text="p (long long)(value)")
        assert client.wait("memory")["value"] == "42"
        client.send("registers")
        registers = {item["name"]: item for item in client.wait("registers")["values"]}
        assert int(registers["rax"]["hex"], 16) == 165
        client.send("next")
        client.wait("stopped")
        client.send("continue")
        assert client.wait("exited")["code"] == 0
        assert client.output.strip() == "165", repr(client.output)
        print("PASS: NASM/macOS metadata, DWARF lines, assembly breakpoints, registers, memory, io64 input/output under Rosetta")
    finally:
        client.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--intel", action="store_true")
    parser.add_argument("--nasm", action="store_true", help="Also debug a NASM program using io64.inc")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="SASM debugger tests with spaces-") as directory:
        directory = Path(directory)
        source = directory / "test.c"
        source.write_text(
            '#include <stdio.h>\nint value = 42;\nint main(void) {\n'
            '    value += 1;\n    puts("before input");\n'
            '    int input = 0;\n    scanf("%d", &input);\n'
            '    printf("value=%d input=%d\\n", value, input);\n    return 0;\n}\n')
        input_file = directory / "input.txt"
        input_file.write_text("123\n")
        executable = directory / "test"
        architecture = ["-arch", "x86_64"] if args.intel else []
        subprocess.run(["/usr/bin/clang", *architecture, "-g", "-O0", str(source), "-o", str(executable)], check=True)
        client = Client(executable, source, input_file)
        try:
            assert client.wait("stopped")["line"] == 3
            client.send("registers")
            assert len(client.wait("registers")["values"]) > 8
            client.send("memory", text="p (int)(value)")
            assert client.wait("memory")["value"] == "42"
            client.send("memory", text="p/x (int)(value)")
            assert client.wait("memory")["value"] == "0x2a"
            client.send("step")
            client.wait("stopped")
            client.send("next")
            client.wait("stopped")
            client.send("breakpoint", line=5, added=True)
            client.send("continue")
            assert client.wait("stopped")["line"] == 5
            client.send("memory", text="p (int)(value)")
            assert client.wait("memory")["value"] == "43"
            client.send("breakpoint", line=5, added=False)
            client.send("command", text="p 20 + 22")
            assert "42" in client.wait("log")["text"]
            client.send("continue")
            assert client.wait("exited")["code"] == 0
            assert "value=43 input=123" in client.output, repr(client.output)
            print("PASS: LLDB entry breakpoint, registers, watches, step/next, source breakpoint, commands, stdin/stdout, exit")
        finally:
            client.close()

        if args.nasm:
            test_nasm(directory)

        source.write_text('volatile int keep_running = 1;\nint main(void) {\n'
                          '    while (keep_running) {}\n    return 0;\n}\n')
        subprocess.run(["/usr/bin/clang", *architecture, "-g", "-O0", str(source), "-o", str(executable)], check=True)
        client = Client(executable, source, input_file)
        try:
            client.wait("stopped")
            client.send("continue")
            client.wait("running")
            client.send("pause")
            client.wait("stopped")
            client.send("command", text="p keep_running = 0")
            client.wait("log")
            client.send("continue")
            assert client.wait("exited")["code"] == 0
            print("PASS: LLDB pause and resume")
        finally:
            client.close()


if __name__ == "__main__":
    main()
