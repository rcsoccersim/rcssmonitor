// Write a glTF binary without EXT_meshopt_compression and KHR_mesh_quantization.
// usage: node decompress.mjs <in.glb> <out.glb>
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS, EXTMeshoptCompression, KHRMeshQuantization } from '@gltf-transform/extensions';
import { dequantize, prune } from '@gltf-transform/functions';
import { MeshoptDecoder } from 'meshoptimizer';

const [input, output] = process.argv.slice(2);
await MeshoptDecoder.ready;
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS).registerDependencies({ 'meshopt.decoder': MeshoptDecoder });
const doc = await io.read(input);
await doc.transform(dequantize(), prune());
for (const ext of doc.getRoot().listExtensionsUsed()) {
  if (ext instanceof EXTMeshoptCompression || ext instanceof KHRMeshQuantization) ext.dispose();
}
await io.write(output, doc);

const root = doc.getRoot();
console.log('extensions:', root.listExtensionsUsed().map((e) => e.extensionName));
console.log('meshes:', root.listMeshes().map((m) => `${m.getName()} (${m.listPrimitives().length} primitives)`));
console.log('materials:', root.listMaterials().map((m) => m.getName()));
console.log('skins:', root.listSkins().map((s) => `${s.getName()} ${s.listJoints().length} joints`));
console.log('animations:', root.listAnimations().map((a) => `${a.getName()} (${a.listChannels().length} channels)`));
for (const p of root.listMeshes()[0].listPrimitives()) {
  console.log('primitive attributes:', p.listSemantics().map((s) => `${s}:${p.getAttribute(s).getComponentSize()*8}bit x${p.getAttribute(s).getElementSize()}`).join(' '), 'indices', p.getIndices()?.getCount());
}
