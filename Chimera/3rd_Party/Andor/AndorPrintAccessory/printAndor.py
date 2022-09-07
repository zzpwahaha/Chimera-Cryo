import re

def printAndorErrorCode():
    with open('./andorErrorCode.txt') as f:
        contents = f.readlines()
    for line in contents:
        _, macro, num = line.strip('\n').split(' ')
        print("case {:>3s}: errorMessage = \"{:<35s} break;".format(num, macro+'\";'))

def printAndorFunctionHeader():
    lower_first = lambda s: s[:1].lower() + s[1:] if s else ''

    with open('./andorFunction.txt') as f:
        contents = f.readlines()
    for text in contents:
        function_params = re.search('\((.+?)\)', text).group(1).split(',')[1:]
        function_name = re.search(' (.+?)\(', text).group(1)
        function_name = lower_first(function_name.split('_')[-1])
        print('void {:s}'.format(function_name)+'('+', '.join(['{:s}'.format(p) for p in function_params])+');')

    # text = 'int AT_IsImplemented(AT_H Hndl, AT_WC* Feature, AT_BOOL* Implemented);'

def printAndorFunction():
    lower_first = lambda s: s[:1].lower() + s[1:] if s else ''
    cameraHandel = 'camHndl'

    with open('./andorFunction.txt') as f:
        contents = f.readlines()
    for text in contents:
        function_params_raw = re.search('\((.+?)\)', text).group(1).split(',')
        function_params = function_params_raw[1:]
        function_params_raw_notype = [fp.split(' ')[-1] for fp in function_params_raw]
        function_params_notype_withhandle = [cameraHandel] + function_params_raw_notype[1:]
        function_name_bare = re.search(' (.+?)\(', text).group(1)
        function_name = lower_first(function_name_bare.split('_')[-1])
        print('void AndorFlume::{:s}'.format(function_name)+'('+', '.join(['{:s}'.format(p) for p in function_params])+') {')
        print('\tif (!safemode) {')
        print("\t\tandorErrorChecker( {:s}".format(function_name_bare) + '(' +', '.join(['{:s}'.format(p) for p in function_params_notype_withhandle])+'), true );' )
        print('\t}')
        print('}')
        print()
    # text = 'int AT_IsImplemented(AT_H Hndl, AT_WC* Feature, AT_BOOL* Implemented);'



if __name__ == '__main__':
    # printAndorErrorCode();
    # printAndorFunctionHeader();
    printAndorFunction();


