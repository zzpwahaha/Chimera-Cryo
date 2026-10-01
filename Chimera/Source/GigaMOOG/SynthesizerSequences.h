#pragma once
// SynthesizerSequences.h - sequence-building library for the YJ205 RF synthesizer.

#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace synth {

    // --- Constants ----------------------------------------------------------------

    constexpr double MAX_FREQUENCY = 307.2e6;          // Hz
    constexpr double MIN_DURATION = 1.0 / 153.6e6;   // s (~6.51 ns, one hardware clock tick)
    constexpr int    N_DIGITAL = 7;
    constexpr int    N_CHANNELS = 4;
    constexpr int    MAX_LENGTH = 16384;            // timestamps per channel
    constexpr double AMPLITUDE_RESOLUTION = 1.0 / 65535.0;

    // Computed at runtime to avoid constexpr overflow on 32-bit targets
    inline double MAX_DURATION() { return MIN_DURATION * static_cast<double>((1ULL << 48) - 1); }

    // --- SequenceState ------------------------------------------------------------
    // Tracks running channel state during compilation.

    struct SequenceState {
        double amplitude = 0.0;
        double phase = 0.0;
        double frequency = 1e6;
        double time = 0.0;
        int    triggers = 0;
        std::array<bool, N_DIGITAL> digital_out{};
    };

    // --- CompiledTimestamp --------------------------------------------------------
    // Output element of compile_sequence. timestamp is the ABSOLUTE start time.

    struct CompiledTimestamp {
        double timestamp;
        int    phase_update;         // 0 = none, 1 = absolute, 2 = relative
        double phase;
        double amplitude;
        double frequency;
        bool   wait_for_trigger;
        std::array<bool, N_DIGITAL> digital_out;
    };

    struct CompileResult {
        std::map<int, std::vector<CompiledTimestamp>> channels;
        std::map<int, std::vector<double>>            durations; // per channel, per trigger section
    };

    // --- Utility ------------------------------------------------------------------

    void validate_parameters(std::optional<double> duration = std::nullopt,
        std::optional<double> amplitude = std::nullopt,
        std::optional<double> phase = std::nullopt,
        std::optional<double> frequency = std::nullopt);

    inline double todB(double a) { return 20.0 * std::log10(a); }
    inline double fromdB(double d) { return std::pow(10.0, d / 20.0); }

    // --- RFBlock base -------------------------------------------------------------

    class RFBlock {
    public:
        virtual ~RFBlock() = default;
        virtual bool isAtomic() const { return false; }
        virtual std::vector<std::shared_ptr<RFBlock>> expand(const SequenceState& state) const {
            throw std::logic_error("expand() called on atomic block: " + repr());
        }
        virtual std::string repr() const = 0;
    };

    // --- Timestamp ----------------------------------------------------------------

    class Timestamp : public RFBlock {
    public:
        double                duration;
        std::optional<double> amplitude;
        std::optional<double> phase;
        std::optional<double> frequency;
        bool                  wait_for_trigger;
        std::map<int, bool>   digital_out;   // sparse: {channel_index: state}
        bool                  absolute_phase;
        int                   phase_update = 0; // filled by compile_sequence

        explicit Timestamp(double duration,
            std::optional<double> amplitude = std::nullopt,
            std::optional<double> phase = std::nullopt,
            std::optional<double> frequency = std::nullopt,
            bool                  wait_for_trigger = false,
            std::map<int, bool>   digital_out = {},
            bool                  absolute_phase = false);

        bool isAtomic() const override { return true; }
        std::string repr() const override;
    };

    // --- Wait ---------------------------------------------------------------------

    class Wait : public Timestamp {
    public:
        explicit Wait(double duration, bool wait_for_trigger = false)
            : Timestamp(duration, std::nullopt, std::nullopt, std::nullopt, wait_for_trigger) {
        }
        std::string repr() const override;
    };

    // --- Duration-adjustment blocks -----------------------------------------------

    class AdjustPrevDuration : public RFBlock {
    public:
        double duration;
        explicit AdjustPrevDuration(double d) : duration(d) {}
        bool isAtomic() const override { return true; }
        std::string repr() const override;
    };

    class AdjustNextDuration : public RFBlock {
    public:
        double duration;
        explicit AdjustNextDuration(double d) : duration(d) {}
        bool isAtomic() const override { return true; }
        std::string repr() const override;
    };

    // --- Shapes -------------------------------------------------

    class RampShape {
    public:
        virtual ~RampShape() = default;
        virtual double value(double tau) const = 0;
        // Mean of s over [tau0, tau1]. Holding this value for the step keeps the phase exact
        // at the step boundaries.
        virtual double average(double tau0, double tau1) const;
        // max |ds/dtau| on [0,1], used to size the steps.
        virtual double maxSlope() const;
        virtual std::string name() const = 0;
    };

    class LinearShape : public RampShape {
    public:
        double value(double t) const override { return t; }
        double average(double a, double b) const override { return 0.5 * (a + b); }
        double maxSlope() const override { return 1.0; }
        std::string name() const override { return "linear"; }
    };

    // Minimum-jerk: zero velocity and acceleration at both ends.
    class MinJerkShape : public RampShape {
    public:
        double value(double t) const override;
        double average(double a, double b) const override;
        double maxSlope() const override { return 1.875; } // at tau = 0.5
        std::string name() const override { return "minjerk"; }
    };

    // sin^2 (raised cosine): zero velocity at both ends.
    class Sin2Shape : public RampShape {
    public:
        double value(double t) const override;
        double average(double a, double b) const override;
        double maxSlope() const override { return PI / 2.0; }
        std::string name() const override { return "sin2"; }
    };

    // tanh with steepness k. Uses the numerical defaults on purpose, to exercise them.
    class TanhShape : public RampShape {
    public:
        explicit TanhShape(double k);
        double value(double t) const override;
        double average(double a, double b) const override;
        double maxSlope() const override; // = derivative at center of ramp
        std::string name() const override { return "tanh:" + std::to_string(k); }
    private:
        double k, norm;
    };

    // "linear", "minjerk", "sin2", "tanh" or "tanh:<k>"
    std::shared_ptr<const RampShape> makeRampShape(const std::string& spec);

    // --- Ramp blocks --------------------------------------------------------------

    class FrequencyRamp : public RFBlock {
    public:
        double duration;
        std::optional<double> amplitude, phase, start_frequency, end_frequency;
        int steps;

        FrequencyRamp(double d,
            std::optional<double> a = std::nullopt,
            std::optional<double> ph = std::nullopt,
            std::optional<double> f0 = std::nullopt,
            std::optional<double> f1 = std::nullopt,
            int steps = 20)
            : duration(d), amplitude(a), phase(ph),
            start_frequency(f0), end_frequency(f1), steps(steps) {
        }

        std::vector<std::shared_ptr<RFBlock>> expand(const SequenceState& state) const override;
        std::string repr() const override;
    };

    class FrequencyFunctionRamp : public RFBlock {
    public:
        double duration;
        std::optional<double> amplitude, phase, start_frequency, end_frequency;
        std::shared_ptr<const RampShape> shape;
        double max_phase_error;
        int max_steps;

        FrequencyFunctionRamp(double d,
            std::shared_ptr<const RampShape> shape,
            std::optional<double> a = std::nullopt,
            std::optional<double> ph = std::nullopt,
            std::optional<double> f0 = std::nullopt,
            std::optional<double> f1 = std::nullopt,
            double max_phase_error = 1e-3,
            int max_steps = MAX_LENGTH - 8)   // leave room for header/stop rows
            : duration(d), amplitude(a), phase(ph), start_frequency(f0), end_frequency(f1),
            shape(std::move(shape)), max_phase_error(max_phase_error), max_steps(max_steps) {
        }

        std::vector<std::shared_ptr<RFBlock>> expand(const SequenceState& state) const override;
        std::string repr() const override;
    };

    class AmplitudeRamp : public RFBlock {
    public:
        double duration;
        std::optional<double> start_amplitude, end_amplitude, phase, frequency;
        int steps;

        AmplitudeRamp(double d,
            std::optional<double> a0 = std::nullopt,
            std::optional<double> a1 = std::nullopt,
            std::optional<double> ph = std::nullopt,
            std::optional<double> fr = std::nullopt,
            int steps = 20)
            : duration(d), start_amplitude(a0), end_amplitude(a1),
            phase(ph), frequency(fr), steps(steps) {
        }

        std::vector<std::shared_ptr<RFBlock>> expand(const SequenceState& state) const override;
        std::string repr() const override;
    };

    class AmplitudeFunctionRamp : public RFBlock {
    public:
        double duration;
        std::optional<double> start_amplitude, end_amplitude, phase, frequency;
        std::shared_ptr<const RampShape> shape;
        double max_amp_error;
        int max_steps;

        AmplitudeFunctionRamp(double d,
            std::shared_ptr<const RampShape> shape,
            std::optional<double> a0 = std::nullopt,
            std::optional<double> a1 = std::nullopt,
            std::optional<double> ph = std::nullopt,
            std::optional<double> fr = std::nullopt,
            double max_amp_error = 1e-3,
            int max_steps = MAX_LENGTH - 8)   // leave room for header/stop rows
            : duration(d), start_amplitude(a0), end_amplitude(a1), phase(ph), frequency(fr),
            shape(std::move(shape)), max_amp_error(max_amp_error), max_steps(max_steps) {
        }

        std::vector<std::shared_ptr<RFBlock>> expand(const SequenceState& state) const override;
        std::string repr() const override;
    };

    class PhaseRamp : public RFBlock {
    public:
        double                duration;
        std::optional<double> amplitude, start_phase, end_phase, frequency;
        int steps;

        PhaseRamp(double d,
            std::optional<double> a = std::nullopt,
            std::optional<double> p0 = std::nullopt,
            std::optional<double> p1 = std::nullopt,
            std::optional<double> fr = std::nullopt,
            int steps = 20)
            : duration(d), amplitude(a), start_phase(p0),
            end_phase(p1), frequency(fr), steps(steps) {
        }

        std::vector<std::shared_ptr<RFBlock>> expand(const SequenceState& state) const override;
        std::string repr() const override;
    };

    // --- Repeat -------------------------------------------------------------------

    class Repeat : public RFBlock {
    public:
        std::vector<std::shared_ptr<RFBlock>> sequence;
        int repetitions;

        Repeat(std::vector<std::shared_ptr<RFBlock>> s, int n)
            : sequence(std::move(s)), repetitions(n) {
        }

        std::vector<std::shared_ptr<RFBlock>> expand(const SequenceState& state) const override;
        std::string repr() const override;
    };

    // --- compile_sequence ---------------------------------------------------------
    // Expands all RFBlocks, resolves nullopt fields from running state, converts
    // relative durations to absolute timestamps, and appends a zero-amplitude terminator.

    CompileResult compile_sequence(
        const std::map<int, std::vector<std::shared_ptr<RFBlock>>>& sequence);

}