import re
from ConfigurationManager.ConfigurationScanParameter.ScanDimension import ScanDimension
from ConfigurationManager.ConfigurationScanParameter.Variable import Variable

class ConfigurationParameter:
    def __init__(self, data_chunk):
        self.scan_dimensions = []
        self.variables = []
        self.parse(data_chunk)

    def parse(self, data_chunk):
        # Extract the chunk between RANGE-INFO and /*# Variables: */
        num_dim = 0
        match = re.search(r'RANGE-INFO(.*?)\/\*\# Variables: \*\/', data_chunk, re.DOTALL)
        if match:
            range_info_chunk = match.group(1).strip()
            if _match := re.search(r'/\*\# Scan Dimensions:\*/\s*(\d+)', range_info_chunk):
                num_dim = int(_match.group(1))

            # Parse each dimension separately
            # dim_chunks = re.findall(r'(/\*Dim #\d+:\*/.*?)(?=/\*Dim|\Z)', range_info_chunk, re.DOTALL) # also work
            dim_chunks = re.findall(r'(/\*Dim #\d+:?\*/.*?)(?=/\*Dim #\d+:?|$)', range_info_chunk, re.DOTALL)
            for dim_chunk in dim_chunks:
                dimension = ScanDimension(dim_chunk.strip('\n'))
                self.scan_dimensions.append(dimension)
        assert num_dim==len(self.scan_dimensions), "len of scan_dimension should match the one in the config."

        # Extract the chunk between /*# Variables: */ and the end
        num_var = 0
        match = re.search(r"(/\*# Variations:\s*\*/.*?)\Z", data_chunk, re.DOTALL)
        if match:
            variable_chunk = match.group(1).strip()
            if _match := re.search(r'/\*# Variables:\s*\*/\s*(\d+)', variable_chunk):
                num_var = int(_match.group(1))
            var_chunks = re.findall(r'(/\*Variable #\d+\*/.*?)(?=/\*Variable #\d+\*/|$)', variable_chunk, re.DOTALL)
            for var_chunk in var_chunks:
                variable = Variable(var_chunk.strip('\n'))
                self.variables.append(variable)
        assert num_var==len(self.variables), "len of variables should match the one in the config."

    def print(self):
        scan_dimension_repr = (f"/*# Scan Dimensions:*/\t{len(self.scan_dimensions)}\n"+
                               "\n".join(str(dimension) for dimension in self.scan_dimensions))
        variable_repr = (f"/*# Variables: */\t\t{len(self.variables)}\n"+
                         "\n".join(str(variable) for variable in self.variables))
        return ("CONFIG_PARAMETERS\n"
                "RANGE-INFO\n" + 
                scan_dimension_repr+
                "\n"+
                variable_repr+
                "\nEND_CONFIG_PARAMETERS")

    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()
    

if __name__ == "__main__":
    data_chunk=\
    """CONFIG_PARAMETERS
    RANGE-INFO
    /*# Scan Dimensions:*/	4
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
    /*# Variations: */		11
    /*Dim #2:*/ 
    /*Number of Ranges:*/	1
    /*Range #1:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		18
    /*Dim #3:*/ 
    /*Number of Ranges:*/	1
    /*Range #1:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		11
    /*Dim #4:*/ 
    /*Number of Ranges:*/	1
    /*Range #1:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		5
    /*# Variables: */ 		3
    /*Variable #0*/
    /*Name:*/			mot_gradient
    /*Scan-Type:*/		Constant 
    /*Scan-Dimension:*/	0
    /*Initial Value: */	0
    /*Final Value: */	0
    /*Initial Value: */	0
    /*Final Value: */	0
    /*Initial Value: */	0
    /*Final Value: */	0
    /*Constant Value:*/	-7
    /*Scope:*/			global

    /*Variable #1*/
    /*Name:*/			bias_x
    /*Scan-Type:*/		Constant 
    /*Scan-Dimension:*/	0
    /*Initial Value: */	-0.5
    /*Final Value: */	1
    /*Initial Value: */	0
    /*Final Value: */	0
    /*Initial Value: */	0
    /*Final Value: */	0
    /*Constant Value:*/	0.386
    /*Scope:*/			global

    /*Variable #2*/
    /*Name:*/			bias_y
    /*Scan-Type:*/		Constant 
    /*Scan-Dimension:*/	0
    /*Initial Value: */	-1
    /*Final Value: */	1
    /*Initial Value: */	0
    /*Final Value: */	0
    /*Initial Value: */	0
    /*Final Value: */	0
    /*Constant Value:*/	-0.428
    /*Scope:*/			global

    END_CONFIG_PARAMETERS"""

    variable = ConfigurationParameter(data_chunk)
    a = variable.__str__()
    print(variable)