import subprocess
from pathlib import Path
import pytest

REPO_ROOT = Path(__file__).parent.parent.resolve()
E2E_DIR = Path(__file__).parent.resolve()


@pytest.fixture(scope="session", autouse=True)
def build_iofs():
    """Builds the iofs-ng binary and plugins once per test session."""
    build_script = REPO_ROOT / "build.sh"
    subprocess.run(["bash", str(build_script)], cwd=REPO_ROOT, check=True)


@pytest.fixture(scope="session", autouse=True)
def build_tester():
    """Compiles the C E2E tester once per test session."""
    tester_src = E2E_DIR / "tester.c"
    tester_bin = E2E_DIR / "tester"
    subprocess.run(
        ["gcc", "-Wall", "-Wextra", "-O2", str(tester_src), "-o", str(tester_bin)],
        check=True
    )
