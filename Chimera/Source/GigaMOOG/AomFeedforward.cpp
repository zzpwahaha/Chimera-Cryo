#include "stdafx.h"
#include "AomFeedforward.h"

#include <algorithm>
#include <cmath>

namespace gm420 {

	void AmplitudeCorrection::load(const std::string& filePath)
	{
		std::vector<std::vector<double>> rows;
		try {
			rows = synth::read_numeric_table(filePath, 2, 2);      // tau_s, gain
		}
		catch (std::exception& e) {
			thrower("ERROR: GigaMoog420 correction file: " + std::string(e.what()));
		}
		std::vector<std::pair<double, double>> samples;   // (tau in s, gain)
		for (size_t i = 0; i < rows.size(); ++i) {
			if (rows[i][1] < 0.0) {
				thrower("ERROR: GigaMoog420 correction file \"" + filePath + "\", data row " + str(i + 1)
					+ ": gain must be >= 0.");
			}
			samples.emplace_back(rows[i][0], rows[i][1]);
		}

		std::sort(samples.begin(), samples.end());
		// Average samples at the same tau so the time axis is strictly increasing.
		std::vector<double> t, g;
		for (size_t i = 0; i < samples.size();) {
			size_t j = i;
			double sum = 0.0;
			while (j < samples.size() && samples[j].first == samples[i].first) {
				sum += samples[j].second;
				++j;
			}
			t.push_back(samples[i].first);
			g.push_back(sum / static_cast<double>(j - i));
			i = j;
		}
		if (t.size() < 2) {
			thrower("ERROR: GigaMoog420 correction file \"" + filePath + "\" needs at least two samples.");
		}

		path = filePath;
		tau = std::move(t);
		gain = std::move(g);
	}

	bool AmplitudeCorrection::rowGain(double tau0, double tau1, double& g) const
	{
		if (tau0 < tau.front() || tau1 > tau.back()) {
			return false;
		}
		// Mean of the samples inside the window...
		auto first = std::lower_bound(tau.begin(), tau.end(), tau0);
		auto last = std::lower_bound(tau.begin(), tau.end(), tau1);
		if (last > first) {
			double sum = 0.0;
			for (auto it = first; it != last; ++it) sum += gain[it - tau.begin()];
			g = sum / static_cast<double>(last - first);
			return true;
		}
		// ...or, for a row shorter than the sample spacing, the interpolated gain at its center.
		double tc = 0.5 * (tau0 + tau1);
		size_t i = std::upper_bound(tau.begin(), tau.end(), tc) - tau.begin();
		i = std::clamp<size_t>(i, 1, tau.size() - 1);
		double f = (tc - tau[i - 1]) / (tau[i] - tau[i - 1]);
		g = gain[i - 1] + f * (gain[i] - gain[i - 1]);
		return true;
	}

	std::string applyCorrections(synth::CompileResult& compiled, const std::map<std::string, AmplitudeCorrection>& corrections)
	{
		std::string warnings;
		for (const auto& [label, corr] : corrections) {
			if (!corr.isLoaded()) continue;
			std::string where = "GigaMoog420 correction \"" + label + "\"";

			// Find the one channel and the one contiguous run of rows carrying this label.
			int channel = -1;
			size_t first = 0, last = 0;
			for (auto& [ch, rows] : compiled.channels) {
				for (size_t i = 0; i < rows.size(); ++i) {
					if (rows[i].label != label) continue;
					if (channel == -1) {
						channel = ch;
						first = last = i;
					}
					else if (ch != channel || i != last + 1) {
						thrower("ERROR: " + where + ": the label is used for more than one block. Labels must be unique.");
					}
					else {
						last = i;
					}
				}
			}
			if (channel == -1) {
				warnings += where + ": no block in the script has this label; the correction was not used.\r\n";
				continue;
			}
			auto& rows = compiled.channels.at(channel);
			const auto& durations = compiled.durations.at(channel);
			for (size_t i = first + 1; i <= last; ++i) {
				if (rows[i].wait_for_trigger) {
					thrower("ERROR: " + where + ": the block contains a trigger, so it has no single time axis.");
				}
			}

			// Row i plays until the next row's timestamp, or, if the next row starts a new trigger section
			// (or there is none), until the end of its own section. Timestamps restart at 0 at every
			// wait-for-trigger row, and section k (counted by those rows) lasts durations[k].
			size_t section = 0;
			for (size_t i = 0; i <= first; ++i) {
				if (rows[i].wait_for_trigger) ++section;
			}
			const double sectionEnd = section < durations.size() ? durations[section] : rows[last].timestamp;
			const double blockStart = rows[first].timestamp;

			// Rows the curve doesn't cover are left unchanged without a warning: a correction curve that
			// ends before the block does (e.g. once the light is already off) is normal.
			size_t clipped = 0;
			for (size_t i = first; i <= last; ++i) {
				double tau0 = rows[i].timestamp - blockStart;
				double tau1 = ((i + 1 < rows.size() && !rows[i + 1].wait_for_trigger) ? rows[i + 1].timestamp : sectionEnd)
					- blockStart;
				if (tau1 <= tau0) continue;
				double g;
				if (!corr.rowGain(tau0, tau1, g)) {
					continue;
				}
				double a = rows[i].amplitude * g;
				if (a > MAX_CORRECTED_AMPLITUDE) {
					a = MAX_CORRECTED_AMPLITUDE;
					++clipped;
				}
				rows[i].amplitude = a;
			}
			if (clipped > 0) {
				warnings += where + " (channel " + str(channel) + "): " + str(clipped) + " rows would exceed "
					+ str(MAX_CORRECTED_AMPLITUDE * 100) + " % and were clipped to it.\r\n";
			}
		}
		return warnings;
	}

}
