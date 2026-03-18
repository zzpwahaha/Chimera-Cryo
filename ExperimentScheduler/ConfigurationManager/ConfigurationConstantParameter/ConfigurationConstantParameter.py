import re
from collections import OrderedDict


class MasterConfiguration:
    VAR_PATTERN = re.compile(r"^(?P<name>\w+)\s+(?P<value>-?\d+(?:\.\d+)?)$")

    def __init__(self, file_path: str):
        self.file_path = file_path

        self.raw_lines = []
        self.variables = OrderedDict()
        self.variable_line_index = {}
        self.var_count_idx = None  # where the "number_of_variable" is
        self.var_start_idx = None  # first variable line
        self.read_file()

    def reopen(self):
        self.__init__(self.file_path)

    def read_file(self):
        """Reads the content from the config file."""
        with open(self.file_path, 'r') as file:
            content = file.read()
            self.parse_content(content)

    def parse_content(self, content):
        """
        Locate variable block and parse variables.
        Strategy:
        - Find consecutive lines matching "name value"
        - The line before that block is the count
        """
        self.raw_lines = content.splitlines()
        matches = []

        for idx, line in enumerate(self.raw_lines):
            if self.VAR_PATTERN.match(line.strip()):
                matches.append(idx)

        if not matches:
            return

        # Find largest consecutive block (this is the variable section)
        groups = []
        current = [matches[0]]

        for i in range(1, len(matches)):
            if matches[i] == matches[i - 1] + 1:
                current.append(matches[i])
            else:
                groups.append(current)
                current = [matches[i]]

        groups.append(current)

        var_block = max(groups, key=len)

        self.var_start_idx = var_block[0]
        self.var_count_idx = self.var_start_idx - 1

        # Parse variables
        for idx in var_block:
            line = self.raw_lines[idx].strip()
            match = self.VAR_PATTERN.match(line)

            name = match.group("name")
            value = float(match.group("value"))

            self.variables[name] = value
            self.variable_line_index[name] = idx

    def get_constant(self, name):
        """
        Get the value of a variable.
        """
        if name not in self.variables:
            raise KeyError(name)

        return self.variables[name]

    def set_constant(self, name, value):
        self.variables[name] = value

        if name not in self.variable_line_index:
            # append at end of variable block
            insert_idx = self.var_start_idx + len(self.variables) - 1
            self.raw_lines.insert(insert_idx, "")
            self.variable_line_index[name] = insert_idx

    def rename_constant(self, old, new):
        if old not in self.variables:
            raise KeyError(old)

        value = self.variables.pop(old)
        idx = self.variable_line_index.pop(old)

        self.variables[new] = value
        self.variable_line_index[new] = idx

    def remove_constant(self, name):
        """
        Remove a variable from the configuration.
        """
        if name not in self.variables:
            raise KeyError(name)

        # get index of the line to remove
        remove_idx = self.variable_line_index[name]

        # remove from storage
        del self.variables[name]
        del self.variable_line_index[name]

        # remove the line from raw_lines
        self.raw_lines.pop(remove_idx)

        # --- update indices of all variables after this line ---
        for key, idx in self.variable_line_index.items():
            if idx > remove_idx:
                self.variable_line_index[key] = idx - 1

    def __str__(self):
        lines = list(self.raw_lines)

        # --- update variable count ---
        if self.var_count_idx is not None:
            lines[self.var_count_idx] = str(len(self.variables))

        # --- update variable lines ---
        for name, value in self.variables.items():
            idx = self.variable_line_index[name]

            if isinstance(value, float) and value.is_integer():
                value = int(value)

            lines[idx] = f"{name} {value}"

        return "\n".join(lines)

    def save(self, output_path=None):
        """Saves the current configuration back to the file."""
        path = output_path if output_path else self.file_path
        with open(path, 'w') as file:
            file.write(str(self))

if __name__ == '__main__':
    master_config = MasterConfiguration('.\\test\\Master-Configuration.txt')
    master_config.set_constant('aod_offset_x',0.206)
    master_config.save()
    print(str(master_config))
