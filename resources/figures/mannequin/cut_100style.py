#!/usr/bin/env python3
# Cuts the bundled clips of walking and jogging backward and sideways from
# the 100STYLE dataset (Ian Mason, CC BY 4.0,
# https://www.ianxmason.com/100style/), with Python's standard library only:
#
#   python3 resources/figures/mannequin/cut_100style.py 100STYLE.zip \
#       resources/figures/mannequin
#
# 100STYLE.zip is the dataset's BVH files (https://zenodo.org/records/8127870).
# From the style Neutral, each clip is one cycle of the gait (two steps):
# from the left foot coming down to it coming down again, a cycle going
# straight the way of the clip (found by its feet and its hips; the frames
# below). The cycle is
#   - looped: what each channel is off at the end from the start (but where
#     the hips go across the floor) is taken off in a line over the cycle,
#     so that the end meets the start (and the hips' heading does not drift);
#   - thinned from 60 to about 30 frames a second: resampled at as many
#     frames as fit the cycle exactly (its Frame Time);
#   - written with the file's own skeleton (XSens MVN, in centimeters).
# A side of which only one clip is cut is mirrored by rcssmonitor (the
# figure pack's motion tells one side: the other side's clip is made).
import io
import sys
import zipfile

# name: (the file's move, the first frame, the last frame: the left foot
# coming down again), frames of the motion counted from 0
CLIPS = {
    "WalkBack": ("BW", 6514, 6585),   # 1.18 s, 0.56 m/s, 179 degrees from facing
    "JogBack": ("BR", 2755, 2795),    # 0.67 s, 1.00 m/s, 174 degrees
    "WalkRight": ("SW", 5064, 5133),  # 1.15 s, 0.51 m/s, 88 degrees to the right
    "JogLeft": ("SR", 547, 591),      # 0.73 s, 1.44 m/s, 88 degrees to the left
}
RATE = 30.0  # frames a second, about


def read(text):
    """the hierarchy's lines, the channels (joint, kind) and the frames"""
    lines = text.splitlines()
    m = next(i for i, l in enumerate(lines) if l.strip() == "MOTION")
    channels = []
    joint = None
    for l in lines[:m]:
        w = l.split()
        if w and w[0] in ("ROOT", "JOINT"):
            joint = w[1]
        elif w and w[0] == "CHANNELS":
            channels += [(joint, c) for c in w[2:]]
    frame_time = float(lines[m + 2].split()[2])
    frames = [[float(v) for v in l.split()] for l in lines[m + 3:] if l.strip()]
    return lines[:m + 1], channels, frame_time, frames


def unwrap(values):
    """angles made continuous (no jumps of 360 degrees)"""
    out = [values[0]]
    for v in values[1:]:
        p = out[-1]
        out.append(v + 360.0 * round((p - v) / 360.0))
    return out


def cut(text, first, last):
    header, channels, frame_time, frames = read(text)
    root = channels[0][0]
    cycle = frames[first:last + 1]
    span = (last - first) * frame_time
    n = max(1, round(span * RATE))
    out_time = span / n
    columns = []
    for c, (joint, kind) in enumerate(channels):
        v = [f[c] for f in cycle]
        if kind.endswith("rotation"):
            v = unwrap(v)
        # the end meets the start, but where the hips go across the floor
        if not (joint == root and kind in ("Xposition", "Zposition")):
            off = v[-1] - v[0]
            v = [x - off * k / (len(v) - 1) for k, x in enumerate(v)]
        # resampled: n intervals over the cycle
        r = []
        for k in range(n + 1):
            t = k * out_time / frame_time
            i = min(int(t), len(v) - 2)
            f = t - i
            r.append(v[i] + (v[i + 1] - v[i]) * f)
        columns.append(r)
    out = io.StringIO()
    out.write("\n".join(header) + "\n")
    out.write("Frames: %d\nFrame Time: %.7f\n" % (n + 1, out_time))
    for k in range(n + 1):
        out.write(" ".join("%.4f" % col[k] for col in columns) + "\n")
    return out.getvalue(), n + 1, out_time


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: cut_100style.py 100STYLE.zip OUT_DIR")
    source, out_dir = sys.argv[1], sys.argv[2]
    with zipfile.ZipFile(source) as z:
        for name, (move, first, last) in CLIPS.items():
            text = z.read("100STYLE/Neutral/Neutral_%s.bvh" % move).decode("ascii")
            data, frames, frame_time = cut(text, first, last)
            with open("%s/%s.bvh" % (out_dir, name), "w", newline="\n") as f:
                f.write(data)
            print("%s.bvh: Neutral_%s.bvh frames %d..%d, %d frames at %.4f s"
                  % (name, move, first, last, frames, frame_time))


if __name__ == "__main__":
    main()
