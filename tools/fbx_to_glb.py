# fbx_to_glb.py: an FBX file (a model, its animations) to a glTF binary, with Blender
# (docs/dev/FIGURE_ASSETS.md):
#
#   blender -b --factory-startup --python tools/fbx_to_glb.py -- IN.fbx OUT.glb
#
# rcssmonitor reads FBX files as they are; this is for editing a model as glTF.
# The leaf bones are kept (a model's Head may be one); no compression is
# written (rcssmonitor does not read KHR_draco_mesh_compression).
import sys

import bpy

src, out = sys.argv[sys.argv.index("--") + 1:][:2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=src, ignore_leaf_bones=False, automatic_bone_orientation=False)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_yup=True, export_animations=True,
                          export_apply=False, export_draco_mesh_compression_enable=False)
print("CONVERTED", src, "->", out)
