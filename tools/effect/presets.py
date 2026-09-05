"""Curated, sample-derived builders for new ``.efkproj`` content.

Effekseer's real schema is hundreds of fields across dozens of node kinds;
most fields in any given file are only present because they differ from the
editor's default. Rather than guess at a complete typed schema, this module
gives you:

* :func:`elem` - a small generic composer for building any nested tag shape
  (leaf text, or nested children from either raw values or other ``Elem``s).
* value-shape helpers (:func:`fixed_axes`, :func:`pva`, :func:`easing`) for
  the "distribution" shapes seen across the 14 real AndrewFM01 samples this
  toolkit was built from.
* common node-block builders (``common_values``, ``location_values``, ...)
  and node builders (:func:`sprite_node`, :func:`ring_node`,
  :func:`ribbon_node`, :func:`group_node`) assembled from real field names
  and defaults observed in those samples.

**v1 scope**: only the ``Sprite`` / ``Ring`` / ``Ribbon`` ``DrawingValues``
kinds are modeled - the 3 kinds actually evidenced across the 14 samples
(``Sprite`` x34, ``Ring`` x49, ``Ribbon`` x8). ``Model`` / ``Track`` and other
kinds have no real example to crib field names/defaults from; add them the
same way this v1 was built, from a real sample, when one is available.
"""

from __future__ import annotations

from .model import Elem

# ---------------------------------------------------------------------------
# generic composition
def _fmt(value) -> str:
    if isinstance(value, bool):
        return "True" if value else "False"
    if isinstance(value, float):
        if value.is_integer():
            return str(int(value))
        return repr(value)
    return str(value)


def elem(tag: str, *, text=None, children: list[Elem] | None = None, **leaf_children) -> Elem:
    """Build an :class:`Elem`.

    ``elem("X", text=1.8)`` -> a text leaf ``<X>1.8</X>``.
    ``elem("Location", X=1.8)`` -> ``<Location><X>1.8</X></Location>``.
    ``elem("Outer_Fixed", Location=elem("Location", X=1.8))`` nests an
    already-built ``Elem`` (keyword name is only for readability at the call
    site - the child's own ``.tag`` is what's written).
    """
    e = Elem(tag)
    if children:
        e.children.extend(children)
    for key, val in leaf_children.items():
        if isinstance(val, Elem):
            e.children.append(val)
        elif val is not None:
            e.children.append(Elem(key, text=_fmt(val)))
    if text is not None and not e.children:
        e.text = _fmt(text)
    return e


# ---------------------------------------------------------------------------
# value-shape helpers
def fixed_axes(tag: str, **axes) -> Elem:
    """A ``Tag_Fixed``-style leaf with per-axis values, e.g.
    ``fixed_axes("ColorAll_Fixed", R=0, G=0, B=0, A=255)``.
    """
    return elem(tag, **axes)


def pva(tag: str, *, x=None, y=None, z=None, center=None, max=None, min=None,
        drawn_as: int | None = None) -> Elem:
    """Either a single scalar PVA block (pass ``center``/``max``/``min``
    directly) or a per-axis PVA block (pass ``x=``/``y=``/``z=`` as dicts of
    ``{"center":..,"max":..,"min":..}``), matching both shapes seen in the
    samples (e.g. ``ScalingValues.PVA.Scale.X`` vs a bare scalar PVA).
    """
    e = Elem(tag)
    axes = {"X": x, "Y": y, "Z": z}
    if any(v is not None for v in axes.values()):
        for name, spec in axes.items():
            if spec is None:
                continue
            e.children.append(elem(name, **spec))
        if drawn_as is not None:
            e.children.append(Elem("DrawnAs", text=_fmt(drawn_as)))
        return e
    if center is not None:
        e.children.append(Elem("Center", text=_fmt(center)))
    if max is not None:
        e.children.append(Elem("Max", text=_fmt(max)))
    if min is not None:
        e.children.append(Elem("Min", text=_fmt(min)))
    if drawn_as is not None:
        e.children.append(Elem("DrawnAs", text=_fmt(drawn_as)))
    return e


def easing(tag: str, start: Elem | None = None, end: Elem | None = None) -> Elem:
    e = Elem(tag)
    if start is not None:
        e.children.append(elem("Start", children=start.children) if start.tag != "Start" else start)
    if end is not None:
        e.children.append(elem("End", children=end.children) if end.tag != "End" else end)
    return e


def color(tag: str, r=None, g=None, b=None, a=None) -> Elem:
    return elem(tag, R=r, G=g, B=b, A=a)


def xyz(tag: str, x=None, y=None, z=None) -> Elem:
    return elem(tag, X=x, Y=y, Z=z)


# ---------------------------------------------------------------------------
# common Node child-block builders
def common_values(*, max_generation=None, infinite: bool | None = None,
                   generation_time=None, generation_time_offset=None,
                   life=None, remove_when_life_extinct: bool | None = None,
                   remove_when_parent_removed: bool | None = None) -> Elem:
    e = Elem("CommonValues")
    if max_generation is not None or infinite is not None:
        mg = Elem("MaxGeneration")
        if max_generation is not None:
            mg.children.append(Elem("Value", text=_fmt(max_generation)))
        if infinite is not None:
            mg.children.append(Elem("Infinite", text=_fmt(infinite)))
        e.children.append(mg)
    if generation_time is not None:
        e.children.append(pva("GenerationTime", **generation_time))
    if generation_time_offset is not None:
        e.children.append(pva("GenerationTimeOffset", **generation_time_offset))
    if life is not None:
        e.children.append(pva("Life", **life))
    if remove_when_life_extinct is not None:
        e.children.append(Elem("RemoveWhenLifeIsExtinct", text=_fmt(remove_when_life_extinct)))
    if remove_when_parent_removed is not None:
        e.children.append(Elem("RemoveWhenParentIsRemoved", text=_fmt(remove_when_parent_removed)))
    return e


def location_values(*, fixed_xyz: dict | None = None, velocity: Elem | None = None,
                     acceleration: Elem | None = None) -> Elem:
    """``LocationValues``. ``fixed_xyz`` is a plain ``{"X":.., "Y":.., "Z":..}``
    dict for a ``Type=0``/``Fixed`` block; ``velocity``/``acceleration`` are
    ``xyz("Velocity", ...)``/``xyz("Acceleration", ...)``-shaped ``Elem``s for
    a ``Type=1``/``PVA`` block.
    """
    e = Elem("LocationValues")
    if fixed_xyz is not None:
        e.children.append(Elem("Type", text="0"))
        e.children.append(elem("Fixed", Location=xyz("Location", **fixed_xyz)))
    elif velocity is not None or acceleration is not None:
        e.children.append(Elem("Type", text="1"))
        pva_block = Elem("PVA")
        if velocity is not None:
            pva_block.children.append(velocity)
        if acceleration is not None:
            pva_block.children.append(acceleration)
        e.children.append(pva_block)
    return e


def rotation_values(*, fixed: Elem | None = None, velocity: Elem | None = None,
                     acceleration: Elem | None = None) -> Elem:
    """``RotationValues``. ``fixed`` is ``xyz("Rotation", ...)``-shaped for a
    ``Type=0``/``Fixed`` block; ``velocity``/``acceleration`` are
    ``xyz("Rotation", X=pva(...), ...)``-style (each axis itself a PVA) for a
    ``Type=1``/``PVA`` block.
    """
    e = Elem("RotationValues")
    if fixed is not None:
        e.children.append(Elem("Type", text="0"))
        e.children.append(elem("Fixed", Rotation=fixed))
    elif velocity is not None or acceleration is not None:
        e.children.append(Elem("Type", text="1"))
        pva_block = Elem("PVA")
        if velocity is not None:
            pva_block.children.append(velocity if velocity.tag == "Velocity" else Elem("Velocity", children=velocity.children))
        if acceleration is not None:
            pva_block.children.append(acceleration if acceleration.tag == "Acceleration" else Elem("Acceleration", children=acceleration.children))
        e.children.append(pva_block)
    return e


def scaling_values(*, fixed: Elem | None = None, pva_scale: Elem | None = None) -> Elem:
    """``ScalingValues``. ``fixed`` is ``xyz("Scale", ...)``-shaped for a
    ``Type=0``/``Fixed`` block; ``pva_scale`` is a ``pva("Scale", x={...},
    y={...}, drawn_as=0)``-shaped ``Elem`` for a ``Type=1``/``PVA`` block.
    """
    e = Elem("ScalingValues")
    if fixed is not None:
        e.children.append(Elem("Type", text="0"))
        e.children.append(elem("Fixed", Scale=fixed))
    if pva_scale is not None:
        if fixed is None:
            e.children.append(Elem("Type", text="1"))
        e.children.append(elem("PVA", Scale=pva_scale))
    return e


def generation_location_circle(*, division=None, angle_start=None, angle_end=None, radius=None) -> Elem:
    e = Elem("GenerationLocationValues")
    e.children.append(Elem("Type", text="0"))
    circle = Elem("Circle")
    if division is not None:
        circle.children.append(pva("Division", **division) if isinstance(division, dict) else elem("Division", text=division))
    if angle_start is not None:
        circle.children.append(pva("AngleStart", **angle_start))
    if angle_end is not None:
        circle.children.append(pva("AngleEnd", **angle_end))
    if radius is not None:
        circle.children.append(pva("Radius", **radius))
    e.children.append(circle)
    return e


def generation_location_sphere(*, rotation_x=None, rotation_y=None, effects_rotation: bool = True) -> Elem:
    e = Elem("GenerationLocationValues")
    e.children.append(Elem("EffectsRotation", text=_fmt(effects_rotation)))
    e.children.append(Elem("Type", text="1"))
    sphere = Elem("Sphere")
    if rotation_x is not None:
        sphere.children.append(pva("RotationX", **rotation_x))
    if rotation_y is not None:
        sphere.children.append(pva("RotationY", **rotation_y))
    e.children.append(sphere)
    return e


def renderer_common(*, filter_: int | None = None, alpha_blend: int | None = None,
                     zwrite: bool | None = None, wrap: int | None = None) -> Elem:
    e = Elem("RendererCommonValues")
    if filter_ is not None:
        e.children.append(Elem("Filter", text=_fmt(filter_)))
    if alpha_blend is not None:
        e.children.append(Elem("AlphaBlend", text=_fmt(alpha_blend)))
    if zwrite is not None:
        e.children.append(Elem("ZWrite", text=_fmt(zwrite)))
    if wrap is not None:
        e.children.append(Elem("Wrap", text=_fmt(wrap)))
    return e


# ---------------------------------------------------------------------------
# DrawingValues kind builders (v1: Sprite / Ring / Ribbon only)
def sprite(*, billboard: int | None = 0, color_all: Elem | None = None) -> Elem:
    # Effekseer's real BillboardType: 0=Billboard (always faces the camera -
    # what almost every particle sprite wants), 1=YAxisFixed, 2=Fixed (no
    # camera-facing correction at all - renders as a static oriented plane,
    # e.g. a ground-flat shockwave ring), 3=RotatedBillboard. Defaulting this
    # to 2 previously (copied from the one real sample on hand that happened
    # to use it, a ground decal) made ordinary puff/spark sprites render as a
    # flat static card instead of a camera-facing cloud.
    e = Elem("Sprite")
    if billboard is not None:
        e.children.append(Elem("Billboard", text=_fmt(billboard)))
    if color_all is not None:
        e.children.append(color_all if color_all.tag == "ColorAll_Fixed" else Elem("ColorAll_Fixed", children=color_all.children))
    return e


def ring(*, vertex_count: int = 36, outer: Elem | None = None, inner: Elem | None = None,
          center_ratio=None, outer_color: Elem | None = None, center_color: Elem | None = None,
          inner_color: Elem | None = None) -> Elem:
    e = Elem("Ring")
    e.children.append(Elem("VertexCount", text=_fmt(vertex_count)))
    if outer is not None:
        e.children.append(elem("Outer_Fixed", Location=outer))
    if inner is not None:
        e.children.append(elem("Inner_Fixed", Location=inner))
    if center_ratio is not None:
        e.children.append(Elem("CenterRatio_Fixed", text=_fmt(center_ratio)))
    if outer_color is not None:
        e.children.append(Elem("OuterColor_Fixed", children=outer_color.children))
    if center_color is not None:
        e.children.append(Elem("CenterColor_Fixed", children=center_color.children))
    if inner_color is not None:
        e.children.append(Elem("InnerColor_Fixed", children=inner_color.children))
    return e


def ribbon(*, viewpoint_dependent: bool = True, color_all: Elem | None = None) -> Elem:
    e = Elem("Ribbon")
    e.children.append(Elem("ViewpointDependent", text=_fmt(viewpoint_dependent)))
    if color_all is not None:
        e.children.append(color_all if color_all.tag == "ColorAll_Fixed" else Elem("ColorAll_Fixed", children=color_all.children))
    return e


DRAWING_TYPE = {"sprite": 2, "ribbon": 3, "ring": 4}


def drawing_values(kind: str, block: Elem) -> Elem:
    if kind not in DRAWING_TYPE:
        raise ValueError(f"unknown DrawingValues kind {kind!r}; v1 supports {sorted(DRAWING_TYPE)}")
    e = Elem("DrawingValues")
    e.children.append(Elem("Type", text=_fmt(DRAWING_TYPE[kind])))
    e.children.append(block)
    return e


# ---------------------------------------------------------------------------
# Node / project assembly
def node(name: str = "Node", *, common: Elem | None = None, location: Elem | None = None,
         rotation: Elem | None = None, scaling: Elem | None = None,
         generation_location: Elem | None = None, renderer_common: Elem | None = None,
         drawing: Elem | None = None, is_rendered: bool | None = None,
         children: list[Elem] | None = None) -> Elem:
    """Build a full ``<Node>`` ready to append to a parent's ``<Children>``."""
    n = Elem("Node")
    for block in (common, location, rotation, scaling, generation_location,
                  renderer_common, drawing):
        if block is not None:
            n.children.append(block)
    if is_rendered is not None:
        n.children.append(Elem("IsRendered", text=_fmt(is_rendered)))
    n.children.append(Elem("Name", text=name))
    kids = Elem("Children")
    if children:
        kids.children.extend(children)
    n.children.append(kids)
    return n


def group_node(name: str = "Node", *, children: list[Elem] | None = None) -> Elem:
    """A pure container node: no ``DrawingValues``, just organizes children."""
    return node(name, children=children)


def sprite_node(name: str = "Node", *, sprite_block: Elem, children: list[Elem] | None = None,
                 **node_kwargs) -> Elem:
    return node(name, drawing=drawing_values("sprite", sprite_block), children=children, **node_kwargs)


def ring_node(name: str = "Node", *, ring_block: Elem, children: list[Elem] | None = None,
              **node_kwargs) -> Elem:
    return node(name, drawing=drawing_values("ring", ring_block), children=children, **node_kwargs)


def ribbon_node(name: str = "Node", *, ribbon_block: Elem, children: list[Elem] | None = None,
                **node_kwargs) -> Elem:
    return node(name, drawing=drawing_values("ribbon", ribbon_block), children=children, **node_kwargs)


def new_project(*, start_frame: int = 0, end_frame: int = 60, is_loop: bool = True,
                 tool_version: str = "0.7CTP1", version: int = 3,
                 root_children: list[Elem] | None = None) -> Elem:
    """Build a full ``<EffekseerProject>`` skeleton.

    Defaults (``tool_version``/``version``) match the real AndrewFM01 samples
    this toolkit was built from; the local Effekseer 1.7.3.0 CUI compiles
    them successfully regardless of the declared ``ToolVersion`` (verified
    this session), so these are safe placeholders rather than a requirement
    to match exactly.
    """
    root = Elem("Root")
    root.children.append(Elem("Name", text="Root"))
    children = Elem("Children")
    if root_children:
        children.children.extend(root_children)
    root.children.append(children)

    proj = Elem("EffekseerProject")
    proj.children.append(root)
    proj.children.append(Elem("ToolVersion", text=tool_version))
    proj.children.append(Elem("Version", text=_fmt(version)))
    proj.children.append(Elem("StartFrame", text=_fmt(start_frame)))
    proj.children.append(Elem("EndFrame", text=_fmt(end_frame)))
    proj.children.append(Elem("IsLoop", text=_fmt(is_loop)))
    return proj
