// SynthesizerSequences.cpp - implementation of the sequence-building library for YJ205.

#include "stdafx.h"
#include "SynthesizerSequences.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace synth {

    namespace {

        // One step of a shaped ramp: its length and the shape's mean value over it.
        struct ShapedStep {
            double duration;
            double sAvg;
        };

        // Ramp length in whole clock ticks, so the hardware step lengths are exactly the ones
        // used to compute each step's value.
        long long rampTicks(double duration, const std::string& who) {
            long long ticks = std::llround(duration / MIN_DURATION);
            if (ticks < 2) throw std::invalid_argument(who + " is shorter than 2 clock ticks");
            return ticks;
        }

        // Turn the step count the tolerance asks for into the one we use: at least 2, at most
        // one step per tick, and within the table.
        long long checkedSteps(double needed, long long totalTicks, int maxSteps, const std::string& who) {
            double n = std::min(std::ceil(needed), static_cast<double>(totalTicks));  // clamp before the cast
            long long steps = std::max(2LL, static_cast<long long>(n));
            if (steps > maxSteps)
                throw std::runtime_error(who + " needs " + std::to_string(steps) + " steps (limit "
                    + std::to_string(maxSteps) + "); lengthen the ramp or relax the tolerance");
            return steps;
        }

        // Split totalTicks into `steps` whole-tick steps (lengths differ by at most one tick).
        std::vector<ShapedStep> shapedSteps(const RampShape& shape, long long totalTicks, long long steps) {
            std::vector<ShapedStep> out;
            out.reserve(static_cast<size_t>(steps));
            for (long long i = 0; i < steps; ++i) {
                long long k0 = (i * totalTicks) / steps;
                long long k1 = ((i + 1) * totalTicks) / steps;
                out.push_back({ (k1 - k0) * MIN_DURATION,
                    shape.average(double(k0) / totalTicks, double(k1) / totalTicks) });
            }
            return out;
        }

    }

    // --- Utility ------------------------------------------------------------------

    void validate_parameters(std::optional<double> duration,
        std::optional<double> amplitude,
        std::optional<double> phase,
        std::optional<double> frequency) {
        if (duration.has_value() && (*duration < 0 || *duration > MAX_DURATION()))
            throw std::out_of_range("Duration " + std::to_string(*duration) +
                " must be between 0 and " + std::to_string(MAX_DURATION()));
        if (amplitude.has_value() && (*amplitude < 0 || *amplitude > 1))
            throw std::out_of_range("Amplitude " + std::to_string(*amplitude) +
                " must be between 0 and 1");
        if (phase.has_value() && !std::isfinite(*phase))
            throw std::out_of_range("Phase " + std::to_string(*phase) +
                " must be a finite real number (not NaN or inf)");
        if (frequency.has_value() && (*frequency < 0 || *frequency > MAX_FREQUENCY))
            throw std::out_of_range("Frequency " + std::to_string(*frequency) +
                " must be between 0 and " + std::to_string(MAX_FREQUENCY));
    }

    // --- Timestamp ----------------------------------------------------------------

    Timestamp::Timestamp(double duration,
        std::optional<double> amplitude,
        std::optional<double> phase,
        std::optional<double> frequency,
        bool wait_for_trigger,
        std::map<int, bool> digital_out,
        bool absolute_phase)
        : duration(duration), amplitude(amplitude), phase(phase), frequency(frequency),
        wait_for_trigger(wait_for_trigger), digital_out(std::move(digital_out)),
        absolute_phase(absolute_phase) {
    }

    std::string Timestamp::repr() const {
        std::ostringstream ss;
        ss << "Timestamp(" << duration;
        if (amplitude) ss << ", amplitude=" << *amplitude;
        if (phase)     ss << ", phase=" << *phase;
        if (frequency) ss << ", frequency=" << *frequency;
        if (wait_for_trigger) ss << ", wait_for_trigger=true";
        ss << ")";
        return ss.str();
    }

    std::string Wait::repr() const {
        return "Wait(" + std::to_string(duration) +
            (wait_for_trigger ? ", wait_for_trigger=true" : "") + ")";
    }

    std::string AdjustPrevDuration::repr() const {
        return "AdjustPrevDuration(" + std::to_string(duration) + ")";
    }

    std::string AdjustNextDuration::repr() const {
        return "AdjustNextDuration(" + std::to_string(duration) + ")";
    }

    // --- FrequencyRamp ------------------------------------------------------------
    // must send a single table entry (consisting of 4 write packets (time low, time high, ftw, atw) to new address) per frequency step
    std::vector<std::shared_ptr<RFBlock>> 
        FrequencyRamp::expand(const SequenceState& state) const {
        validate_parameters(duration, amplitude, phase, start_frequency); // shouldn't validate ramp type with this function
        validate_parameters(std::nullopt, std::nullopt, std::nullopt, end_frequency);
        if (steps < 2)
            throw std::invalid_argument("FrequencyRamp steps must be >= 2");
        double f0 = start_frequency.value_or(state.frequency);
        double f1 = end_frequency.value_or(f0);
        double step = duration / steps;
        std::vector<std::shared_ptr<RFBlock>> seq;
        for (int i = 0; i < steps; ++i) {
            double fr = f0 + (f1 - f0) * i / (steps - 1);
            auto ts = std::make_shared<Timestamp>(step, std::nullopt, std::nullopt, fr);
            if (i == 0) { ts->phase = phase; ts->amplitude = amplitude; }
            seq.push_back(ts);
        }
        return seq;
    }

    std::string FrequencyRamp::repr() const {
        return "FrequencyRamp(" + std::to_string(duration) + ", steps=" + std::to_string(steps) + ")";
    }

    // --- FrequencyFunctionRamp -------------------------
    std::vector<std::shared_ptr<RFBlock>>
        FrequencyFunctionRamp::expand(const SequenceState& state) const {
        validate_parameters(duration, amplitude, phase, start_frequency);
        validate_parameters(std::nullopt, std::nullopt, std::nullopt, end_frequency);
        if (!shape) throw std::invalid_argument("FrequencyFunctionRamp needs a shape");
        if (!(max_phase_error > 0)) throw std::invalid_argument("max_phase_error must be > 0");

        const double f0 = start_frequency.value_or(state.frequency);
        const double f1 = end_frequency.value_or(f0);
        const double df = f1 - f0;

        const long long totalTicks = rampTicks(duration, repr());
        const double T = totalTicks * MIN_DURATION;

        // Peak phase error per step is pi * alpha * dt^2 / 4; size dt for the fastest point.
        const double alphaMax = std::abs(df) / T * shape->maxSlope();   // Hz/s
        const double needed = alphaMax > 0
            ? T / std::sqrt(4.0 * max_phase_error / (PI * alphaMax)) : 2.0;
        const long long steps = checkedSteps(needed, totalTicks, max_steps, repr());

        std::vector<std::shared_ptr<RFBlock>> seq;
        for (const auto& st : shapedSteps(*shape, totalTicks, steps)) {
            auto ts = std::make_shared<Timestamp>(st.duration, std::nullopt, std::nullopt, f0 + df * st.sAvg);
            if (seq.empty()) { ts->phase = phase; ts->amplitude = amplitude; }
            seq.push_back(ts);
        }
        seq.push_back(std::make_shared<Timestamp>(0.0, std::nullopt, std::nullopt, f1)); // updating the last step with final freq instead of leaving as the avg.
        return seq;
    }

    std::string FrequencyFunctionRamp::repr() const {
        return "FrequencyFunctionRamp(" + std::to_string(duration) + ", "
            + (shape ? shape->name() : "null") + ")";
    }

    // --- AmplitudeRamp ------------------------------------------------------------

    std::vector<std::shared_ptr<RFBlock>>
        AmplitudeRamp::expand(const SequenceState& state) const {
        validate_parameters(duration, start_amplitude, phase, frequency);
        validate_parameters(std::nullopt, end_amplitude);
        if (steps < 2)
            throw std::invalid_argument("AmplitudeRamp steps must be >= 2");
        double a0 = start_amplitude.value_or(state.amplitude);
        double a1 = end_amplitude.value_or(a0);
        double step = duration / steps;
        std::vector<std::shared_ptr<RFBlock>> seq;
        for (int i = 0; i < steps; ++i) {
            double a = a0 + (a1 - a0) * i / (steps - 1);
            auto ts = std::make_shared<Timestamp>(step, a);
            if (i == 0) { ts->phase = phase; ts->frequency = frequency; }
            seq.push_back(ts);
        }
        return seq;
    }

    std::string AmplitudeRamp::repr() const {
        return "AmplitudeRamp(" + std::to_string(duration) + ", steps=" + std::to_string(steps) + ")";
    }

    // --- AmplitudeFunctionRamp -----------------------------

    std::vector<std::shared_ptr<RFBlock>>
        AmplitudeFunctionRamp::expand(const SequenceState& state) const {
        validate_parameters(duration, start_amplitude, phase, frequency);
        validate_parameters(std::nullopt, end_amplitude);
        if (!shape) throw std::invalid_argument("AmplitudeFunctionRamp needs a shape");
        if (!(max_amp_error > 0)) throw std::invalid_argument("max_amp_error must be > 0");

        const double a0 = start_amplitude.value_or(state.amplitude);
        const double a1 = end_amplitude.value_or(a0);
        const double da = a1 - a0;

        const long long totalTicks = rampTicks(duration, repr());

        // ATW rounding already costs up to half an LSB, so a tighter tolerance would only
        // produce rows that round to the same word.
        const double tol = std::max(max_amp_error, 0.5 * AMPLITUDE_RESOLUTION);
        // Holding the step average, the worst error is at the step edges: |dA/dt| * dt / 2.
        // With |dA/dt|max = |da| / T * maxSlope, T cancels out of the step count.
        const double needed = std::abs(da) * shape->maxSlope() / (2.0 * tol);
        const long long steps = checkedSteps(needed, totalTicks, max_steps, repr());

        std::vector<std::shared_ptr<RFBlock>> seq;
        for (const auto& st : shapedSteps(*shape, totalTicks, steps)) {
            // Clamp guards against numerical overshoot at the ends pushing a value out of [0, 1].
            double a = std::clamp(a0 + da * st.sAvg, 0.0, 1.0);
            auto ts = std::make_shared<Timestamp>(st.duration, a);
            if (seq.empty()) { ts->phase = phase; ts->frequency = frequency; }
            seq.push_back(ts);
        }
        // No table row, but leaves the running state at exactly a1.
        seq.push_back(std::make_shared<Timestamp>(0.0, a1));
        return seq;
    }

    std::string AmplitudeFunctionRamp::repr() const {
        return "AmplitudeFunctionRamp(" + std::to_string(duration) + ", "
            + (shape ? shape->name() : "null") + ")";
    }

    // --- PhaseRamp ----------------------------------------------------------------

    std::vector<std::shared_ptr<RFBlock>>
        PhaseRamp::expand(const SequenceState& state) const {
        validate_parameters(duration, amplitude, start_phase, frequency);
        validate_parameters(std::nullopt, std::nullopt, end_phase);
        if (steps < 2)
            throw std::invalid_argument("PhaseRamp steps must be >= 2");
        double p0 = start_phase.value_or(state.phase);
        double p1 = end_phase.value_or(p0);
        double step = duration / steps;
        std::vector<std::shared_ptr<RFBlock>> seq;
        for (int i = 0; i < steps; ++i) {
            double p = p0 + (p1 - p0) * i / (steps - 1);
            auto ts = std::make_shared<Timestamp>(step, std::nullopt, p);
            if (i == 0) { ts->amplitude = amplitude; ts->frequency = frequency; }
            seq.push_back(ts);
        }
        return seq;
    }

    std::string PhaseRamp::repr() const {
        return "PhaseRamp(" + std::to_string(duration) + ", steps=" + std::to_string(steps) + ")";
    }

    // --- Shapes -----------------------------------------------------------------

    double RampShape::average(double a, double b) const {
        if (b <= a) return value(a);
        // Claude using Simpson's method here to approximate integral over normalized time step. 
        // Want to send the frequency which corresponds to average: integral[shape]/T
        // Guess this is fine as the default if we don't have a closed expression. But all
        // functions here have a closed form and override the average attribute.
        constexpr int n = 8;
        double h = (b - a) / n;
        double sum = value(a) + value(b);
        for (int i = 1; i < n; ++i) sum += value(a + i * h) * (i % 2 ? 4.0 : 2.0); // Simp
        return sum * h / 3.0 / (b - a);
    }
    double RampShape::maxSlope() const {
        constexpr int n = 2000;
        double m = 0.0;
        for (int i = 0; i < n; ++i)
            m = std::max(m, std::abs(value((i + 1.0) / n) - value(double(i) / n)) * n);
        return m;
    }

    double MinJerkShape::value(double t) const {
        return t * t * t * (10.0 - 15.0 * t + 6.0 * t * t);
    }
    double MinJerkShape::average(double a, double b) const {
        if (b <= a) return value(a);
        auto F = [](double t) { double t4 = t * t * t * t; return t4 * (2.5 - 3.0 * t + t * t); };
        return (F(b) - F(a)) / (b - a);
    }

    double Sin2Shape::value(double t) const { return 0.5 * (1.0 - std::cos(PI * t)); }
    double Sin2Shape::average(double a, double b) const {
        if (b <= a) return value(a);
        auto F = [](double t) { return 0.5 * t - std::sin(PI * t) / (2.0 * PI); };
        return (F(b) - F(a)) / (b - a);
    }

    static double logCosh(double x) { // helper function for antiderivative of tanh
        double ax = std::abs(x);
        return ax + std::log1p(std::exp(-2.0 * ax)) - std::log(2.0);
    }
    TanhShape::TanhShape(double k) : k(k), norm(std::tanh(k / 2.0)) {
        if (!(k > 0)) throw std::invalid_argument("tanh steepness must be > 0");
    }
    double TanhShape::value(double t) const {
        return (std::tanh(k * (t - 0.5)) + norm) / (2.0 * norm);
    }
    double TanhShape::average(double a, double b) const {
        if (b <= a) return value(a);
        auto F = [this](double t) {
            return (logCosh(k * (t - 0.5)) / k + t * norm) / (2.0 * norm);
            };
        return (F(b) - F(a)) / (b - a);
    }
    double TanhShape::maxSlope() const {
        return k / (2.0 * norm);
    }

    std::shared_ptr<const RampShape> makeRampShape(const std::string& spec) {
        std::string name = spec, arg;
        if (auto colon = spec.find(':'); colon != std::string::npos) {
            name = spec.substr(0, colon);
            arg = spec.substr(colon + 1);
        }
        if (name == "linear")  return std::make_shared<LinearShape>();
        if (name == "minjerk") return std::make_shared<MinJerkShape>();
        if (name == "sin2")    return std::make_shared<Sin2Shape>();
        if (name == "tanh")    return std::make_shared<TanhShape>(arg.empty() ? 4.0 : std::stod(arg));
        throw std::invalid_argument("unknown ramp shape \"" + spec + "\"");
    }


    // --- Repeat -------------------------------------------------------------------

    std::vector<std::shared_ptr<RFBlock>>
        Repeat::expand(const SequenceState&) const {
        std::vector<std::shared_ptr<RFBlock>> result;
        for (int i = 0; i < repetitions; ++i)
            for (auto& b : sequence)
                result.push_back(b);
        return result;
    }

    std::string Repeat::repr() const {
        return "Repeat(..., " + std::to_string(repetitions) + ")";
    }

    // --- compile_sequence ---------------------------------------------------------

    CompileResult compile_sequence(
        const std::map<int, std::vector<std::shared_ptr<RFBlock>>>& sequence) {

        CompileResult result;

        for (auto& [channel, input_blocks] : sequence) {

            std::vector<std::shared_ptr<RFBlock>> stack(input_blocks.rbegin(), input_blocks.rend());
            SequenceState state;

            std::vector<std::shared_ptr<RFBlock>> compiled_channel;

            while (!stack.empty()) {
                auto head = stack.back();
                stack.pop_back();

                if (!head->isAtomic()) {
                    auto children = head->expand(state);
                    for (auto it = children.rbegin(); it != children.rend(); ++it)
                        stack.push_back(*it);
                    continue;
                }

                bool last_is_adj_next = !compiled_channel.empty() &&
                    dynamic_cast<AdjustNextDuration*>(compiled_channel.back().get());

                if (auto* ts_raw = dynamic_cast<Timestamp*>(head.get())) {
                    auto ts = std::make_shared<Timestamp>(*ts_raw);

                    if (last_is_adj_next) {
                        auto adj = std::dynamic_pointer_cast<AdjustNextDuration>(compiled_channel.back());
                        compiled_channel.pop_back();
                        if (-adj->duration > ts->duration)
                            throw std::runtime_error("AdjustNextDuration would make duration negative for " + ts->repr());
                        ts->duration += adj->duration;
                    }

                    validate_parameters(ts->duration, ts->amplitude, ts->phase, ts->frequency);
                    if (ts->absolute_phase && !ts->phase.has_value())
                        throw std::invalid_argument("absolute_phase=true but phase is nullopt");

                    state.time += ts->duration;
                    if (ts->amplitude.has_value()) state.amplitude = *ts->amplitude;
                    if (ts->absolute_phase) {
                        ts->phase_update = 1;
                        state.phase = *ts->phase;
                    }
                    else if (ts->phase.has_value() && *ts->phase != state.phase) {
                        ts->phase_update = 2;
                        state.phase = *ts->phase;
                    }
                    if (ts->frequency.has_value()) state.frequency = *ts->frequency;
                    if (ts->wait_for_trigger) {
                        state.triggers++;
                        state.time = 0.0;
                    }

                    auto new_digital = state.digital_out;
                    for (auto& [k, v] : ts->digital_out) {
                        if (k < 0 || k >= N_DIGITAL)
                            throw std::out_of_range("digital_out key " + std::to_string(k) + " out of range");
                        new_digital[k] = v;
                    }
                    new_digital[0] = state.amplitude > 0.0; // D0 = RF switch
                    state.digital_out = new_digital;

                    if (!ts->amplitude.has_value()) ts->amplitude = state.amplitude;
                    if (!ts->phase.has_value())     ts->phase = state.phase;
                    if (!ts->frequency.has_value()) ts->frequency = state.frequency;

                    ts->digital_out.clear();
                    for (int i = 0; i < N_DIGITAL; ++i)
                        ts->digital_out[i] = state.digital_out[i];

                    bool include = ts->duration > 0.0;
                    if (!include && ts->wait_for_trigger) {
                        ts->duration = MIN_DURATION;
                        include = true;
                    }
                    else if (!include) {
                        if (!stack.empty())
                            if (auto* next = dynamic_cast<Timestamp*>(stack.back().get()))
                                if (next->wait_for_trigger) {
                                    ts->duration = MIN_DURATION;
                                    include = true;
                                }
                    }
                    if (include)
                        compiled_channel.push_back(ts);

                }
                else if (auto* adj_p = dynamic_cast<AdjustPrevDuration*>(head.get())) {
                    if (!compiled_channel.empty()) {
                        auto* last_ts = dynamic_cast<Timestamp*>(compiled_channel.back().get());
                        if (!last_ts)
                            throw std::runtime_error(
                                "AdjustPrevDuration must follow a Timestamp, got " +
                                compiled_channel.back()->repr());
                        if (-adj_p->duration > last_ts->duration)
                            throw std::runtime_error("AdjustPrevDuration would make duration negative");
                        last_ts->duration += adj_p->duration;
                        state.time += adj_p->duration;
                    }

                }
                else if (auto* adj_n = dynamic_cast<AdjustNextDuration*>(head.get())) {
                    if (last_is_adj_next)
                        throw std::runtime_error("AdjustNextDuration must be followed by a Timestamp");
                    compiled_channel.push_back(head);
                    state.time += adj_n->duration;
                }
            }

            // -- Filter to Timestamps only -----------------------------------------
            std::vector<std::shared_ptr<Timestamp>> ts_channel;
            for (auto& b : compiled_channel)
                if (auto ts = std::dynamic_pointer_cast<Timestamp>(b))
                    ts_channel.push_back(ts);

            // -- Convert relative durations -> absolute timestamps ------------------
            std::vector<double> durations;
            double cumulative = 0.0;
            for (auto& ts : ts_channel) {
                if (ts->wait_for_trigger) {
                    durations.push_back(cumulative);
                    cumulative = 0.0;
                }
                double next = ts->duration + cumulative;
                ts->duration = cumulative;
                cumulative = next;
                if (cumulative > MAX_DURATION())
                    throw std::runtime_error(
                        "Channel " + std::to_string(channel) + " exceeds MAX_DURATION");
            }
            durations.push_back(cumulative);

            // -- Append end-of-sequence timestamp if needed ------------------------
            if (!ts_channel.empty() && ts_channel.back()->duration != cumulative) {
                auto final_ts = std::make_shared<Timestamp>(cumulative,
                    state.amplitude, state.phase, state.frequency);
                for (int i = 0; i < N_DIGITAL; ++i)
                    final_ts->digital_out[i] = state.digital_out[i];
                ts_channel.push_back(final_ts);
            }

            // -- Zero-amplitude terminator -----------------------------------------
            auto term = std::make_shared<Timestamp>(0.0, 0.0, 0.0, 0.0);
            term->phase_update = 0;
            for (int i = 0; i < N_DIGITAL; ++i)
                term->digital_out[i] = false;
            ts_channel.push_back(term);

            if (static_cast<int>(ts_channel.size()) > MAX_LENGTH)
                throw std::runtime_error(
                    "Channel " + std::to_string(channel) + " exceeds MAX_LENGTH");

            // -- Emit CompiledTimestamp records ------------------------------------
            for (auto& ts : ts_channel) {
                CompiledTimestamp ct;
                ct.timestamp = ts->duration;
                ct.phase_update = ts->phase_update;
                ct.phase = ts->phase.value_or(0.0);
                ct.amplitude = ts->amplitude.value_or(0.0);
                ct.frequency = ts->frequency.value_or(0.0);
                ct.wait_for_trigger = ts->wait_for_trigger;
                for (int i = 0; i < N_DIGITAL; ++i)
                    ct.digital_out[i] = ts->digital_out.count(i) ? ts->digital_out.at(i) : false;
                result.channels[channel].push_back(ct);
            }
            result.durations[channel] = durations;
        }

        return result;
    }

}