import argparse
import math
import shutil
import struct
import subprocess
import tempfile
from itertools import product
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

LOWER = np.array([-0.065, -0.08, -0.065])
UPPER = np.array([0.065, 0.1, 0.065])
CENTER = (LOWER + UPPER) / 2
WIDTH, HEIGHT = 520, 500
FOCAL = 900.0
DISTANCE = 0.5
PITCH = math.radians(28)
YAW_START, YAW_END = 30.0, 75.0
RADIUS = 2.6
BACKGROUND = (13, 17, 30)
BACK_EDGE = (45, 54, 78)
FRONT_EDGE = (95, 110, 145)
SLOW = np.array([25, 85, 200])
FAST = np.array([190, 235, 255])
TOP_SPEED = 1.5


def read_state(path):
    data = path.read_bytes()
    _, count = struct.unpack("<fi", data[:8])
    values = np.frombuffer(data[8:], dtype="<f4").reshape(count, 9).astype(np.float64)
    return values[:, 0:3], values[:, 6:9]


def project(points, yaw):
    relative = np.atleast_2d(points) - CENTER
    cos_yaw, sin_yaw = math.cos(yaw), math.sin(yaw)
    x = relative[:, 0] * cos_yaw - relative[:, 2] * sin_yaw
    z = relative[:, 0] * sin_yaw + relative[:, 2] * cos_yaw
    y = relative[:, 1] * math.cos(PITCH) + z * math.sin(PITCH)
    z = z * math.cos(PITCH) - relative[:, 1] * math.sin(PITCH)
    depth = DISTANCE + z
    return WIDTH / 2 + FOCAL * x / depth, HEIGHT * 0.46 - FOCAL * y / depth, depth


def box_edges():
    corners = [np.array(c) for c in product(*zip(LOWER, UPPER))]
    return [
        (a, b)
        for i, a in enumerate(corners)
        for b in corners[i + 1 :]
        if np.count_nonzero(a != b) == 1
    ]


def draw_edges(draw, yaw, front):
    for a, b in box_edges():
        midpoint_depth = project((a + b) / 2, yaw)[2][0]
        if (midpoint_depth < DISTANCE) == front:
            ax, ay, _ = project(a, yaw)
            bx, by, _ = project(b, yaw)
            color = FRONT_EDGE if front else BACK_EDGE
            draw.line([(ax[0], ay[0]), (bx[0], by[0])], fill=color, width=2)


def render(positions, velocities, yaw):
    image = Image.new("RGB", (WIDTH, HEIGHT), BACKGROUND)
    draw = ImageDraw.Draw(image)
    draw_edges(draw, yaw, front=False)
    screen_x, screen_y, depth = project(positions, yaw)
    speed = np.clip(np.linalg.norm(velocities, axis=1) / TOP_SPEED, 0, 1)
    nearest, farthest = depth.min(), depth.max()
    shade = 1.0 - 0.45 * (depth - nearest) / max(farthest - nearest, 1e-9)
    for index in np.argsort(-depth):
        color = tuple(int(c) for c in (SLOW + (FAST - SLOW) * speed[index]) * shade[index])
        radius = RADIUS * DISTANCE / depth[index]
        x, y = screen_x[index], screen_y[index]
        draw.ellipse([x - radius, y - radius, x + radius, y + radius], fill=color)
    draw_edges(draw, yaw, front=True)
    return image


def main():
    parser = argparse.ArgumentParser(description="Render a 3D dam-break simulation to a GIF.")
    parser.add_argument("fluid", type=Path, help="path to the fluid executable")
    parser.add_argument("output", type=Path)
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--steps-per-frame", type=int, default=5)
    parser.add_argument("--frame-ms", type=int, default=40)
    args = parser.parse_args()

    scripts = Path(__file__).resolve().parent
    yaws = np.radians(np.linspace(YAW_START, YAW_END, args.frames + 1))
    with tempfile.TemporaryDirectory() as folder:
        state = Path(folder) / "state.fld"
        following = Path(folder) / "next.fld"
        subprocess.run(["python", str(scripts / "make_dam_break.py"), str(state)], check=True)
        frames = [render(*read_state(state), yaws[0])]
        for yaw in yaws[1:]:
            subprocess.run(
                [str(args.fluid), str(args.steps_per_frame), str(state), str(following)],
                check=True,
                stdout=subprocess.DEVNULL,
            )
            shutil.move(following, state)
            frames.append(render(*read_state(state), yaw))

    palette = [frame.quantize(colors=40, method=Image.Quantize.MEDIANCUT) for frame in frames]
    palette[0].save(
        args.output,
        save_all=True,
        append_images=palette[1:] + [palette[-1]] * 25,
        duration=args.frame_ms,
        loop=0,
        optimize=True,
    )
    print(f"{len(frames)} frames written to {args.output}")


if __name__ == "__main__":
    main()
