import subprocess
import tempfile
import time
from contextlib import contextmanager
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent.resolve()


@contextmanager
def iofs_mount(sleep_seconds=1, show_output=False):
    """
    Context manager that sets up temp dirs, spawns iofs-ng, yields the paths, and forcefully cleans up on exit.
    """
    with tempfile.TemporaryDirectory() as real_dir, tempfile.TemporaryDirectory() as fake_dir:
        fake_path = Path(fake_dir)
        real_path = Path(real_dir)

        cmd = [str(REPO_ROOT / "iofs-ng"), "-fd", "-p", str(REPO_ROOT / "plugins/sample.so"),
               "-p", str(REPO_ROOT / "plugins/lastn.so"), "-p", str(REPO_ROOT / "plugins/stats.so"),
               str(fake_path), str(real_path)]
        out_dest = None if show_output else subprocess.DEVNULL
        print(f"\n[FUSE] Spawning: {' '.join(cmd)}")
        process = subprocess.Popen(cmd, cwd=REPO_ROOT, stdout=out_dest, stderr=out_dest)

        try:
            print(f"[FUSE] Waiting {sleep_seconds} seconds for mount...")
            time.sleep(sleep_seconds)

            if process.poll() is not None:
                raise RuntimeError(f"iofs-ng process exited prematurely with code {process.returncode}")

            yield fake_path, real_path

        finally:
            print("\n[FUSE] Tearing down...")
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                print("[FUSE] Process didn't exit, force killing...")
                process.kill()
                process.wait()

            subprocess.run(["fusermount", "-uz", str(fake_path)], check=False, capture_output=True)
