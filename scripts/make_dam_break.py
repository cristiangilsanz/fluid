import argparse
import struct
from pathlib import Path

LOWER = (-0.065, -0.08, -0.065)
UPPER = (0.065, 0.1, 0.065)


def lattice(particles_per_meter, width, height, depth):
    spacing = 1.0 / particles_per_meter
    margin = spacing / 2
    count_x = int(width / spacing)
    count_y = int(height / spacing)
    count_z = int(depth / spacing)
    for i in range(count_x):
        for j in range(count_y):
            for k in range(count_z):
                yield (
                    LOWER[0] + margin + i * spacing,
                    LOWER[1] + margin + j * spacing,
                    LOWER[2] + margin + k * spacing,
                )


def main():
    parser = argparse.ArgumentParser(description="Write a dam-break initial state as a .fld file.")
    parser.add_argument("output", type=Path)
    parser.add_argument("--particles-per-meter", type=float, default=204.0)
    parser.add_argument("--width", type=float, default=0.065)
    parser.add_argument("--height", type=float, default=0.1)
    parser.add_argument("--depth", type=float, default=0.065)
    args = parser.parse_args()

    positions = list(lattice(args.particles_per_meter, args.width, args.height, args.depth))
    with args.output.open("wb") as file:
        file.write(struct.pack("<fi", args.particles_per_meter, len(positions)))
        for position in positions:
            file.write(struct.pack("<9f", *position, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0))
    print(f"{len(positions)} particles written to {args.output}")


if __name__ == "__main__":
    main()
