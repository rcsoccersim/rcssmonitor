# Makes the FBX files of the tests from the bundled Mannequin, with Blender (5.3):
#
#   blender -b --factory-startup --python test/data/fbx/make_fbx.py -- \
#       resources/figures/mannequin/Mannequin.glb test/data/fbx
#
# The Mannequin's bones of the humanoid roles get Mixamo's names
# (mixamorig:Hips, mixamorig:LeftArm, ...); the other bones keep theirs.
#
# figure.fbx: the Mannequin's body with fewer triangles, its body material
#   with a small texture embedded (one color, the UVs all 0); no animation.
#   As Blender writes a file in meters by default: the values in meters,
#   the root nodes scaled 100 times, the unit 1 cm, turned to Y up.
# walk.fbx: the skeleton of the role bones only (no mesh, no fingers), the clip Walk_Loop, the hips going
#   forward by a stride (1.28 m) each cycle: not in place, as a Mixamo
#   animation downloaded without "In Place". In centimeters as Mixamo's
#   files: the skeleton itself 100 times larger, the values in cm.
# plane.fbx, plane_zup.fbx, Holes.png: see below.
import os
import sys

import bpy
from bpy_extras import anim_utils
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
SOURCE, OUT = argv[0], argv[1]

MIXAMO = {
    "pelvis": "Hips", "spine_01": "Spine", "spine_02": "Spine1", "spine_03": "Spine2",
    "neck_01": "Neck", "Head": "Head",
    "clavicle_l": "LeftShoulder", "upperarm_l": "LeftArm", "lowerarm_l": "LeftForeArm", "hand_l": "LeftHand",
    "clavicle_r": "RightShoulder", "upperarm_r": "RightArm", "lowerarm_r": "RightForeArm", "hand_r": "RightHand",
    "thigh_l": "LeftUpLeg", "calf_l": "LeftLeg", "foot_l": "LeftFoot", "ball_l": "LeftToeBase",
    "thigh_r": "RightUpLeg", "calf_r": "RightLeg", "foot_r": "RightFoot", "ball_r": "RightToeBase",
}
STRIDE = 1.28  # m a cycle of the walk


def load():
    """the Mannequin, its role bones of Mixamo's names; its body only"""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=SOURCE)
    for o in list(bpy.data.objects):
        if o.type == 'MESH' and o.name != 'Mannequin':
            bpy.data.objects.remove(o)
    arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    for b in arm.data.bones:
        if b.name in MIXAMO:
            b.name = "mixamorig:" + MIXAMO[b.name]
    return arm


def select_only(objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]


# figure.fbx
arm = load()
mesh = bpy.data.objects['Mannequin']
select_only([mesh])
dec = mesh.modifiers.new("Decimate", 'DECIMATE')
dec.ratio = 0.06
bpy.ops.object.modifier_move_to_index(modifier="Decimate", index=0)
bpy.ops.object.modifier_apply(modifier="Decimate")
image = bpy.data.images.new("Skin", 4, 4)
image.pixels = [0.8, 0.5, 0.2, 1.0] * 16
image.filepath_raw = OUT + "/Skin.png"
image.file_format = 'PNG'
image.save()
image.pack()
mat = bpy.data.materials['M_Main']
mat.use_nodes = True
bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
tex = mat.node_tree.nodes.new('ShaderNodeTexImage')
tex.image = image
mat.node_tree.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
if not mesh.data.uv_layers:
    mesh.data.uv_layers.new(name="UVMap")
bpy.ops.export_scene.fbx(filepath=OUT + "/figure.fbx", add_leaf_bones=False, bake_anim=False,
                         path_mode='COPY', embed_textures=True)
os.remove(OUT + "/Skin.png")  # embedded in figure.fbx

# walk.fbx
arm = load()
bpy.data.objects.remove(bpy.data.objects['Mannequin'])
act = bpy.data.actions['Walk_Loop']
arm.animation_data.action = act
select_only([arm])
# the role bones only (and the root above them): a smaller file
bpy.ops.object.mode_set(mode='EDIT')
for b in list(arm.data.edit_bones):
    if b.name != "root" and not b.name.startswith("mixamorig:"):
        arm.data.edit_bones.remove(b)
bpy.ops.object.mode_set(mode='OBJECT')
arm.scale = (100.0, 100.0, 100.0)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
channels = anim_utils.action_get_channelbag_for_slot(act, arm.animation_data.action_slot)
for fc in channels.fcurves:
    if fc.data_path.endswith(".location"):
        for k in fc.keyframe_points:
            k.co.y *= 100.0
            k.handle_left.y *= 100.0
            k.handle_right.y *= 100.0
# the hips forward (the Mannequin faces -y in Blender) by a stride a cycle, in the bone's rest frame
hips = arm.data.bones["mixamorig:Hips"]
first, last = act.frame_range
forward = hips.matrix_local.to_3x3().inverted() @ Vector((0.0, -STRIDE * 100.0, 0.0))
for fc in channels.fcurves:
    if fc.data_path == 'pose.bones["mixamorig:Hips"].location':
        for k in fc.keyframe_points:
            k.interpolation = 'LINEAR'
            k.co.y += forward[fc.array_index] * (k.co.x - first) / (last - first)
scene = bpy.context.scene
scene.frame_start, scene.frame_end = int(first), int(last)
scene.unit_settings.scale_length = 0.01
bpy.ops.export_scene.fbx(filepath=OUT + "/walk.fbx", object_types={'ARMATURE'}, add_leaf_bones=False,
                         bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                         bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0)
made = "frames %d..%d at %d fps" % (first, last, scene.render.fps)

# plane.fbx: a square (1 m, no skin) on a node moved by (1, 0, 2) m, its
#   material with the texture Holes.png beside it (not embedded; 4x2
#   pixels: the left half opaque green, the right half transparent)
bpy.ops.wm.read_factory_settings(use_empty=True)
holes = bpy.data.images.new("Holes", 4, 2, alpha=True)
holes.pixels = ([0.0, 1.0, 0.0, 1.0] * 2 + [0.0, 0.0, 1.0, 0.0] * 2) * 2  # rows from the bottom
holes.filepath_raw = OUT + "/Holes.png"
holes.file_format = 'PNG'
holes.save()
bpy.ops.mesh.primitive_plane_add(size=1.0)
square = bpy.context.active_object
square.name = "Square"
holder = bpy.data.objects.new("Holder", None)
bpy.context.scene.collection.objects.link(holder)
holder.location = (1.0, -2.0, 0.0)  # Blender's -y: +z in Y up
square.parent = holder
mat = bpy.data.materials.new("Holes")
mat.use_nodes = True
bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
tex = mat.node_tree.nodes.new('ShaderNodeTexImage')
tex.image = holes
mat.node_tree.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
mat.node_tree.links.new(tex.outputs['Alpha'], bsdf.inputs['Alpha'])
square.data.materials.append(mat)
bpy.ops.export_scene.fbx(filepath=OUT + "/plane.fbx", path_mode='RELATIVE', embed_textures=False)
# plane_zup.fbx: the same written z up, y ahead (as some tools write)
bpy.ops.export_scene.fbx(filepath=OUT + "/plane_zup.fbx", path_mode='RELATIVE', embed_textures=False,
                         axis_up='Z', axis_forward='Y')

print("MADE", OUT, made)
