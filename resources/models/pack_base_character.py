#!/usr/bin/env python3
"""Write the human body of the 3D view (BaseCharacter.glb) from the
Universal Base Characters [Standard] of Quaternius (CC0 1.0).

usage: python3 pack_base_character.py <body.gltf> <hair.gltf> <out.glb>

  <body.gltf>  Base Characters/Godot - UE/Superhero_Male_FullBody.gltf
  <hair.gltf>  Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_SimpleParted.gltf

Each .gltf is read with its .bin beside it. The output is one glTF binary
with what the 3D view uses: the skeleton (the nodes and the skin), and the
meshes of the body, the eyebrows, the eyes and the hair, each with its
positions, normals, joints, weights and triangles. The textures, the other
texture coordinates and the vertex colors are left out: the 3D view paints
the body by its parts (the uniform, the skin), and the others in one color
each, set here as their materials' base colors.

The hair is skinned to the same 65 joints (by name) as the body; its mesh
joins the body's skin, whose inverse bind matrices must be the same.
Only the Python standard library is used.
"""

import json
import os
import struct
import sys

# the colors of the materials (linear RGBA, as glTF's baseColorFactor)
COLORS = {
    'MI_Hair_1': [0.10, 0.06, 0.035, 1.0],  # the hair and the eyebrows: dark brown
    'MI_Eyes': [0.06, 0.04, 0.03, 1.0],     # the eyes: dark, without their texture
}
# the attributes kept
ATTRIBUTES = ('POSITION', 'NORMAL', 'JOINTS_0', 'WEIGHTS_0')

COMPONENT_SIZE = {5120: 1, 5121: 1, 5122: 2, 5123: 2, 5125: 4, 5126: 4}
TYPE_COUNT = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def load(path):
    with open(path, 'r', encoding='utf-8') as f:
        gltf = json.load(f)
    buffers = []
    for b in gltf['buffers']:
        with open(os.path.join(os.path.dirname(path), b['uri']), 'rb') as f:
            buffers.append(f.read())
    return gltf, buffers


def accessor_bytes(gltf, buffers, index):
    """the elements of an accessor, packed tight"""
    a = gltf['accessors'][index]
    view = gltf['bufferViews'][a['bufferView']]
    size = COMPONENT_SIZE[a['componentType']] * TYPE_COUNT[a['type']]
    stride = view.get('byteStride', size)
    start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
    data = buffers[view['buffer']]
    if stride == size:
        return data[start:start + size * a['count']]
    return b''.join(data[start + i * stride:start + i * stride + size] for i in range(a['count']))


def matrices(gltf, buffers, index):
    raw = accessor_bytes(gltf, buffers, index)
    values = struct.unpack('<%df' % (len(raw) // 4), raw)
    return [values[i:i + 16] for i in range(0, len(values), 16)]


def used_joints(gltf, buffers):
    """the joints with weight in the meshes"""
    used = set()
    for m in gltf['meshes']:
        for p in m['primitives']:
            fmt = {5121: 'B', 5123: 'H', 5126: 'f'}
            ja = gltf['accessors'][p['attributes']['JOINTS_0']]
            wa = gltf['accessors'][p['attributes']['WEIGHTS_0']]
            jr = accessor_bytes(gltf, buffers, p['attributes']['JOINTS_0'])
            wr = accessor_bytes(gltf, buffers, p['attributes']['WEIGHTS_0'])
            joints = struct.unpack('<%d%s' % (ja['count'] * 4, fmt[ja['componentType']]), jr)
            weights = struct.unpack('<%d%s' % (wa['count'] * 4, fmt[wa['componentType']]), wr)
            used.update(j for j, w in zip(joints, weights) if w > 0)
    return sorted(used)


class Writer:
    """the accessors and the binary chunk of the output"""

    def __init__(self):
        self.bin = bytearray()
        self.views = []
        self.accessors = []

    def add(self, src, buffers, index, target=None):
        a = dict(src['accessors'][index])
        raw = accessor_bytes(src, buffers, index)
        while len(self.bin) % 4:
            self.bin.append(0)
        view = {'buffer': 0, 'byteOffset': len(self.bin), 'byteLength': len(raw)}
        if target is not None:
            view['target'] = target
        self.bin += raw
        self.views.append(view)
        a['bufferView'] = len(self.views) - 1
        a.pop('byteOffset', None)
        a.pop('sparse', None)
        self.accessors.append(a)
        return len(self.accessors) - 1


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    body, body_buffers = load(sys.argv[1])
    hair, hair_buffers = load(sys.argv[2])

    skin = body['skins'][0]
    joint_names = [body['nodes'][j]['name'] for j in skin['joints']]
    hair_skin = hair['skins'][0]
    hair_names = [hair['nodes'][j]['name'] for j in hair_skin['joints']]
    if hair_names != joint_names:
        sys.exit('the hair is not skinned to the joints of the body')
    # the joints that move the hair (the root and the head) must be bound
    # where the body's are (its armature differs in the arms and the legs)
    body_bind = matrices(body, body_buffers, skin['inverseBindMatrices'])
    hair_bind = matrices(hair, hair_buffers, hair_skin['inverseBindMatrices'])
    for j in used_joints(hair, hair_buffers):
        if max(abs(x - y) for x, y in zip(body_bind[j], hair_bind[j])) > 1e-4:
            sys.exit('the hair is bound to %s other than the body' % joint_names[j])

    out = Writer()
    materials = []

    def material(name):
        for i, m in enumerate(materials):
            if m['name'] == name:
                return i
        m = {'name': name, 'pbrMetallicRoughness': {'metallicFactor': 0.0, 'roughnessFactor': 0.8}}
        if name in COLORS:
            m['pbrMetallicRoughness']['baseColorFactor'] = COLORS[name]
        materials.append(m)
        return len(materials) - 1

    def mesh(src, buffers, index):
        prims = []
        for p in src['meshes'][index]['primitives']:
            attributes = {k: out.add(src, buffers, p['attributes'][k], 34962) for k in ATTRIBUTES}
            prims.append({'attributes': attributes,
                          'indices': out.add(src, buffers, p['indices'], 34963),
                          'material': material(src['materials'][p['material']]['name'])})
        return {'name': src['meshes'][index]['name'], 'primitives': prims}

    # the skeleton: the nodes as they are, without the meshes
    nodes = []
    meshes = []
    mesh_nodes = []
    for n in body['nodes']:
        n = dict(n)
        if 'mesh' in n:
            meshes.append(mesh(body, body_buffers, n['mesh']))
            mesh_nodes.append({'name': n['name'], 'mesh': len(meshes) - 1, 'skin': 0})
            n.pop('mesh')
            n.pop('skin', None)
        nodes.append(n)
    for n in hair['nodes']:
        if 'mesh' in n:
            meshes.append(mesh(hair, hair_buffers, n['mesh']))
            mesh_nodes.append({'name': n['name'], 'mesh': len(meshes) - 1, 'skin': 0})
    # the meshes' nodes at the end, beside the armature
    roots = list(body['scenes'][body.get('scene', 0)]['nodes'])
    for m in mesh_nodes:
        nodes.append(m)
        roots.append(len(nodes) - 1)
    # the nodes of the meshes in the body were left in the tree without their meshes: harmless

    out_skin = {'joints': skin['joints'],
                'inverseBindMatrices': out.add(body, body_buffers, skin['inverseBindMatrices'])}
    if 'skeleton' in skin:
        out_skin['skeleton'] = skin['skeleton']

    gltf = {
        'asset': {'version': '2.0',
                  'generator': 'rcssmonitor pack_base_character.py',
                  'copyright': 'Universal Base Characters by Quaternius, CC0 1.0'},
        'scene': 0,
        'scenes': [{'nodes': roots}],
        'nodes': nodes,
        'meshes': meshes,
        'materials': materials,
        'skins': [out_skin],
        'accessors': out.accessors,
        'bufferViews': out.views,
        'buffers': [{'byteLength': len(out.bin)}],
    }
    text = json.dumps(gltf, separators=(',', ':')).encode('utf-8')
    text += b' ' * ((4 - len(text) % 4) % 4)
    while len(out.bin) % 4:
        out.bin.append(0)
    total = 12 + 8 + len(text) + 8 + len(out.bin)
    with open(sys.argv[3], 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<II', len(text), 0x4E4F534A))
        f.write(text)
        f.write(struct.pack('<II', len(out.bin), 0x004E4942))
        f.write(out.bin)

    tris = sum(out.accessors[p['indices']]['count'] // 3 for m in meshes for p in m['primitives'])
    print('%s: %d bytes, %d meshes, %d triangles, %d joints, materials %s'
          % (sys.argv[3], total, len(meshes), tris, len(skin['joints']), [m['name'] for m in materials]))


if __name__ == '__main__':
    main()
