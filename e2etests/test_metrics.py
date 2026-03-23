import os
import re
import requests
from utils import iofs_mount


def test_metrics_endpoint_is_alive():
    """
    Sanity check to ensure the FUSE filesystem is up and the embedded HTTP server is serving metrics on port 9090.
    """
    with iofs_mount(show_output=False) as (fake_dir, real_dir):
        assert fake_dir.exists()
        assert real_dir.exists()
        url = "http://localhost:9090/metrics"
        response = requests.get(url, timeout=2)
        assert response.status_code == 200
        text = response.text
        assert "application_info" in text
        assert "iofs-ng" in text
        assert "exporter_plugin_info{name=\"StatsPlugin\"" in text


def parse_prometheus_metrics(text):
    """
    Parses Prometheus text output into a flat dictionary.
    Example: 'iofs_ops_total{op="write"} 5' -> {'iofs_ops_total{op="write"}': 5}
    """
    metrics = {}
    for line in text.strip().split('\n'):
        line = line.strip()
        # Ignore comments and empty lines
        if not line or line.startswith('#'):
            continue

        # Match metric name/labels and the value
        # e.g., iofs_ops_total{op="write"} 12
        m = re.match(r'^(.*?)\s+([0-9\.]+)$', line)
        if m:
            key, val = m.groups()
            metrics[key] = float(val)
    return metrics


def get_metrics():
    """Fetches and parses metrics from the local iofs-ng server."""
    resp = requests.get("http://localhost:9090/metrics", timeout=2)
    resp.raise_for_status()
    return parse_prometheus_metrics(resp.text)

def test_prometheus_metrics_with_noise():
    """
    Tests that the StatsPlugin correctly records metrics
    """
    with iofs_mount(show_output=False) as (fake_dir, real_dir):

        # 1. Get the baseline metrics BEFORE we do our test
        pre_metrics = get_metrics()

        # 2. Perform our controlled operations
        # We will write exactly 65536 bytes (falls exactly on the bounds of bucket 4)
        payload_size = 65536
        payload = b"A" * payload_size
        test_file = fake_dir / "target_file.dat"

        # tried to do as low level IO as python gets to make it more predictable
        fd_w = os.open(test_file, os.O_CREAT | os.O_WRONLY, 0o644)
        bytes_written = os.write(fd_w, payload)
        os.close(fd_w)

        assert bytes_written == payload_size, f"os.write only wrote {bytes_written} bytes"

        # open with O_RDONLY
        fd_r = os.open(test_file, os.O_RDONLY)
        read_back = os.read(fd_r, payload_size)
        os.close(fd_r)

        assert len(read_back) == payload_size, "os.read did not return the full payload"

        # 3. Get the metrics AFTER our operations
        post_metrics = get_metrics()

        # Assertions:
        # A. Strict check: Writes.
        # The delta should be exactly 1.
        pre_writes = pre_metrics.get('iofs_ops_total{op="write"}', 0)
        post_writes = post_metrics.get('iofs_ops_total{op="write"}', 0)
        assert post_writes - pre_writes == 1, "Expected exactly 1 write operation"

        # B. Strict check: Histogram Buckets.
        # Prometheus histograms are cumulative. For a write of size X:
        # - Any bucket with le < X should NOT increment (delta = 0)
        # - Any bucket with le >= X MUST increment (delta = 1)
        # Hardcoded from now, stolen from the plugin...
        buckets = [4096, 8192, 16384, 65536, 131072, 262144]

        for b in buckets:
            pre_b = pre_metrics.get(f'iofs_io_bytes_bucket{{op="write",le="{b}"}}', 0)
            post_b = post_metrics.get(f'iofs_io_bytes_bucket{{op="write",le="{b}"}}', 0)
            delta = post_b - pre_b

            if b < payload_size:
                assert delta == 0, f"Bucket le='{b}' should NOT increment for {payload_size} byte write"
            else:
                assert delta == 1, f"Bucket le='{b}' MUST increment for {payload_size} byte write"

        # Check the catch-all +Inf bucket
        pre_inf = pre_metrics.get('iofs_io_bytes_bucket{op="write",le="+Inf"}', 0)
        post_inf = post_metrics.get('iofs_io_bytes_bucket{op="write",le="+Inf"}', 0)
        assert post_inf - pre_inf == 1, "Bucket le='+Inf' MUST increment"

        # C. Strict check: Write Bytes Sum.
        pre_sum = pre_metrics.get('iofs_io_bytes_sum{op="write"}', 0)
        post_sum = post_metrics.get('iofs_io_bytes_sum{op="write"}', 0)
        assert post_sum - pre_sum == payload_size, f"Write sum should increase by exactly {payload_size} bytes"

        # D. Fuzzy check: Reads.
        # Background indexers might have snooped on our file, so we expect AT LEAST 1 read.
        pre_reads = pre_metrics.get('iofs_ops_total{op="read"}', 0)
        post_reads = post_metrics.get('iofs_ops_total{op="read"}', 0)
        assert post_reads - pre_reads >= 1, "Expected at least 1 read operation"
