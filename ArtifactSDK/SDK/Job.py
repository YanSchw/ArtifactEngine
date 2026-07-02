"""
Job — a small cross-platform terminal task runner for the Artifact SDK.

Each high-level step the SDK performs (generating project files, building,
cooking, packaging, ...) can be wrapped in a `Job`. While the job runs it shows
a braille spinner plus a live tail of the step's log output:

    ⠹ Building...  1.4s
      [last few lines of output, dimmed]

When the job finishes successfully the whole live region is erased and replaced
by a single summary line with timing information:

    ✔ Building  3.2s

If the job fails, the spinner region is erased, a red summary is printed, and
the full captured log is dumped so the error is still visible.

Everything is plain ANSI + colorama, so it works in bash, zsh, cmd.exe and
PowerShell. When stdout is not a TTY (CI, pipes, redirected files) the job
degrades gracefully to simple line-by-line logging with no cursor tricks.
"""

import os
import re
import sys
import time
import shutil
import threading
import subprocess

import colorama
from colorama import Fore, Style

# Braille "dots" spinner — the classic 10-frame cycle used by ng/npm/etc.
SPINNER_FRAMES = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
FRAME_INTERVAL = 0.08  # seconds between spinner frames

# ANSI control sequences.
HIDE_CURSOR = "\033[?25l"
SHOW_CURSOR = "\033[?25h"
CLEAR_TO_END = "\033[0J"   # erase from cursor to end of screen

# Encoding used when we re-open the terminal / capture pipe as text streams.
STDOUT_ENCODING = getattr(sys.__stdout__, "encoding", None) or "utf-8"

# Ensure Windows consoles (cmd.exe / PowerShell) interpret ANSI escapes and
# colour codes. Safe and idempotent on every platform.
colorama.just_fix_windows_console()


def format_duration(seconds: float) -> str:
    """Human-friendly elapsed time, e.g. '0.4s', '3.2s', '1m 05s'."""
    if seconds < 60.0:
        return f"{seconds:.1f}s"
    minutes = int(seconds // 60)
    rest = int(round(seconds - minutes * 60))
    return f"{minutes}m {rest:02d}s"


def _truncate_visible(text: str, width: int) -> str:
    """Truncate to at most ``width`` *visible* columns, keeping ANSI escapes.

    Escape sequences don't count toward the width, and a reset is appended when
    the text is cut so a half-applied colour can't bleed onto the rest of the
    line. This guarantees every rendered line occupies exactly one terminal row,
    which is what keeps the in-place redraw/erase math exact.
    """
    if width <= 0:
        return ""
    out = []
    count = 0
    i = 0
    truncated = False
    while i < len(text):
        ch = text[i]
        if ch == "\033":
            j = i + 1
            while j < len(text) and not text[j].isalpha():
                j += 1
            j += 1
            out.append(text[i:j])
            i = j
            continue
        if count >= width:
            truncated = True
            break
        out.append(ch)
        count += 1
        i += 1
    result = "".join(out)
    if truncated:
        result += Style.RESET_ALL
    return result


class JobError(Exception):
    """Intentional job failure (e.g. a build error or a non-zero subprocess).

    Carries an exit code so the CLI can terminate with it. Raising this (rather
    than an arbitrary exception) tells the Job the failure is expected, so it
    shows a clean red summary instead of a Python traceback.
    """

    def __init__(self, message: str = "", returncode: int = 1):
        super().__init__(message or f"Job failed with return code {returncode}")
        self.returncode = returncode


class Job:
    """Context manager that renders a single SDK step as a live spinner.

    Usage:
        with Job("Building") as job:
            job.log("compiling foo.cpp")
            job.run(["cmake", "--build", "Build"])   # streamed into the tail

    Inside the `with` block ``sys.stdout`` / ``sys.stderr`` are redirected into
    the job, so ordinary ``print()`` calls made by the step are captured too.
    """

    # The currently active job (so nested helpers can find it). Jobs are not
    # meant to nest visually; the outer one owns the terminal.
    _active = None

    def __init__(self, title: str, max_tail: int = 8, dump_on_error: bool = True):
        self.title = title
        self.max_tail = max_tail
        self.dump_on_error = dump_on_error

        self._logs: list[str] = []
        self._persist: list[str] = []
        self._lock = threading.Lock()
        self._tty = sys.__stdout__
        self._interactive = bool(self._tty) and self._tty.isatty() and not os.environ.get("ARTIFACT_NO_SPINNER")

        self._frame = 0
        self._drawn_rows = 0
        self._start = 0.0
        self._stop = threading.Event()
        self._thread = None

        # Optional determinate progress: (current, total). When set, the spinner
        # line shows a percentage and a remaining-time estimate.
        self._progress = None

        # Non-interactive fallback: saved Python-level streams while proxied.
        self._saved_stdout = None
        self._saved_stderr = None

        # Interactive fd-level capture state (see _install_capture).
        self._saved_fd_stdout = None
        self._saved_fd_stderr = None
        self._reader = None
        self._captured = False

    # -- public API --------------------------------------------------------

    def log(self, message: str):
        """Append one or more lines of output to the job."""
        if message is None:
            return
        text = message.rstrip("\n")
        if text == "" and not message.endswith("\n"):
            return
        # Normalise control characters that would throw off width/row math.
        text = text.replace("\r", "").replace("\t", "    ")
        with self._lock:
            for line in text.split("\n"):
                self._logs.append(line)
            if not self._interactive:
                # Non-interactive: echo immediately, indented under the step.
                for line in text.split("\n"):
                    self._tty.write(f"   {line}\n")
                self._tty.flush()

    def set_progress(self, current: int, total: int):
        """Report determinate progress for the spinner (e.g. ninja's [N/M]).

        Enables a progress bar, percentage and an estimated time remaining,
        derived from the average time taken per completed unit so far.
        """
        if total <= 0:
            return
        with self._lock:
            self._progress = (max(0, min(current, total)), total)

    def persist(self, message: str):
        """Queue a message to print *after* the live region is cleared.

        Use this for output that must remain on screen once the job ends
        (e.g. build warnings, the final 'Build succeeded' line) rather than
        vanishing with the ephemeral log tail.
        """
        if message is None:
            return
        with self._lock:
            self._persist.append(message.rstrip("\n"))

    def fail(self, message: str = None, returncode: int = 1):
        """Fail the job: queue an optional message and raise ``JobError``.

        The surrounding ``with Job(...)`` block tears down the spinner and shows
        a red summary; the ``JobError`` propagates so the CLI can exit with
        ``returncode``. This is the generic version of a build failure.
        """
        if message:
            self.persist(message)
        raise JobError(message or "", returncode)

    def run(self, command, check: bool = False, progress_pattern=None, **kwargs) -> int:
        """Run a subprocess, streaming its combined output into the job tail.

        Returns the process exit code. When ``check`` is True a non-zero exit
        fails the job (raises ``JobError``). When ``progress_pattern`` is given
        (a regex with two numeric groups, e.g. ``r"\\[(\\d+)/(\\d+)\\]"``), each
        output line is searched for ``current``/``total`` to drive the spinner's
        progress bar and ETA. Any other keyword arguments are forwarded to
        ``subprocess.Popen`` (e.g. ``cwd``).
        """
        pattern = re.compile(progress_pattern) if isinstance(progress_pattern, str) else progress_pattern
        proc = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            **kwargs,
        )
        for line in proc.stdout:
            if pattern is not None:
                match = pattern.search(line)
                if match:
                    self.set_progress(int(match.group(1)), int(match.group(2)))
            self.log(line)
        proc.wait()
        if check and proc.returncode != 0:
            self.fail(returncode=proc.returncode)
        return proc.returncode

    # -- context manager ---------------------------------------------------

    def __enter__(self):
        Job._active = self
        self._start = time.monotonic()

        if self._interactive:
            # Capture everything the step emits — including child processes that
            # write straight to the stdout/stderr file descriptors — so nothing
            # leaks onto the spinner. Rendering goes to a private dup of the tty.
            self._install_capture()
            self._tty.write(HIDE_CURSOR)
            self._tty.flush()
            self._thread = threading.Thread(target=self._spin, daemon=True)
            self._thread.start()
        else:
            # Non-interactive: just funnel Python-level prints through the job.
            self._saved_stdout = sys.stdout
            self._saved_stderr = sys.stderr
            sys.stdout = _StreamProxy(self)
            sys.stderr = _StreamProxy(self)
            self._tty.write(f"{Fore.CYAN}>{Style.RESET_ALL} {self.title}...\n")
            self._tty.flush()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        Job._active = None

        if self._interactive:
            self._stop.set()
            if self._thread:
                self._thread.join()
            # Restore the fds and drain any remaining captured output.
            self._remove_capture()
            with self._lock:
                self._erase()
        else:
            sys.stdout = self._saved_stdout
            sys.stderr = self._saved_stderr

        elapsed = format_duration(time.monotonic() - self._start)

        if exc_type is None:
            self._print_summary(f"{Fore.GREEN}✔{Style.RESET_ALL} {self.title}  "
                                f"{Style.DIM}{elapsed}{Style.RESET_ALL}")
            self._flush_persisted()
        else:
            label = "cancelled" if exc_type is KeyboardInterrupt else "failed"
            self._print_summary(f"{Fore.RED}✖{Style.RESET_ALL} {self.title} {label}  "
                                f"{Style.DIM}{elapsed}{Style.RESET_ALL}")
            self._flush_persisted()
            if self.dump_on_error and self._interactive:
                self._dump_logs()

        if self._interactive:
            self._tty.write(SHOW_CURSOR)
            self._tty.flush()
            self._tty.close()  # closes our private dup of the terminal

        # Do not suppress the exception — let callers handle/propagate it.
        return False

    # -- fd-level capture --------------------------------------------------

    def _install_capture(self):
        """Redirect the stdout/stderr fds into a pipe drained by the job.

        A private dup of the original stdout becomes ``self._tty`` (used only
        for rendering), while fds 1 and 2 are pointed at a pipe whose reader
        thread feeds every byte — prints and child-process output alike — into
        the job's log tail.
        """
        render_fd = os.dup(1)
        self._tty = os.fdopen(render_fd, "w", encoding=STDOUT_ENCODING, errors="replace")

        self._saved_fd_stdout = os.dup(1)
        self._saved_fd_stderr = os.dup(2)

        read_fd, write_fd = os.pipe()
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(write_fd, 1)
        os.dup2(write_fd, 2)
        os.close(write_fd)
        self._captured = True

        self._reader = threading.Thread(target=self._drain, args=(read_fd,), daemon=True)
        self._reader.start()

    def _drain(self, read_fd):
        with os.fdopen(read_fd, "r", encoding=STDOUT_ENCODING, errors="replace") as pipe:
            for line in pipe:
                self.log(line)

    def _remove_capture(self):
        if not self._captured:
            return
        sys.stdout.flush()
        sys.stderr.flush()
        # Restoring the fds closes the pipe's write ends, so the reader hits EOF.
        os.dup2(self._saved_fd_stdout, 1)
        os.dup2(self._saved_fd_stderr, 2)
        os.close(self._saved_fd_stdout)
        os.close(self._saved_fd_stderr)
        self._captured = False
        if self._reader:
            self._reader.join()

    # -- rendering ---------------------------------------------------------

    def _spin(self):
        while not self._stop.is_set():
            with self._lock:
                self._draw()
            self._frame += 1
            self._stop.wait(FRAME_INTERVAL)

    def _compose(self) -> list[str]:
        # Leave a one-column margin so a full-width line can't trigger the
        # terminal's auto-wrap; that keeps every line at exactly one row.
        width = max(1, shutil.get_terminal_size((80, 24)).columns - 1)
        lines = []
        tail = self._logs[-self.max_tail:]
        for line in tail:
            lines.append(_truncate_visible(f"  {Style.DIM}{line}{Style.RESET_ALL}", width))
        spinner = SPINNER_FRAMES[self._frame % len(SPINNER_FRAMES)]
        elapsed = time.monotonic() - self._start
        status = f"{Fore.CYAN}{spinner}{Style.RESET_ALL} {self.title}"
        if self._progress is not None:
            status += "  " + self._progress_segment(elapsed)
        else:
            status += "..."
        status += f"  {Style.DIM}{format_duration(elapsed)}{Style.RESET_ALL}"
        lines.append(_truncate_visible(status, width))
        return lines

    def _progress_segment(self, elapsed: float) -> str:
        current, total = self._progress
        fraction = current / total if total else 0.0

        percent = f"{Fore.GREEN}{int(fraction * 100):3d}%{Style.RESET_ALL}"

        # Estimate remaining time from the average duration per completed unit.
        if current > 0 and current < total:
            eta = elapsed * (total - current) / current
            remaining = f"{Style.DIM}~{format_duration(eta)} left{Style.RESET_ALL}"
        else:
            remaining = ""

        segment = f"{percent} {Style.DIM}[{current}/{total}]{Style.RESET_ALL}"
        if remaining:
            segment += f"  {remaining}"
        return segment

    def _draw(self):
        lines = self._compose()
        self._erase()
        self._tty.write("\n".join(lines))
        self._tty.flush()
        # Each composed line is truncated to one terminal row, so the number of
        # rows we drew equals the number of lines — making erase exact.
        self._drawn_rows = len(lines)

    def _erase(self):
        if self._drawn_rows <= 0:
            return
        self._tty.write("\r")
        if self._drawn_rows > 1:
            self._tty.write(f"\033[{self._drawn_rows - 1}A")
        self._tty.write(CLEAR_TO_END)
        self._tty.flush()
        self._drawn_rows = 0

    def _print_summary(self, text: str):
        self._tty.write(text + "\n")
        self._tty.flush()

    def _flush_persisted(self):
        with self._lock:
            for line in self._persist:
                self._tty.write(line + "\n")
            self._persist.clear()
        self._tty.flush()

    def _dump_logs(self):
        with self._lock:
            for line in self._logs:
                self._tty.write(f"  {line}\n")
        self._tty.flush()


class _StreamProxy:
    """File-like object that funnels writes from print() into a Job."""

    def __init__(self, job: "Job"):
        self._job = job
        self._buffer = ""

    def write(self, data: str):
        if not data:
            return 0
        self._buffer += data
        while "\n" in self._buffer:
            line, self._buffer = self._buffer.split("\n", 1)
            self._job.log(line)
        return len(data)

    def flush(self):
        if self._buffer:
            self._job.log(self._buffer)
            self._buffer = ""

    def isatty(self):
        return False
