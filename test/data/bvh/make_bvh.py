# Makes the BVH file of the tests from the bundled Mannequin, with Blender (5.3):
#
#   blender -b --factory-startup --python test/data/bvh/make_bvh.py -- \
#       resources/figures/mannequin/Mannequin.glb test/data/bvh
#
# walk.bvh: the skeleton of the Mannequin's role bones (and the root above
#   them; no fingers), the clip Walk_Loop, written by Blender's BVH
#   exporter: in centimeters (scaled 100 times), the rotations in the
#   order Y X Z (as 100STYLE's files), every joint with position channels
#   (the root "root" above the hips stays at the origin; the hips move).
#   Blender writes its own axes (z up) and the bones' rest as it holds
#   them; the tests read it as any BVH file.
import sys

import bpy

argv = sys.argv[sys.argv.index("--") + 1:]
SOURCE, OUT = argv[0], argv[1]

ROLES = {
    "pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "Head",
    "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l",
    "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",
    "thigh_l", "calf_l", "foot_l", "ball_l",
    "thigh_r", "calf_r", "foot_r", "ball_r",
}

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=SOURCE)
for o in list(bpy.data.objects):
    if o.type == 'MESH':
        bpy.data.objects.remove(o)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
act = bpy.data.actions['Walk_Loop']
arm.animation_data.action = act
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
bpy.context.view_layer.objects.active = arm
# the role bones only (and the root above them): a smaller file
bpy.ops.object.mode_set(mode='EDIT')
for b in list(arm.data.edit_bones):
    if b.name != "root" and b.name not in ROLES:
        arm.data.edit_bones.remove(b)
bpy.ops.object.mode_set(mode='OBJECT')
first, last = act.frame_range
scene = bpy.context.scene
scene.frame_start, scene.frame_end = int(first), int(last)
bpy.ops.export_anim.bvh(filepath=OUT + "/walk.bvh", global_scale=100.0, frame_start=int(first),
                        frame_end=int(last), rotate_mode='YXZ', root_transform_only=False)
print("MADE", OUT, "frames %d..%d at %d fps" % (first, last, scene.render.fps))
