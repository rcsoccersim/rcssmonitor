#!/usr/bin/env python3
"""
A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): convert the
glTF model to a Qt Quick 3D component with balsam, without its Timelines
(the poses are computed from the log every frame, plan 5.8), and with a
property `jersey` for the color of the M_Main material.

usage: make_quick3d_model.py <balsam> <model.glb> <output directory>
"""
import os
import re
import subprocess
import sys

balsam, model, out = sys.argv[1:4]
os.makedirs(out, exist_ok=True)
subprocess.run([balsam, '--outputPath', out, model], check=True, stdout=subprocess.DEVNULL)

name = os.path.splitext(os.path.basename(model))[0] + '.qml'
path = os.path.join(out, name)
lines = open(path, encoding='utf-8').read().split('\n')

kept = []
depth = 0
skipping = False
for line in lines:
    if line.strip() == 'import QtQuick.Timeline':
        continue
    if not skipping and line.startswith('    Timeline {'):
        skipping = True
        depth = 0
    if skipping:
        depth += line.count('{') - line.count('}')
        if depth == 0:
            skipping = False
        continue
    kept.append(line)

text = '\n'.join(kept)
text = text.replace('Node {\n', 'Node {\n    property color jersey: "#ffcc670b"\n', 1)
text = re.sub(r'(id: m_Main_material\n\s*baseColor: )"[^"]*"', r'\1jersey', text)
text = re.sub(r'(id: m_Joints_material\n\s*baseColor: )"[^"]*"', r'\1"#ff404040"', text)
open(path, 'w', encoding='utf-8').write(text)

# the animation data are not used
anim = os.path.join(out, 'animations')
if os.path.isdir(anim):
    for f in os.listdir(anim):
        os.remove(os.path.join(anim, f))
    os.rmdir(anim)
