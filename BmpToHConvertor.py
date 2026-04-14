import argparse
from os import write

parser = argparse.ArgumentParser()
parser.add_argument("--INPUT_FILES", nargs='+', help="Path vers le fichier d'entrée")
parser.add_argument("--HEADER_NAME", help="Path pour le fichier de sortie")
parser.add_argument("--VAR_NAMES", nargs='+', help="Nom de la varriable à crée")

args = parser.parse_args()

in_files = args.INPUT_FILES
var_names = args.VAR_NAMES


output_file = open(args.HEADER_NAME+".h", "w")

output_file.write("""#ifndef INC_{}_H_
#define INC_{}_H_

#include "stdint.h"
""".format(args.HEADER_NAME.upper(), args.HEADER_NAME.upper()))

for in_file, var_name in zip(in_files, var_names):
    input_file = open(in_file, "rb")
    output_file.write("""uint8_t {}[] = """.format(var_name) + """{
""")

    bytes_array = []
    while input_file.peek(1) != b'' :
        bytes_array.append("0x" + input_file.read(1).hex())

    print("Il y a {} octets dans le fichier".format(len(bytes_array)))

    data = ""
    for n, repr in enumerate(bytes_array):
        data += repr +', '
        if n%8 == 7:
            data+= "\n"
    output_file.write(data)
    output_file.write("""
};

""")
    input_file.close()

output_file.write("""
#endif /* INC_{}_H_ */
""".format(args.HEADER_NAME.upper()))

output_file.close()