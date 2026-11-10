#!/usr/bin/env python3
"""Read-only SATIE diagnostic capture; reconnect after a device restart."""
import argparse
import json
import struct
import time
from pathlib import Path

import mido
import fm1log

FIELDS = "schema ms p3_reset reset_source watchdog boot_failed boot_pending boots core1_barred core1_online timeouts rejected core1_faults core1_pc core1_debug core1_emu crashes crash_pc crash_debug crash_emu last_magic last_ticks last_timeouts last_rejected last_request last_complete last_faults last_pc last_debug last_emu last_function last_context".split()
FAULT_FIELDS = (
    "schema magic count vector pc rets emu debug sp psr icfg uptime_ticks_ms etm0 etm1 etm2 etm3 early".split()
    + [f"r{i}" for i in range(16)]
    + "worker_faults worker_pc worker_debug worker_emu raw_debug raw_emu cpu_plus_one".split()
)


def decode(data):
    if list(data[:3]) != [0x7D, 0x50, 0x5C]:
        return None
    raw = bytes(fm1log.unpack7(data[3:]))
    if len(raw) not in (128, 144):
        return None
    fields = FIELDS + ["previous_audio_stage", "previous_in_audio", "audio_stage", "in_audio"]
    return dict(zip(fields, struct.unpack("<%dI" % (len(raw) // 4), raw)))


def decode_fault(data):
    if list(data[:3]) != [0x7D, 0x50, 0x5D]:
        return None
    raw = bytes(fm1log.unpack7(data[3:]))
    if len(raw) != 160:
        return None
    values = struct.unpack("<40I", raw)
    if values[0] != 1:
        return None
    return dict(zip(FAULT_FIELDS, values))


def boot_status(frame):
    if not frame["core1_online"] or frame["core1_barred"]:
        return "WARNING: second audio core is OFFLINE; this is not a normal playback test."
    if frame["boot_pending"] or frame["boot_failed"]:
        return "Startup check still pending; keep connected."
    return "Both audio cores are online."


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=float, default=45)
    parser.add_argument("--out", default="build/satie/boot-diagnostics.json")
    args = parser.parse_args()
    records = []
    end = time.monotonic() + args.seconds
    previous = None
    previous_fault = None
    previous_status = None
    path = Path(args.out)
    path.parent.mkdir(parents=True, exist_ok=True)
    # Stream a second copy so an interrupted computer/session cannot lose hours
    # of evidence. The familiar JSON array is still written on exit or Ctrl+C.
    journal_path = path.with_suffix(".jsonl")
    journal = journal_path.open("w", buffering=1)
    def record(item):
        records.append(item)
        journal.write(json.dumps(item) + "\n")
    print(f"Recording to {journal_path}; Ctrl+C saves {path}", flush=True)
    try:
        while time.monotonic() < end:
            pick = lambda names: next((n for n in names if "fm-1" in n.lower()), None)
            pin, pout = pick(mido.get_input_names()), pick(mido.get_output_names())
            if not pin or not pout:
                time.sleep(0.25)
                continue
            try:
                with mido.open_input(pin) as fin, mido.open_output(pout) as fout:
                    record({"host_time": time.time(), "event": "connected"})
                    sent, seen, request = 0.0, time.monotonic(), 0
                    while time.monotonic() < end and time.monotonic() - seen < 4:
                        if time.monotonic() - sent >= 1:
                            # Only status requests: telemetry, boot, retained exception.
                            # Command 4 switches telemetry off on older builds; the
                            # next scheduled request switches it on again.
                            command = (1, 3, 4)[request % 3]
                            fout.send(mido.Message("sysex", data=[0x7D, 0x50, 0x46, command]))
                            request += 1
                            sent = time.monotonic()
                        for msg in fin.iter_pending():
                            if msg.type != "sysex":
                                continue
                            boot = decode(msg.data)
                            fault = decode_fault(msg.data)
                            frame = boot or fault or fm1log.decode(msg.data)
                            if frame is None:
                                continue
                            seen = time.monotonic()
                            record({"host_time": time.time(), "kind": "boot" if boot else "fault" if fault else "audio", **frame})
                            if boot:
                                key = {k: v for k, v in boot.items() if k not in ("ms", "audio_stage", "in_audio")}
                                if key != previous:
                                    print(json.dumps(boot), flush=True)
                                    if previous and boot["boots"] != previous["boots"]:
                                        print("WARNING: FM-1 restarted during this recording.", flush=True)
                                    previous = key
                                status = boot_status(boot)
                                if status != previous_status:
                                    print(status, flush=True)
                                    previous_status = status
                            if fault and fault != previous_fault:
                                print("Retained exception: " + json.dumps(fault), flush=True)
                                previous_fault = fault
                        time.sleep(0.02)
            except (OSError, RuntimeError) as exc:
                record({"host_time": time.time(), "event": str(exc)})
                time.sleep(0.25)
    except KeyboardInterrupt:
        pass
    finally:
        journal.close()
        path.write_text(json.dumps(records, indent=2))
        print(f"Saved {len(records)} records to {path}")


if __name__ == "__main__":
    main()
