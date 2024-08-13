import re
from ConfigurationManager.ConfigurationScanParameter.ScanRange import ScanRange

class ScanDimension:
    def __init__(self, data_chunk):
        self.index = None
        self.ranges = []
        self.parse(data_chunk)

    def parse(self, data_chunk):
        num_ranges = 0
        # Extract the scan dimensions
        if _match := re.search(r'/\*Dim #(\d+):\*/', data_chunk):
            self.index = int(_match.group(1))

        if _match := re.search(r'/\*Number of Ranges:\*/\s*(\d+)', data_chunk):
            num_ranges = int(_match.group(1))

        # Split the data chunk into separate ranges based on the Range delimiter
        # range_chunks = re.split(r'(/\*Range #\d+:\*/)', data_chunk)[1:]  # Skip the first element which is before the first range
        
        # Use regex to capture both the Range delimiter and its content
        range_chunks = re.findall(r'(/\*Range #\d+:\*/.*?)(?=(/\*Range #\d+:|\Z))', data_chunk, re.DOTALL)


        for range_chunk in range_chunks:
            # Create a new Range instance for each range chunk
            range_instance = ScanRange(range_chunk[0].strip())
            self.ranges.append(range_instance)
        assert num_ranges==len(self.ranges), "len of ranges should match the one in the config."

    def print(self):
        range_output = [str(range_instance) for range_instance in self.ranges]
        return (f"/*Dim #{self.index}:*/\n" 
                f"/*Number of Ranges:*/	{len(self.ranges)}\n"
                + "\n".join(range_output))

    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()

if __name__ == "__main__":
    data_chunk = \
    """
    /*Dim #1:*/ 
    /*Number of Ranges:*/	3
    /*Range #1:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		16
    /*Range #2:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		11
    /*Range #3:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		11"""

    variable = ScanDimension(data_chunk)
    a = variable.__str__()
    print(variable)