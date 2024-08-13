import re
class ScanRange:
    def __init__(self, data_chunk):
        self.index = None
        self.left_inclusive = None
        self.right_inclusive = None
        self.variations = None
        self.parse(data_chunk)

    def parse(self, data_chunk):
        if _match := re.search(r'/\*Range #(\d+):\*/', data_chunk):
            self.index = int(_match.group(1))

        # Regex pattern to capture key-value pairs
        pattern = re.compile(r'/\*(.*?)\s*\*/(?:\t*)(.*)')
        for match in pattern.finditer(data_chunk):
            key = match.group(1).strip()  # Capture the key and remove extra spaces
            value = match.group(2).strip()  # Capture the value and remove extra spaces
            if key == "Left-Inclusive?":
                self.left_inclusive = bool(int(value))
            elif key == "Right-Inclusive?":
                self.right_inclusive = bool(int(value))
            elif key == "# Variations:":
                self.variations = int(value)

    def print(self):
        left_inclusive_str = f"/*Left-Inclusive?*/\t\t{int(self.left_inclusive)}"
        right_inclusive_str = f"/*Right-Inclusive?*/\t{int(self.right_inclusive)}"
        variations_str = f"/*# Variations: */\t\t{self.variations}"
        return (f"/*Range #{self.index}:*/\n"
                f"{left_inclusive_str}\n"
                f"{right_inclusive_str}\n"
                f"{variations_str}")
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()
    

if __name__ == "__main__":
    data_chunk = \
    """/*Range #1:*/
    /*Left-Inclusive?*/     1
    /*Right-Inclusive?*/    1
    /*# Variations: */      16"""

    variable = ScanRange(data_chunk)
    a = variable.__str__()
    print(variable)