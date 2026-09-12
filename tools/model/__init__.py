"""tools.model - convert .fbx source models to NanamiEngine's Mv1File asset
(.mv1, DxLib's model format) by driving the DxLibModelViewer GUI tool - the
only tool that can actually load .fbx, since it has no documented CLI/CUI -
and install the result the same way tools/effect does for ParticleFile.

See tools/model/README.md for the command reference and known limitations.
"""
