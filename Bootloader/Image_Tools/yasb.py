import sys
import argparse
import math
import random
from ecc import make_keypair
from ecc import sign_message
from ecc import verify_signature
from ecc import scalar_mult
import collections
import clipboard
# pip install clipboard
# https://pypi.org/project/clipboard/

from Crypto.Hash import SHA256

# New image format:
# Magic Number | Signature | Length | Version | Padding | Binary Image
# 
# Magic number  - 4 bytes
# Signature     - 64 bytes for ECC-256
# Length        - 4 bytes
# Version       - 4 bytes
# Padding       - Space added to make header up to the specified header_size
# Binary image  - Image dependent
#
# Header is Magic Number, Signature, Length, Version, Padding
#
# The Signature is from (and including) the Length field
# The Length field is from (and including) the Version field
# So, total size of the image is:
# Length + 4 (Length field) + 64 (Signature) + 4 (Magic Number)

magic_number    = [89, 65, 83, 66]
ecc_bit_len     = 256
private_key_len = int((ecc_bit_len / 8))
public_key_len  = (private_key_len * 2)
signature_len   = 64
header_size     = 0x100
# padding size is the header size less magic number size less signature field less length field size less version field size
padding_size    = int(header_size - len(magic_number) - signature_len - 4 - 4)
padding_value   = 0

#
# Generate ECC 256 keypair for signing (private) and verification (public)
#
def keygen(output_filename):
    
    private_key, public_key = make_keypair()
    
    # write key to file
    # open the file for binary write
    try:
        f = open(output_filename, "wb")
    except:
        print("ERROR: Cannot open file for writing: " + output_filename)
        sys.exit(2)
    
    # write private key
    # convert local public key to byte string format
    # Code snippet from:
    # https://stackoverflow.com/questions/14043886/python-2-3-convert-integer-to-bytes-cleanly/22074428
    x = private_key
    number_of_bytes = int(math.ceil(x.bit_length() / 8))
    x_bytes = x.to_bytes(number_of_bytes, byteorder='big')
    f.write(x_bytes)
    
    # write public key part 0
    x = public_key[0]
    number_of_bytes = int(math.ceil(x.bit_length() / 8))
    x_bytes = x.to_bytes(number_of_bytes, byteorder='big')
    f.write(x_bytes)
    # write public key part 1
    x = public_key[1]
    number_of_bytes = int(math.ceil(x.bit_length() / 8))
    x_bytes = x.to_bytes(number_of_bytes, byteorder='big')
    f.write(x_bytes)
    
    # Done writing
    f.close() 
    
    return private_key, public_key

#
# Print the public key to the console and copy to the clipboard for pasting into bootloader code
#
def print_key(key_filename):
    # open the keyfile
    try:
        f = open(key_filename, "rb")
    except:
        print("ERROR: Cannot open file for reading: " + key_filename)
        sys.exit(2)

    # seek past the private key
    f.seek(private_key_len)
    public_key_0 = int.from_bytes(f.read(int(public_key_len / 2)), "big")
    public_key_1 = int.from_bytes(f.read(int(public_key_len / 2)), "big")

    outstr = ""
    input = str(hex(public_key_0))[2:] + str(hex(public_key_1))[2:]

    print("Public verification key:")
    for n in range(0, len(input), 2):
        # Add a newline after every 16 values
        if (n > 0) and ((n % 32) == 0):
            print("")
            outstr = outstr + " \ \n"
        
        print("0x" + input[n:n+2] + ", ", end="")
        outstr = outstr + "0x" + input[n:n+2] + ", "

    clipboard.copy(outstr)
    print("\n\nThe key has been copied onto the clipboard ready for pasting into a C file.")

#
# Build the new image from the original image and add:
#   Magic number
#   Signature
#   Length
#   Version
#   Padding
#   Original binary image
#
def create_and_sign_image(input_filename, key_filename, version, output_filename):
    # open the input file
    try:
        f_infile = open(input_filename, "rb")
    except:
        print("ERROR: Cannot open file for reading: " + input_filename)
        sys.exit(2)

    # open the key file
    try:
        f_keyfile = open(key_filename, "rb")
    except:
        print("ERROR: Cannot open file for reading: " + key_filename)
        f_infile.close()
        sys.exit(2)

    # open the output file
    try:
        f_outfile = open(output_filename, "wb")
    except:
        print("ERROR: Cannot open file for writing: " + output_filename)
        f_infile.close()
        f_keyfile.close()
        sys.exit(2)
    
    # read in the original image
    image_orig = f_infile.read()

    image_new = []

    print("Original image length: " + str(len(image_orig)))
    
    # add the magic number
    for mn in magic_number:
        image_new.append(mn)

    # add space for the signature
    for s in range(signature_len):
        image_new.append(0)

    # add the length
    # add 4 for the version number
    x = 4 + len(image_orig) + padding_size
    # snippet from https://stackoverflow.com/questions/6187699/how-to-convert-integer-value-to-array-of-four-bytes-in-python/6187741#6187741
    x_bytes = [x >> i & 0xff for i in (24,16,8,0)]
    # change the endian
    x_bytes.reverse()

    for x in x_bytes:
        image_new.append(x)

    # add the version number
    x_bytes = [version >> i & 0xff for i in (24,16,8,0)]
    # change the endian
    x_bytes.reverse()

    for v in x_bytes:
        image_new.append(v)

    # add the padding
    for i in range(padding_size):
        image_new.append(padding_value)
    
    # add the original image
    for x in image_orig:
        image_new.append(x)

    # read the private signing key from the key file
    private_key = int.from_bytes(f_keyfile.read(int(private_key_len)), "big")

    # sign the new package
    # first element included in the signature is the length so skip over magic number and signature space
    r, s = sign_message(private_key, bytes(image_new[(4 + signature_len):]))

    # write the signature to the new image
    # r
    number_of_bytes = int(math.ceil(r.bit_length() / 8))
    x_bytes = r.to_bytes(number_of_bytes, byteorder='big')
    
    for i in range(0, len(x_bytes)):
        image_new[4 + i] = x_bytes[i]

    # s
    number_of_bytes = int(math.ceil(s.bit_length() / 8))
    x_bytes = s.to_bytes(number_of_bytes, byteorder='big')
    
    for i in range(0, len(x_bytes)):
        image_new[4 + int((signature_len / 2)) + i] = x_bytes[i]

    # write out the new image
    for x in image_new:
        f_outfile.write(x.to_bytes(1, "big"))

    print("New image size: " + str(len(image_new)))

    f_infile.close()
    f_keyfile.close()
    f_outfile.close()

def main(argv):
    # -i input file
    # -o ouput file
    # -k key file
    # -v version number
    # command from sign/keygen/print
    
    parser = argparse.ArgumentParser(description="Sign an image or create and show (print) keys for signing.",
                                    epilog='e.g. Signing:\n \
    \tpython yasb.py sign -i app.bin -k signingkey.bin -v 2 -o app_signed.bin\n\n \
    Generating an ECC secp256r1 keypair:\n \
    \tpython yasb.py keygen -o signingkey.bin\n\n \
    Showing the signing key public part and copying to the clipboard:\n \
    \tpython yasb.py print -k signingkey.bin', formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['sign', 'keygen', 'print'], type=str, help='Operation to perform - sign/keygen/print')
    parser.add_argument('-i', '--inputfile', type=str, help='Input image file to be signed')
    parser.add_argument('-k', '--keyfile', type=str, help='Key file used for signing the image (input only)')
    parser.add_argument('-v', '--version', type=int, help='Version number for the signed image')
    parser.add_argument('-o', '--outputfile', type=str, help='Output file, either the signed image or generated key file')
    args = parser.parse_args()

    missing_arg = False
    
    if (args.command == "print"):
        # check for keyfile arguments
        if (not args.keyfile):
            print("Keyfile not specified. Use -k or -h for help.")
            missing_arg = True

    if (args.command == "keygen"):
        # check for keygen arguments
        if (not args.outputfile):
            print("Output file not specified. Use -o or -h for help.")
            missing_arg = True

    if (args.command == "sign"):
        # check for signing arguments
        if (not args.inputfile):
            print("Input file not specified. Use -i or -h for help.")
            missing_arg = True
        if (not args.outputfile):
            print("Output file not specified. Use -o or -h for help.")
            missing_arg = True
        if (not args.keyfile):
            print("Keyfile not specified. Use -k or -h for help.")
            missing_arg = True
        if (not args.version):
            print("Version number not specified. Use -v or -h for help.")
            missing_arg = True

    if (True == missing_arg):
        sys.exit(2)

    if (args.command == "keygen"):
        priv_key, pub_key = keygen(args.outputfile)

    if (args.command == "print"):
        print_key(args.keyfile)
        print("")

    if (args.command == "sign"):
        create_and_sign_image(args.inputfile, args.keyfile, args.version, args.outputfile)
    
    print("Done")

if __name__ == "__main__":
    main(sys.argv[1:])
