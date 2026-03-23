import os
import subprocess
import sys
from pathlib import Path

from utils import iofs_mount

E2E_DIR = Path(__file__).parent.resolve()


def test_c_level_posix_compliance():
    tester_bin = E2E_DIR / "tester"
    assert tester_bin.exists()
    with iofs_mount(show_output=False) as (fake_dir, real_dir):
        print(f"running C {str(tester_bin)}")
        result = subprocess.run([str(tester_bin), str(fake_dir), str(real_dir)], check=False)
        assert result.returncode == 0


def test_build_linux_kernel():
    """
    Stress-tests the FUSE filesystem by cloning and compiling the Linux kernel.

    Assumes the following packages are already installed on the host (deb13):
    # sudo apt update && sudo apt install build-essential libncurses-dev bison flex libssl-dev libelf-dev bc -y
    """
    # Be careful enabling the debug fuse output... its a lot
    with iofs_mount(show_output=False) as (fake_dir, real_dir):
        work_dir = fake_dir
        print(f"\n[TEST] Cloning Linux kernel into {work_dir}...")
        subprocess.run(
            ["git", "clone", "--depth", "1", "https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"],
            cwd=work_dir, check=True, stdout=sys.stdout, stderr=sys.stderr
        )
        linux_src_dir = work_dir / "linux"
        assert linux_src_dir.exists()
        print("\nConfiguring kernel (make defconfig)...")
        subprocess.run(["make", "defconfig"], cwd=linux_src_dir, check=True, stdout=sys.stdout, stderr=sys.stderr)
        cores = str(os.cpu_count() or 4)
        print(f"\n[TEST] Compiling kernel with -j{cores}.")
        subprocess.run(["make", f"-j{cores}"], cwd=linux_src_dir, check=True, stdout=sys.stdout, stderr=sys.stderr)
        print("\n[TEST] Kernel successfully compiled on the FUSE mount!")
