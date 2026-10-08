"""
tableramp.py - write GigaMoog420 table ramps and amplitude-correction files for Chimera.

Files written here are read by Chimera's GigaMoog420 script commands:

    @mylabel tableramp <ch> C:/Chimera/Chimera-Cryo/Ramps/mylabel.csv
    correction mylabel C:/Chimera/Chimera-Cryo/Ramps/mylabel_gain.csv

(paths must not contain spaces). Formats, matching synth::read_numeric_table / synth::TableRamp and
gm420::AmplitudeCorrection in Chimera/Source/GigaMOOG:

    table ramp:   tau_s, frequency_MHz, amplitude_percent [, phase_deg]
                  tau from the start of the block, starting at 0 and increasing by >= 1 clock tick per
                  row. Row i plays from tau[i] to tau[i+1]; the last row only marks the end and sets the
                  output that continues after the block.
    correction:   tau_s, gain
                  same tau axis; each table row's amplitude is multiplied by the gain over its window.

Usage: run this file to write the example (a simultaneous linear frequency + amplitude ramp), or
import make_table / write_tableramp / write_correction from your own optimization code.
"""

import os
import numpy as np

SEQ_CLOCK_HZ = 153.6e6               # sequencer tick rate; every row boundary falls on a tick
TICK_S = 1.0 / SEQ_CLOCK_HZ          # ~6.51 ns
MAX_FREQUENCY_MHZ = 307.2
MAX_ROWS = 16384                     # rows per channel in the box's table (synth::MAX_LENGTH)

RAMPS_DIR = os.path.dirname(os.path.abspath(__file__))


def make_table(duration_s, n_steps, freq_fn, amp_fn, phase_fn=None, sample="mid"):
    """
    Build a table ramp from functions of time.

    duration_s : total ramp length in seconds.
    n_steps    : number of held steps (the table gets n_steps + 1 rows, the last being the end marker).
    freq_fn    : t (numpy array, s) -> frequency in MHz.
    amp_fn     : t -> amplitude in percent (0-100).
    phase_fn   : optional t -> phase in degrees.
    sample     : "mid" evaluates each step at the middle of its interval, which for smooth ramps keeps
                 the held value equal to the interval average (best for phase accuracy of frequency
                 ramps); "start" evaluates at the start of each interval.

    Row boundaries are snapped to whole clock ticks, so the values are computed for exactly the
    intervals the hardware plays. Returns a dict of numpy arrays: tau, freq, amp and (optionally) phase.
    """
    if n_steps < 1:
        raise ValueError("n_steps must be >= 1")
    ticks = np.round(np.linspace(0.0, duration_s, n_steps + 1) / TICK_S)
    if np.any(np.diff(ticks) < 1):
        raise ValueError(f"steps shorter than one clock tick ({TICK_S * 1e9:.2f} ns): "
                         f"use fewer than {int(duration_s / TICK_S)} steps for {duration_s} s")
    tau = ticks * TICK_S

    if sample == "mid":
        t_steps = 0.5 * (tau[:-1] + tau[1:])
    elif sample == "start":
        t_steps = tau[:-1]
    else:
        raise ValueError('sample must be "mid" or "start"')
    t_eval = np.append(t_steps, tau[-1])   # the end-marker row takes the value at the very end

    table = {
        "tau": tau,
        "freq": np.broadcast_to(np.asarray(freq_fn(t_eval), dtype=float), t_eval.shape).copy(),
        "amp": np.broadcast_to(np.asarray(amp_fn(t_eval), dtype=float), t_eval.shape).copy(),
    }
    if phase_fn is not None:
        table["phase"] = np.broadcast_to(np.asarray(phase_fn(t_eval), dtype=float), t_eval.shape).copy()
    return table


def _check_table(tau, freq, amp, phase):
    n = len(tau)
    if n < 2:
        raise ValueError("a table ramp needs at least two rows (the last one marks the end)")
    if n > MAX_ROWS:
        raise ValueError(f"{n} rows exceeds the box's table depth of {MAX_ROWS}")
    if any(len(a) != n for a in (freq, amp) + ((phase,) if phase is not None else ())):
        raise ValueError("tau, freq, amp (and phase) must have the same length")
    if tau[0] != 0.0:
        raise ValueError("tau must start at 0 (time from the start of the block)")
    if np.any(np.diff(tau) < TICK_S * (1 - 1e-9)):
        raise ValueError(f"tau must increase by at least one clock tick ({TICK_S * 1e9:.2f} ns) per row")
    if np.any(freq < 0) or np.any(freq > MAX_FREQUENCY_MHZ):
        raise ValueError(f"frequencies must be within 0-{MAX_FREQUENCY_MHZ} MHz")
    if np.any(amp < 0) or np.any(amp > 100):
        raise ValueError("amplitudes must be within 0-100 %")
    if not all(np.all(np.isfinite(a)) for a in (tau, freq, amp) + ((phase,) if phase is not None else ())):
        raise ValueError("table contains NaN or inf")


def write_tableramp(path, tau, freq, amp, phase=None):
    """Write a table ramp file. tau in s, freq in MHz, amp in %, phase in degrees (optional)."""
    tau, freq, amp = (np.asarray(a, dtype=float) for a in (tau, freq, amp))
    phase = None if phase is None else np.asarray(phase, dtype=float)
    _check_table(tau, freq, amp, phase)
    columns = [tau, freq, amp] + ([phase] if phase is not None else [])
    header = "tau_s, frequency_MHz, amplitude_percent" + (", phase_deg" if phase is not None else "")
    np.savetxt(path, np.column_stack(columns), delimiter=", ", fmt="%.17g", header=header)
    return path


def write_correction(path, tau, gain):
    """Write an amplitude-correction file. tau in s from the block start, gain >= 0."""
    tau, gain = np.asarray(tau, dtype=float), np.asarray(gain, dtype=float)
    if len(tau) != len(gain) or len(tau) < 2:
        raise ValueError("tau and gain must have the same length (at least 2)")
    if np.any(np.diff(tau) <= 0):
        raise ValueError("tau must be strictly increasing")
    if np.any(gain < 0) or not np.all(np.isfinite(gain)) or not np.all(np.isfinite(tau)):
        raise ValueError("gains must be finite and >= 0")
    np.savetxt(path, np.column_stack([tau, gain]), delimiter=", ", fmt="%.17g", header="tau_s, gain")
    return path


def script_lines(label, channel, table_path, correction_path=None):
    """GigaMoog420 script lines that play a table (and apply a correction). Paths use forward slashes."""
    table_path = table_path.replace("\\", "/")
    if " " in table_path or (correction_path and " " in correction_path):
        raise ValueError("Chimera script paths must not contain spaces")
    lines = [f"@{label} tableramp {channel} {table_path}"]
    if correction_path is not None:
        lines.append(f"correction {label} {correction_path.replace(chr(92), '/')}")
    return "\n".join(lines)


if __name__ == "__main__":
    # Example: 1 ms ramp, frequency 80 -> 90 MHz and amplitude 20 -> 60 % at the same time, both linear.
    label = "linear_fa_example"
    duration = 1e-3            # s
    n_steps = 1000             # 1 us per step
    f0, f1 = 80.0, 90.0        # MHz
    a0, a1 = 20.0, 60.0        # %

    table = make_table(
        duration, n_steps,
        freq_fn=lambda t: f0 + (f1 - f0) * t / duration,
        amp_fn=lambda t: a0 + (a1 - a0) * t / duration,
    )
    table_path = write_tableramp(os.path.join(RAMPS_DIR, f"{label}.csv"),
                                 table["tau"], table["freq"], table["amp"])

    # Unity gain on the same tau grid: a no-op correction to start the feedback loop from.
    gain_path = write_correction(os.path.join(RAMPS_DIR, f"{label}_gain.csv"),
                                 table["tau"], np.ones_like(table["tau"]))

    print(f"wrote {table_path} ({len(table['tau'])} rows)")
    print(f"wrote {gain_path}")
    print("\nGigaMoog420 script:\n")
    print("set 0 0 80 0 1")
    print(script_lines(label, 0, table_path, gain_path))
