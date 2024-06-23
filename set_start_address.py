#!/usr/bin/env python3
import argparse

def main():
    parser = argparse.ArgumentParser("pad binary and set start address at end")
    parser.add_argument("-s", "--start-addr", type=int, default=0xF000,
                        help="start address to set default 0xF000")
    parser.add_argument("-o", "--output", help="output filename")
    parser.add_argument("input", help="input binary filename")
    args = parser.parse_args()

    if args.output:
        outfn = args.output
    elif args.input.endswith('.bin'):
        outfn = args.input[:-4] + "_out.bin"
    else:
        outfn = args.input + "_out.bin"

    with open(args.input,'rb') as fd:
        data = bytearray(fd.read(4096))
    print("input data len", len(data))
    data += bytearray(4096 - len(data))
    data[4093] = (args.start_addr >> 8) & 0xFF
    data[4092] = args.start_addr & 0xFF
    print("Saving output to ", outfn)
    with open(outfn, 'wb') as fd:
        fd.write(data)

if __name__ == "__main__":
    main()


