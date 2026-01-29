import re
from collections import OrderedDict

class DataAnalysisSection:
    PARAMETER_PATTERN = re.compile(r"/\*(.*?)\*/\s*(.*)")

    def __init__(self, section_name: str, data_chunk: str):
        self.section_name = section_name
        self.parameters = OrderedDict()          # master source of truth
        self.parameters_before = OrderedDict()
        self.parameters_after = OrderedDict()
        self.active_plots = []                   # list of dicts
        self._active_plots_declared = None
        self._parse(data_chunk)

    def _parse(self, data_chunk: str):
        lines = [ln.strip() for ln in data_chunk.splitlines() if ln.strip()]
        in_active_plots = False
        passed_active_plots = False
        current_plot = None
        current_plot_key = None

        for line in lines:
            if line == self.section_name or line.startswith("END_"+self.section_name):
                continue

            if line == "BEGIN_ACTIVE_PLOTS":
                in_active_plots = True
                self.parameters["BEGIN_ACTIVE_PLOTS"] = None
                continue

            if line == "END_ACTIVE_PLOTS":
                in_active_plots = False
                passed_active_plots = True
                if current_plot:
                    self.parameters[current_plot_key] = current_plot
                    self.active_plots.append(current_plot)
                    current_plot = None
                self.parameters["END_ACTIVE_PLOTS"] = None
                continue

            if in_active_plots:
                if (m := self.PARAMETER_PATTERN.match(line)):
                    key, val = m.groups()
                    key, val = key.strip(), val.strip()
                    if key == "Number of Active Plots:":
                        self._active_plots_declared = int(val) if val.isdigit() else None
                        self.parameters[key] = val
                    elif key.startswith("Active Plot"):
                        if current_plot:
                            self.parameters[current_plot_key] = current_plot
                            self.active_plots.append(current_plot)
                        current_plot_key = key
                        current_plot = OrderedDict()
                    else:
                        if current_plot is not None:
                            current_plot[key] = val
                continue

            if (m := self.PARAMETER_PATTERN.match(line)):
                key, val = m.groups()
                key, val = key.strip(), val.strip()
                if not passed_active_plots:
                    self.parameters_before[key] = val
                else:
                    self.parameters_after[key] = val
                self.parameters[key] = val

        if current_plot:
            self.parameters[current_plot_key] = current_plot
            self.active_plots.append(current_plot)

        if self._active_plots_declared is None:
            self._active_plots_declared = len(self.active_plots)

    def _rebuild_active_plots_parameters(self):
        # remove old Active Plot entries
        keys_to_delete = [
            k for k in self.parameters
            if k.startswith("Active Plot")
        ]
        for k in keys_to_delete:
            del self.parameters[k]

        # update declared number
        self._active_plots_declared = len(self.active_plots)
        self.parameters["Number of Active Plots:"] = str(self._active_plots_declared)
        active_plot_index = list(self.parameters.keys()).index("Number of Active Plots:") + 1

        # reinsert plots with normalized indices
        parameter_list = list(self.parameters.items())
        for i, plot in enumerate(self.active_plots, start=1):
            parameter_list.insert(active_plot_index, (f"Active Plot #{i}", plot))
            active_plot_index += 1
        self.parameters = OrderedDict(parameter_list)

    def add_active_plot(self, plot_name: str, which_grid: int = 0):
        # Append a new active plot.
        plot_dict = OrderedDict({"Plot Name:": plot_name, "Which Grid:": str(which_grid)})
        self.active_plots.append(plot_dict)
        self._rebuild_active_plots_parameters()

    def delete_active_plot(self, index: int):
        # Delete an active plot by index (0-based).
        if not (0 <= index < len(self.active_plots)):
            raise IndexError("Active plot index out of range")
        del self.active_plots[index]
        self._rebuild_active_plots_parameters()

    def insert_active_plot(self, index: int, plot_name: str, which_grid: int = 0):
        # Insert a new active plot at position `index` (0-based).
        if not (0 <= index <= len(self.active_plots)):
            raise IndexError("Active plot index out of range")
        plot_dict = OrderedDict({"Plot Name:": plot_name, "Which Grid:": str(which_grid)})
        self.active_plots.insert(index, plot_dict)
        self._rebuild_active_plots_parameters()

    def get_num_active_plots(self) -> int:
        return len(self.active_plots)

    def print(self) -> str:
        out_lines = [self.section_name]
        for key, val in self.parameters.items():
            if key == "BEGIN_ACTIVE_PLOTS":
                out_lines.append("BEGIN_ACTIVE_PLOTS")
                continue
            if key.startswith("Active Plot"):
                idx = int(key.split("#")[-1])
                out_lines.append(f"/*Active Plot #{idx}*/")
                for subk, subv in val.items():
                    out_lines.append(f"/*{subk}*/ {subv}")
                continue
            if key == "END_ACTIVE_PLOTS":
                out_lines.append("END_ACTIVE_PLOTS")
                continue
            out_lines.append(f"/*{key}*/\t{val}")
        out_lines.append(f"END_{self.section_name}")
        return "\n".join(out_lines)

    def __str__(self):
        return self.print()

    def __repr__(self):
        return self.print()


if __name__ == '__main__':
    data_chunk = '''DATA_ANALYSIS
/*Auto-Threshold Analysis?*/	0
/*Number of Analysis Grids: */	1
/*Grid #1:*/ 
/*Grid Origin X(Bottom-Left Corner Column):*/		0
/*Grid Origin Y(Bottom-Left Corner Row):*/		0
/*Grid Width:*/					0
/*Grid Height:*/				0
/*Pixel Spacing X:*/			0
/*Pixel Spacing Y:*/			0
/*Include Pixels X:*/			0
/*Include Pixels Y:*/			0
/*Use External File:*/			1
/*Grid File Name:*/				atomgrid_1x13_4points_2025-9-8
BEGIN_ACTIVE_PLOTS
/*Number of Active Plots:*/ 3
/*Active Plot #1*/
/*Plot Name:*/ Histogram-2Pic
/*Which Grid:*/ 0
/*Active Plot #2*/
/*Plot Name:*/ Loadingrate-2Pic
/*Which Grid:*/ 0
/*Active Plot #3*/
/*Plot Name:*/ Survival-2Pic
/*Which Grid:*/ 0
END_ACTIVE_PLOTS
/*Display Grid?*/ 0
/*Auto Bump Analysis?*/ 0
/*Bump Param?*/ "!#EMPTY_STRING#!"
END_DATA_ANALYSIS
'''

    variable = DataAnalysisSection("DATA_ANALYSIS",data_chunk)
    a = variable.__str__()
    variable.insert_active_plot(1,'test')
    print(variable)