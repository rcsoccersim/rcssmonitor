#!/usr/bin/env python3
"""Write the bundled clips of the figures' actions from the Universal
Animation Library 2 [Standard] of Quaternius (CC0 1.0), with Python's
standard library only:

  python3 resources/figures/mannequin/pack_ual2_actions.py \\
      "Unreal-Godot/UAL2_Standard.glb" resources/figures/mannequin/Actions.glb

UAL2_Standard.glb is the library's file without root motion (the clips in
place). The output is a glTF binary of the skeleton (its nodes, without
the mesh and the skin) and the clips the 3D view plays for the actions:

  Slide_Start  the sliding tackle: from running, feet first onto the ground
  Slide_Exit   getting up from the slide
  LayToIdle    getting up from lying on the back (after a foul, a dive)

(Hit_Knockback, a fall back, is not taken: the figure flies back with its
feet higher than a meter, unlike a player tripped by a foul, who sits down
as Figures3D poses it.)

A channel that keeps a node at its rest all through a clip is left out
(the 3D view keeps a node without keys at its rest).
"""

import json
import struct
import sys

CLIPS = ('Slide_Start', 'Slide_Exit', 'LayToIdle')
EPSILON = 1.0e-5

COMPONENT_SIZE = {5120: 1, 5121: 1, 5122: 2, 5123: 2, 5125: 4, 5126: 4}
TYPE_COUNT = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}
REST = {'translation': [0.0, 0.0, 0.0], 'rotation': [0.0, 0.0, 0.0, 1.0], 'scale': [1.0, 1.0, 1.0]}


def read_glb(path):
    with open(path, 'rb') as f:
        data = f.read()
    magic, version, _ = struct.unpack('<III', data[:12])
    if magic != 0x46546C67 or version != 2:
        sys.exit('%s: not a glTF binary of version 2' % path)
    gltf, binary = None, b''
    at = 12
    while at < len(data):
        length, kind = struct.unpack('<II', data[at:at + 8])
        chunk = data[at + 8:at + 8 + length]
        if kind == 0x4E4F534A:
            gltf = json.loads(chunk)
        elif kind == 0x004E4942:
            binary = chunk
        at += 8 + length
    return gltf, binary


def floats(gltf, binary, index):
    """the floats of an accessor, as a list of elements"""
    a = gltf['accessors'][index]
    if a['componentType'] != 5126:
        sys.exit('accessor %d: not of floats' % index)
    view = gltf['bufferViews'][a['bufferView']]
    count = TYPE_COUNT[a['type']]
    stride = view.get('byteStride', 4 * count)
    start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
    out = []
    for k in range(a['count']):
        at = start + k * stride
        out.append(list(struct.unpack('<%df' % count, binary[at:at + 4 * count])))
    return out


def constant(values, rest, path):
    for v in values:
        if path == 'rotation':
            # q and -q are the same turn
            d = min(max(abs(a - b) for a, b in zip(v, rest)), max(abs(a + b) for a, b in zip(v, rest)))
        else:
            d = max(abs(a - b) for a, b in zip(v, rest))
        if d > EPSILON:
            return False
    return True


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: pack_ual2_actions.py UAL2_Standard.glb OUT.glb')
    gltf, binary = read_glb(sys.argv[1])

    # the nodes but those of a mesh, renumbered
    keep = [i for i, n in enumerate(gltf['nodes']) if 'mesh' not in n and 'skin' not in n]
    new_index = {old: new for new, old in enumerate(keep)}
    nodes = []
    for old in keep:
        n = {k: v for k, v in gltf['nodes'][old].items() if k in ('name', 'translation', 'rotation', 'scale', 'children')}
        if 'children' in n:
            n['children'] = [new_index[c] for c in n['children'] if c in new_index]
            if not n['children']:
                del n['children']
        nodes.append(n)
    roots = [new_index[r] for r in gltf['scenes'][gltf.get('scene', 0)]['nodes'] if r in new_index]

    out = bytearray()
    accessors, views, animations = [], [], []

    def add(values, kind, minmax=False):
        view = len(views)
        data = b''.join(struct.pack('<%df' % len(v), *v) for v in values)
        views.append({'buffer': 0, 'byteOffset': len(out), 'byteLength': len(data)})
        out.extend(data)
        while len(out) % 4:
            out.append(0)
        a = {'bufferView': view, 'componentType': 5126, 'count': len(values), 'type': kind}
        if minmax:
            a['min'] = [min(v[0] for v in values)]
            a['max'] = [max(v[0] for v in values)]
        accessors.append(a)
        return len(accessors) - 1

    for name in CLIPS:
        found = [a for a in gltf['animations'] if a['name'] == name]
        if not found:
            sys.exit('%s: no clip %s' % (sys.argv[1], name))
        anim = found[0]
        channels, samplers = [], []
        inputs = {}
        for ch in anim['channels']:
            node = ch['target']['node']
            path = ch['target']['path']
            if node not in new_index:
                continue
            s = anim['samplers'][ch['sampler']]
            values = floats(gltf, binary, s['output'])
            rest = gltf['nodes'][node].get(path, REST[path])
            if constant(values, rest, path):
                continue
            if s['input'] not in inputs:
                inputs[s['input']] = add(floats(gltf, binary, s['input']), 'SCALAR', True)
            output = add(values, 'VEC4' if path == 'rotation' else 'VEC3')
            samplers.append({'input': inputs[s['input']], 'output': output,
                             'interpolation': s.get('interpolation', 'LINEAR')})
            channels.append({'sampler': len(samplers) - 1,
                             'target': {'node': new_index[node], 'path': path}})
        animations.append({'name': name, 'channels': channels, 'samplers': samplers})
        print('%s: %d channels' % (name, len(channels)))

    doc = {
        'asset': {'version': '2.0',
                  'generator': 'pack_ual2_actions.py (Universal Animation Library 2 [Standard], Quaternius, CC0 1.0)'},
        'scene': 0,
        'scenes': [{'name': 'Scene', 'nodes': roots}],
        'nodes': nodes,
        'animations': animations,
        'accessors': accessors,
        'bufferViews': views,
        'buffers': [{'byteLength': len(out)}],
    }
    text = json.dumps(doc, separators=(',', ':')).encode('utf-8')
    while len(text) % 4:
        text += b' '
    total = 12 + 8 + len(text) + 8 + len(out)
    with open(sys.argv[2], 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<II', len(text), 0x4E4F534A))
        f.write(text)
        f.write(struct.pack('<II', len(out), 0x004E4942))
        f.write(out)
    print('%s: %d bytes' % (sys.argv[2], total))


if __name__ == '__main__':
    main()
