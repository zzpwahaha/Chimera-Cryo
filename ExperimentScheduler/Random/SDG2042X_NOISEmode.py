"""
Siglent SDG2042X Noise Controller Class

Requirements:
    pip install pyvisa pyvisa-py
"""

import pyvisa


class SDG2042XNoise:
    """
    Controller for Siglent SDG2042X noise output.
    """

    def __init__(
        self,
        resource,
        channel=1,
        output_load="HZ",
        timeout=5000,
    ):
        """
        Parameters
        ----------
        resource : str
            VISA resource string

        channel : int
            Output channel (1 or 2)

        output_load : str
            "50" or "HZ"

        timeout : int
            VISA timeout in ms
        """

        self.resource = resource
        self.channel = channel
        self.output_load = output_load

        self.rm = pyvisa.ResourceManager()
        self.inst = self.rm.open_resource(resource)

        self.inst.timeout = timeout

        print(self.query("*IDN?"))

    # ============================================================
    # INTERNAL HELPERS
    # ============================================================

    @property
    def ch(self):
        return f"C{self.channel}"

    def write(self, cmd):
        self.inst.write(cmd)

    def query(self, cmd):
        return self.inst.query(cmd)

    # ============================================================
    # OUTPUT CONTROL
    # ============================================================

    def output_on(self):
        self.write(f"{self.ch}:OUTP ON")

    def output_off(self):
        self.write(f"{self.ch}:OUTP OFF")

    def set_load(self, load=None):
        """
        Set output load.

        Parameters
        ----------
        load : str
            "50" or "HZ"
        """

        if load is not None:
            self.output_load = load

        self.write(f"{self.ch}:OUTP LOAD,{self.output_load}")

    # ============================================================
    # NOISE CONTROL
    # ============================================================

    def enable_noise(self, std_rms=0.1, mean=0.0):
        """
        Configure Gaussian noise output.

        Parameters
        ----------
        std_rms : float
            Noise standard deviation (Vrms)

        mean : float
            Mean voltage / DC offset
        """

        self.write(f"{self.ch}:BSWV WVTP,NOISE")

        self.set_std(std_rms)
        self.set_mean(mean)

        self.set_load()

        self.output_on()

    def set_std(self, std_rms):
        """
        Update noise standard deviation.

        If std_rms <= 0:
            switches to DC mode automatically.
        """

        if std_rms <= 0:
            current_mean = self.get_mean()

            self.write(f"{self.ch}:BSWV WVTP,DC")
            self.write(f"{self.ch}:BSWV OFST,{current_mean}")

            return

        # Ensure waveform is NOISE
        self.write(f"{self.ch}:BSWV WVTP,NOISE")

        # SDG uses STDEV in NOISE mode
        self.write(f"{self.ch}:BSWV STDEV,{std_rms}")

    def set_mean(self, mean):
        """
        Update noise mean / DC offset.
        """

        waveform = self.get_waveform_type()

        if waveform == "DC":
            self.write(f"{self.ch}:BSWV OFST,{mean}")
        else:
            self.write(f"{self.ch}:BSWV MEAN,{mean}")

    # ============================================================
    # SINE MODE
    # ============================================================

    def enable_sine(
        self,
        frequency,
        amplitude,
        offset=0.0,
        phase=0.0,
    ):
        """
        Configure sine output.

        Parameters
        ----------
        frequency : float
            Frequency in Hz

        amplitude : float
            Peak-to-peak amplitude in Volts

        offset : float
            DC offset in Volts

        phase : float
            Phase in degrees
        """

        self.write(
            f"{self.ch}:BSWV "
            f"WVTP,SINE,"
            f"FRQ,{frequency},"
            f"AMP,{amplitude},"
            f"OFST,{offset},"
            f"PHSE,{phase}"
        )

        self.set_load()
        self.output_on()

    def set_sine_frequency(self, frequency):
        """
        Update sine frequency in Hz.
        """

        self.write(f"{self.ch}:BSWV FRQ,{frequency}")

    def set_sine_amplitude(self, amplitude):
        """
        Update sine amplitude in Vpp.
        """

        self.write(f"{self.ch}:BSWV AMP,{amplitude}")

    def set_sine_offset(self, offset):
        """
        Update sine DC offset.
        """

        self.write(f"{self.ch}:BSWV OFST,{offset}")

    def set_sine_phase(self, phase_deg):
        """
        Update sine phase in degrees.
        """

        self.write(f"{self.ch}:BSWV PHSE,{phase_deg}")

    # ============================================================
    # GENERAL QUERY
    # ============================================================

    def get_waveform_type(self):
        response = self.query(f"{self.ch}:BSWV?")

        if "WVTP,NOISE" in response:
            return "NOISE"

        if "WVTP,SINE" in response:
            return "SINE"

        if "WVTP,DC" in response:
            return "DC"

        return "UNKNOWN"

    def get_mean(self):
        """
        Parse MEAN or OFST from response.
        """

        response = self.query(f"{self.ch}:BSWV?")

        try:
            parts = response.split(",")

            for i, p in enumerate(parts):

                if p.strip() in ["MEAN", "OFST"]:
                    value = parts[i + 1]

                    value = value.replace("V", "")
                    return float(value)

        except Exception:
            pass

        return 0.0

    def query_settings(self):
        """
        Print waveform and output settings.
        """

        print(self.query(f"{self.ch}:BSWV?"))
        print(self.query(f"{self.ch}:OUTP?"))

    # ============================================================
    # CLEANUP
    # ============================================================

    def close(self):
        """
        Close VISA connection.
        """

        self.inst.close()
        self.rm.close()

    # ============================================================
    # CONTEXT MANAGER
    # ============================================================

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


# ============================================================
# EXAMPLE USAGE
# ============================================================

if __name__ == "__main__":

    RESOURCE = "TCPIP0::10.10.0.50::INSTR"

    sdg2042 = SDG2042XNoise(resource=RESOURCE, channel=1, output_load="HZ")
    sdg2042.set_sine_amplitude(0.1)
    sdg2042.query_settings()

    # INJECTED_NOISE = [2, 0, 2]
    # for idn, injected_noise in enumerate(INJECTED_NOISE):
    #     sdg2042.set_std(injected_noise*1e-3)
    #     sdg2042.query_settings()

    FREQS = [1.5,2,2.5]
    for idf, freq in enumerate(FREQS):
        sdg2042.set_sine_frequency(freq*1e6)
        sdg2042.query_settings()

    # with SDG2042XNoise(
    #     resource=RESOURCE,
    #     channel=1,
    #     output_load="HZ",
    # ) as gen:

    #     gen.enable_noise(
    #         std_rms=0.1,
    #         mean=0.0,
    #     )

    #     gen.query_settings()

    #     print("\nUpdating std to 0.05 Vrms\n")
    #     gen.set_std(0.05)

    #     gen.query_settings()

    #     print("\nSetting std = 0 (switches to DC)\n")
    #     gen.set_std(0)

    #     gen.query_settings()
