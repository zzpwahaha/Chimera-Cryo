#pragma once
// AomFeedforward.h - per-block amplitude corrections for the GigaMoog420.
//
// A correction is a gain curve g(tau) attached to one labeled script block. tau is time from the
// start of that block, so it doesn't depend on which trigger the block follows or what comes before
// it. Every compiled row of the block is rescaled
//
//     A' = A * g_row
//
// where g_row is the mean of the g samples inside the row's window [tau0, tau1) (or g interpolated
// at the window's center when the row is shorter than the sample spacing). Rows the curve doesn't
// cover are left unchanged. A' is clipped at MAX_CORRECTED_AMPLITUDE (90 %).
//
// The curve is computed outside Chimera (e.g. the Python optimization loop measures the PD trace,
// aligns it to the block including any trigger-to-light delay, and updates g), so Chimera only maps
// it onto rows. A table ramp is labeled with its file name; other blocks get a label from a
// "label <name>" line just before them:
//     tableramp 0 sweepa                         (plays Ramps\sweepa.csv, label "sweepa")
//     correction sweepa sweepa_gain              (applies Ramps\sweepa_gain.csv to it)
//
// File format (.csv or .txt), one sample per line:  tau_s, gain
// Same separators/comments/header rules as synth::read_numeric_table. Gains must be >= 0.

#include "SynthesizerSequences.h"

#include <map>
#include <string>
#include <vector>

namespace gm420 {

	// Highest amplitude (fraction of full scale) a correction may produce. Above about 0.9 the RF chain /
	// AOM is in the danger zone, so corrected rows are clipped here rather than at 1.0.
	constexpr double MAX_CORRECTED_AMPLITUDE = 0.9;

	class AmplitudeCorrection {
	public:
		// Parse a gain file. Throws a ChimeraError and leaves this object unchanged on failure.
		void load(const std::string& filePath);
		bool isLoaded() const { return !tau.empty(); }

		const std::string& getPath() const { return path; }
		const std::vector<double>& getTimes() const { return tau; }
		const std::vector<double>& getGains() const { return gain; }

		// Gain for a row playing over [tau0, tau1) seconds from the block start. Returns false if the
		// curve doesn't cover the window.
		bool rowGain(double tau0, double tau1, double& g) const;

	private:
		std::string path;
		std::vector<double> tau;      // strictly increasing
		std::vector<double> gain;
	};

	// Apply each correction to the rows carrying its label. Throws a ChimeraError if a label is used
	// for more than one block or spans a trigger. Returns warning text, empty if there is nothing to report.
	// Several corrections can be stacked on one label, in the order they were given. Each row's amplitude
	// is multiplied by the gain of every correction in the stack that covers it; the result is clipped to
	// MAX_CORRECTED_AMPLITUDE once, after all of them, so the order of the stack doesn't change the result.
	using CorrectionStack = std::vector<AmplitudeCorrection>;
	using CorrectionMap = std::map<std::string, CorrectionStack>;

	std::string applyCorrections(synth::CompileResult& compiled, const CorrectionMap& corrections);

}
