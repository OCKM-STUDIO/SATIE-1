#!/usr/bin/env python3
"""Wire-format compatibility and actionable monitoring status."""
import struct
import sys
import unittest
import contextlib
import io
import json
import tempfile
from unittest.mock import patch, MagicMock
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import fm1log
import satie_bootlog as log


def frame(kind, values):
    return [0x7D, 0x50, kind] + fm1log.pack7(struct.pack(f"<{len(values)}I", *values))


class BootlogTest(unittest.TestCase):
    def test_old_and_current_boot(self):
        for count in (32, 36):
            values = [1 if count == 32 else 2] + list(range(1, count))
            decoded = log.decode(frame(0x5C, values))
            self.assertEqual(decoded["boots"], 7)
            self.assertEqual(len(decoded), count)
        self.assertIsNone(log.decode(frame(0x5C, [0] * 35)))

    def test_fault_registers_and_context(self):
        values = [1] + list(range(1, 40))
        decoded = log.decode_fault(frame(0x5D, values))
        self.assertEqual(decoded["vector"], 3)
        self.assertEqual(decoded["r0"], 17)
        self.assertEqual(decoded["r15"], 32)
        self.assertEqual(decoded["worker_faults"], 33)
        self.assertEqual(decoded["cpu_plus_one"], 39)
        self.assertLessEqual(len(frame(0x5D, values)) + 2, 192)
        self.assertIsNone(log.decode_fault(frame(0x5C, values)))
        self.assertIsNone(log.decode_fault(frame(0x5D, values[:-1])))
        self.assertIsNone(log.decode_fault(frame(0x5D, [2] + values[1:])))

    def test_fallback_cannot_look_healthy(self):
        f = dict(core1_online=1, core1_barred=0, boot_pending=0, boot_failed=0)
        self.assertEqual(log.boot_status(f), "Both audio cores are online.")
        for changes in (dict(core1_online=0), dict(core1_barred=1)):
            self.assertIn("WARNING", log.boot_status({**f, **changes}))
        self.assertIn("pending", log.boot_status({**f, "boot_pending": 1}))

    def test_interrupted_capture_saves_evidence_and_warns(self):
        values = [0] * 36
        values[0], values[7], values[9] = 2, 9, 1
        healthy = log.mido.Message("sysex", data=frame(0x5C, values))
        values[7], values[8], values[9] = 10, 1, 0
        fallback = log.mido.Message("sysex", data=frame(0x5C, values))
        fault = log.mido.Message("sysex", data=frame(0x5D, [1] + [0] * 39))
        fin, fout = MagicMock(), MagicMock()
        fin.__enter__.return_value = fin
        fout.__enter__.return_value = fout
        fin.iter_pending.side_effect = [[healthy, fallback, fault], KeyboardInterrupt]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "capture.json"
            output = io.StringIO()
            with patch.object(sys, "argv", ["bootlog", "--seconds", "1", "--out", str(path)]), \
                 patch.object(log.mido, "get_input_names", return_value=["FM-1 test"]), \
                 patch.object(log.mido, "get_output_names", return_value=["FM-1 test"]), \
                 patch.object(log.mido, "open_input", return_value=fin), \
                 patch.object(log.mido, "open_output", return_value=fout), \
                 contextlib.redirect_stdout(output):
                log.main()
            records = json.loads(path.read_text())
            journal = [json.loads(line) for line in path.with_suffix(".jsonl").read_text().splitlines()]
            self.assertEqual(records, journal)
            self.assertEqual([x.get("kind") for x in records], [None, "boot", "boot", "fault"])
            self.assertIn("restarted", output.getvalue())
            self.assertIn("OFFLINE", output.getvalue())


if __name__ == "__main__":
    unittest.main()
