#!/usr/bin/env python3
"""Generate data/Assets/UI/Hud.svg -- the art for the in-game HUD widgets.

The file is a normal SVG and can be edited in Inkscape afterwards; this script
just gets the geometry (chamfered plates, hexagons, the heart and the gem)
exact and repeatable.

Spark's SVG importer (traktor/code/Spark/Editor/ConvertSvg.cpp) turns every
element carrying a "traktor:sprite" attribute into a sprite character:

  * a top level one is *exported* under its id, which is what
    Widget._createResource(parent, "MC_...") instantiates;
  * a nested one is placed on its parent's frame under its id, which is what
    mc:getMember("...") returns -- those are the parts the widget scripts
    animate.

Two rules the importer imposes on the authoring:

  * Shapes that sit directly in a sprite are placed on the parent frame *after*
    its child sprites, so they would cover them. Every drawable therefore lives
    inside a named child sprite.
  * Coordinates end up absolute in movie space (the viewBox is mapped onto
    width x height), so each exported widget is authored at the origin. They
    consequently overlap in the document, exactly as the engine's own
    UiKit2.svg does; the layers are display:none except the first so the file
    stays editable.

Numbers are always written with a leading digit -- the importer's number
scanner skips a leading '.' and would read ".5" as 5.
"""

import math
import os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "data", "Assets", "UI", "Hud.svg")

# ---------------------------------------------------------------- geometry --

DOC_W, DOC_H = 460, 216         # document; big enough for the widest (health bar)
                                # and the tallest (minimap) export, since every
                                # one of them is authored at the origin

W, H = 460, 56                  # health bar widget box

BADGE_X, BADGE_Y = 28, 28       # heart badge centre
BADGE_R = 24

BAR_X, BAR_Y = 58, 12           # bar plate
BAR_W, BAR_H = 396, 32
CUT = 8

TRACK_X, TRACK_Y = 62, 16       # track / fill
TRACK_W, TRACK_H = 388, 24

TC_W = 180                      # treasure counter widget box (fixed width, so
TC_PLATE_Y = 10                 # the plate never has to be redrawn or relaid)
TC_PLATE_H = 36
GEM_X, GEM_Y = 28, 28
GEM_S = 30
SPARK_X, SPARK_Y = 21, 21

AC_W = TC_W                     # ammo counter shares the treasure plate, so the
ROUND_X, ROUND_Y = 28, 28       # two right hand HUD tiles read as a matched pair
ROUND_S = 32                    # cartridge height
RELOAD_X, RELOAD_W = 50, TC_W - 60  # reload bar, under the count and clear of
RELOAD_Y, RELOAD_H = 39, 3          # the cartridge so it does not cut across it

WC_W = 380                      # weapon carousel: one tile for the whole arsenal,
WC_SEAT_X = 68                  # as wide as the three it replaced. Every seat is
WC_SEAT_Y = 34                  # authored here, at the front of the ring, and the
                                # widget translates it round (as the minimap does
                                # with BLIP_C), so they overlap in the document
WC_CX, WC_CY = 68, 25           # the ring the seats ride: centre...
WC_RX, WC_RY = 44, 9            # ...and radii. Front of the ring = (CX, CY + RY),
                                # which is what SEAT_X/SEAT_Y above are
WC_ICON = 20                    # a seat's icon at the front, in plate pixels
WC_SOCKET_W, WC_SOCKET_H = 34, 22   # the lit seat the selected weapon sits in
WC_SOCKET_Y = 33
WC_TEXT_X, WC_TEXT_W = 140, 232 # the selected weapon's name and count
WC_RELOAD_X, WC_RELOAD_W = 160, 190
WC_RELOAD_Y, WC_RELOAD_H = 41, 3

OM_W = TC_W                     # objective marker: third tile on the same plate
DIAL_X, DIAL_Y = 28, 28         # compass dial centre, and the needle's pivot
DIAL_R = 17
NEEDLE_R = 13                   # needle tip, measured from the pivot

CH_W = 40                       # crosshair box, authored centred on CH_C
CH_C = 20
CH_ARM = 10                     # length of one tick
CH_TH = 4                       # its thickness, outline included
CH_GAP = 4                      # clear space between the centre and a tick

MM_W = 200                      # minimap box, square
MM_BEZEL = 5                    # frame thickness around the map face
MM_CUT = 8                      # plate chamfer, as on the HUD tiles

BLIP_C = 40                     # every map marker is authored centred here, so
                                # the widget places one with a single offset
WARD_AREA_R = 36                # authored radius of the ward disc; scaled at
                                # runtime to whatever Main's _wardRadius is

# ------------------------------------------------------------------ palette --

EDGE = "#050508"                # outermost dark line of a plate
BEVEL = "#5c5f6b"               # lit rim
FACE = "#1e1e24"                # plate face
TRACK = "#0a0a0e"
BLACK = "#000000"
WHITE = "#ffffff"
WARN = "#ff2a1a"
GHOST = "#b82316"
HEART = "#de2129"
HEART_RIM = "#4c060c"
HEART_HI = "#ff9e99"
GEM = "#f2bf1a"                 # matches the Pickup material's gold
GEM_TABLE = "#ffedb0"
GEM_LEFT = "#c79309"
GEM_RIGHT = "#8c5c07"
GEM_RIM = "#42280a"

BRASS = "#c8962a"               # cartridge case
BRASS_HI = "#f2d488"
BRASS_LO = "#8a6416"
BRASS_RIM = "#3d2b07"
LEAD = "#b9b9c2"                # bullet tip
LEAD_HI = "#e8e8ef"
LEAD_LO = "#6e6e78"
RELOAD = "#4aa3ff"              # reload sweep, matching the ammo pickup's blue

GREN_BODY = "#4a5236"           # grenade: olive body, scored band, steel spoon
GREN_HI = "#79835c"
GREN_LO = "#2b3020"
GREN_RIM = "#171a10"
GREN_METAL = "#9297a4"
GREN_METAL_LO = "#4d515c"

SEL = "#ffe6ad"                 # the carousel's lit socket, in the same warm cream
SEL_RIM = "#4a3f27"             # the HUD writes the selected weapon's numbers in
RAIL = "#141419"                # the groove the seats ride in; a shade under the
                                # plate face and no more, or the top of the ring
                                # reads as a bar drawn across the tile

DIAL_RIM = "#4a4d57"            # compass dial
DIAL_FACE = "#121218"
DIAL_TICK = "#8b8f9c"
NEEDLE = "#ffb347"              # needle, in the relic's amber
NEEDLE_HI = "#ffe0a8"
NEEDLE_RIM = "#3a2405"

CH_DARK = "#050508"             # crosshair outline, so the reticle survives a
CH_LIGHT = "#eef1f7"            # pale background as well as the dark forest

MAP_FACE = "#0b0e12"            # minimap ground inside the bezel
MAP_GRID = "#1b222c"
MAP_TICK = "#8b8f9c"
MAP_RELIC = NEEDLE              # deliberately the compass needle's amber: the
                                # two read as pointing at the same thing
MAP_WARD_DARK = "#6a7480"
MAP_WARD_LIT = "#ffd08a"
MAP_WARD_CORE = "#fff4dd"
MAP_WARD_AREA = "#ffc061"
MAP_LANDMARK = "#9fb0c4"
MAP_PLAYER = "#eef1f7"

# The fill is drawn greyscale and tinted per frame by the widget's colour
# transform, so these are shades, not colours.
FILL_BODY = "#a8a8a8"
FILL_TOP = "#e6e6e6"
FILL_BOTTOM = "#5c5c5c"


def n(v):
    """Number, never with a leading '.' and never in scientific notation."""
    s = "%.2f" % v
    s = s.rstrip("0").rstrip(".")
    return s if s not in ("", "-") else "0"


def pts(points):
    return " ".join("%s,%s" % (n(x), n(y)) for x, y in points)


def style(fill, opacity=None):
    s = "fill:%s" % fill
    if opacity is not None:
        s += ";fill-opacity:%s" % n(opacity)
    return s


def poly(points, fill, opacity=None):
    return '<polygon points="%s" style="%s"/>' % (pts(points), style(fill, opacity))


def rect(x, y, w, h, fill, opacity=None):
    return ('<rect x="%s" y="%s" width="%s" height="%s" style="%s"/>'
            % (n(x), n(y), n(w), n(h), style(fill, opacity)))


def circle(cx, cy, r, fill, opacity=None):
    return ('<circle cx="%s" cy="%s" r="%s" style="%s"/>'
            % (n(cx), n(cy), n(r), style(fill, opacity)))


def ellipse(cx, cy, rx, ry, fill, opacity=None):
    return ('<ellipse cx="%s" cy="%s" rx="%s" ry="%s" style="%s"/>'
            % (n(cx), n(cy), n(rx), n(ry), style(fill, opacity)))


def cut_rect(x, y, w, h, c):
    """Rectangle with chamfered corners -- the plated HUD look."""
    return [
        (x + c, y), (x + w - c, y),
        (x + w, y + c), (x + w, y + h - c),
        (x + w - c, y + h), (x + c, y + h),
        (x, y + h - c), (x, y + c),
    ]


def hexagon(cx, cy, r):
    return [(cx + math.cos(math.radians(i * 60 + 30)) * r,
             cy + math.sin(math.radians(i * 60 + 30)) * r) for i in range(6)]


def heart(cx, cy, s):
    """Heart centred on (cx, cy), s ~ half width. Six quadratics closing on
    the start point: up the left flank, over both lobes through the centre
    dip, then back down to the tip."""
    def p(dx, dy):
        return "%s %s" % (n(cx + dx * s), n(cy + dy * s))
    return ("M %s Q %s %s Q %s %s Q %s %s Q %s %s Q %s %s Q %s %s Z" % (
        p(0, 0.95),
        p(-1.05, 0.15), p(-1.05, -0.35),
        p(-1.05, -0.95), p(-0.5, -0.95),
        p(-0.18, -0.95), p(0, -0.5),
        p(0.18, -0.95), p(0.5, -0.95),
        p(1.05, -0.95), p(1.05, -0.35),
        p(1.05, 0.15), p(0, 0.95)))


def gem(cx, cy, s):
    """Brilliant cut: table across the top, girdle at the widest point,
    pavilion down to the tip, with three facet tones over the silhouette."""
    hw, top, gird, tip, tw = 0.5 * s, -0.42 * s, -0.15 * s, 0.5 * s, 0.27 * s

    def o(points):
        return [(cx + x, cy + y) for x, y in points]

    return [
        poly(o([(-tw - 1.2, top - 1.2), (tw + 1.2, top - 1.2),
                (hw + 1.2, gird), (0, tip + 1.4), (-hw - 1.2, gird)]), GEM_RIM),
        poly(o([(-tw, top), (tw, top), (hw, gird), (0, tip), (-hw, gird)]), GEM),
        poly(o([(-tw, top), (tw, top), (tw * 1.55, gird), (-tw * 1.55, gird)]), GEM_TABLE),
        poly(o([(-hw, gird), (-tw * 1.55, gird), (0, tip)]), GEM_LEFT),
        poly(o([(tw * 1.55, gird), (hw, gird), (0, tip)]), GEM_RIGHT),
    ]


def _cartridge_parts(cx, cy, s):
    """Rim, case and bullet polygons of a rifle round, tip up, centred on
    (cx, cy) and s tall. Split out so the silhouette can be redrawn slightly
    inflated underneath, the way the gem's rim is."""
    half = s * 0.5
    y_base = cy + half
    y_rim = cy + half - s * 0.09        # top of the extractor rim
    y_shoulder = cy - half + s * 0.48   # top of the straight case
    y_neck = cy - half + s * 0.36       # case neck, where the bullet starts
    y_tip = cy - half
    w_rim, w_case, w_neck = s * 0.19, s * 0.16, s * 0.1

    rim = [(cx - w_rim, y_rim), (cx + w_rim, y_rim),
           (cx + w_rim, y_base), (cx - w_rim, y_base)]

    case = [(cx - w_case, y_rim), (cx + w_case, y_rim),
            (cx + w_case, y_shoulder), (cx + w_neck, y_neck),
            (cx - w_neck, y_neck), (cx - w_case, y_shoulder)]

    # Circular ogive, four segments a flank.
    steps = 5

    def flank(sign):
        out = []
        for i in range(1, steps):
            t = i / steps
            out.append((cx + sign * w_neck * math.sqrt(max(0.0, 1.0 - t * t)),
                        y_neck - (y_neck - y_tip) * t))
        return out

    bullet = ([(cx - w_neck, y_neck), (cx + w_neck, y_neck)]
              + flank(1) + [(cx, y_tip)] + list(reversed(flank(-1))))

    return rim, case, bullet


def cartridge(cx, cy, s):
    o_rim, o_case, o_bullet = _cartridge_parts(cx, cy, s * 1.14)
    rim, case, bullet = _cartridge_parts(cx, cy, s)
    return [
        poly(o_rim, BRASS_RIM), poly(o_case, BRASS_RIM), poly(o_bullet, BRASS_RIM),
        poly(rim, BRASS_LO),
        poly(case, BRASS),
        poly(bullet, LEAD),
        # Lit from the upper left, like every other plate on the HUD.
        rect(cx - s * 0.12, cy - s * 0.05, s * 0.05, s * 0.32, BRASS_HI, 0.85),
        poly([(cx - s * 0.07, cy - s * 0.16), (cx - s * 0.02, cy - s * 0.17),
              (cx - s * 0.01, cy - s * 0.42), (cx - s * 0.05, cy - s * 0.4)],
             LEAD_HI, 0.8),
        rect(cx - s * 0.16, cy + s * 0.29, s * 0.32, s * 0.03, BRASS_RIM, 0.7),
    ]


def casing(cx, cy):
    """Spent case, drawn lying on its side; the widget flings it off on each
    shot. No bullet, and shorter than the live round it leaves behind."""
    w, h = 13, 6
    return [
        rect(cx - w / 2 - 1, cy - h / 2 - 1, w + 2, h + 2, BRASS_RIM),
        rect(cx - w / 2, cy - h / 2, w, h, BRASS),
        rect(cx - w / 2, cy - h / 2, w, 2, BRASS_HI, 0.8),
        rect(cx + w / 2 - 2, cy - h / 2, 2, h, BRASS_LO),
    ]


def star(cx, cy, r):
    k = r * 0.2
    return poly([(cx, cy - r), (cx + k, cy - k), (cx + r, cy), (cx + k, cy + k),
                 (cx, cy + r), (cx - k, cy + k), (cx - r, cy), (cx - k, cy - k)],
                WHITE, 0.9)


def sprite(sid, body, indent=4):
    pad = " " * indent
    inner = "\n".join(pad + "  " + b for b in body)
    return ('%s<g id="%s" inkscape:label="%s" traktor:sprite="1">\n%s\n%s</g>'
            % (pad, sid, sid, inner, pad))


# ------------------------------------------------------------- health bar ---

def health_bar():
    """Layers back to front. Only 'ghost', 'fill' and 'edge' change length;
    'warn', 'flash' and 'heart' are animated through alpha/scale alone."""
    plate = cut_rect(BAR_X, BAR_Y, BAR_W, BAR_H, CUT)

    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(BAR_X, BAR_Y + 3, BAR_W, BAR_H, CUT), BLACK, 0.45),
        poly(hexagon(BADGE_X, BADGE_Y + 3, BADGE_R), BLACK, 0.45),
    ]))

    # Sits under the plate, a few pixels proud of it, so only a rim shows.
    layers.append(sprite("warn", [
        poly(cut_rect(BAR_X - 4, BAR_Y - 4, BAR_W + 8, BAR_H + 8, CUT + 3), WARN, 0.35),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R + 3.5), WARN, 0.35),
        poly(cut_rect(BAR_X - 2, BAR_Y - 2, BAR_W + 4, BAR_H + 4, CUT + 1), WARN),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R + 1.5), WARN),
    ]))

    # Dark outline, lit bevel, then a face inset further from the top than the
    # bottom so the bevel reads as lit from above.
    layers.append(sprite("plate", [
        poly(plate, EDGE),
        poly(cut_rect(BAR_X + 1, BAR_Y + 1, BAR_W - 2, BAR_H - 2, CUT - 1), BEVEL),
        poly(cut_rect(BAR_X + 2, BAR_Y + 3, BAR_W - 4, BAR_H - 4, CUT - 2), FACE),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R), EDGE),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R - 1.5), BEVEL),
        poly(hexagon(BADGE_X, BADGE_Y + 0.5, BADGE_R - 3.5), FACE),
    ]))

    layers.append(sprite("track", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, TRACK),
        rect(TRACK_X, TRACK_Y, TRACK_W, 2, BLACK, 0.55),
    ]))

    layers.append(sprite("ghost", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, GHOST, 0.92),
    ]))

    layers.append(sprite("fill", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, FILL_BODY),
        rect(TRACK_X, TRACK_Y + 1, TRACK_W, TRACK_H * 0.38, FILL_TOP),
        rect(TRACK_X, TRACK_Y + TRACK_H * 0.74, TRACK_W, TRACK_H * 0.26, FILL_BOTTOM),
    ]))

    # Rides the head of the fill; translated, never scaled, so it stays crisp.
    layers.append(sprite("edge", [
        rect(TRACK_X, TRACK_Y, 3, TRACK_H, WHITE),
    ]))

    pips = [rect(TRACK_X + round(TRACK_W * i / 10) - 1, TRACK_Y, 2, TRACK_H, BLACK, 0.45)
            for i in range(1, 10)]
    pips.append(rect(TRACK_X, TRACK_Y, TRACK_W, 1, BLACK, 0.55))
    pips.append(rect(TRACK_X, TRACK_Y + TRACK_H - 1, TRACK_W, 1, BLACK, 0.55))
    layers.append(sprite("pips", pips))

    layers.append(sprite("flash", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, WHITE),
    ]))

    layers.append(sprite("heart", [
        '<path d="%s" style="%s"/>' % (heart(BADGE_X, BADGE_Y, 14.5), style(HEART_RIM)),
        '<path d="%s" style="%s"/>' % (heart(BADGE_X, BADGE_Y, 13), style(HEART)),
        '<ellipse cx="%s" cy="%s" rx="3" ry="2.2" style="%s"/>'
        % (n(BADGE_X - 4.6), n(BADGE_Y - 5.4), style(HEART_HI, 0.8)),
    ]))

    return layers


# -------------------------------------------------------- treasure counter --

def treasure_counter():
    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(2, TC_PLATE_Y + 3, TC_W - 4, TC_PLATE_H, 7), BLACK, 0.45),
    ]))

    layers.append(sprite("plate", [
        poly(cut_rect(2, TC_PLATE_Y, TC_W - 4, TC_PLATE_H, 7), EDGE),
        poly(cut_rect(3, TC_PLATE_Y + 1, TC_W - 6, TC_PLATE_H - 2, 6), BEVEL),
        poly(cut_rect(4, TC_PLATE_Y + 3, TC_W - 8, TC_PLATE_H - 4, 5), FACE),
    ]))

    layers.append(sprite("gem", gem(GEM_X, GEM_Y, GEM_S)))
    layers.append(sprite("sparkle", [star(SPARK_X, SPARK_Y, 7)]))

    layers.append(sprite("glow", [
        poly(cut_rect(3, TC_PLATE_Y + 1, TC_W - 6, TC_PLATE_H - 2, 6), WHITE),
    ]))

    return layers


# ------------------------------------------------------------ ammo counter --

def ammo_counter():
    """Shares the treasure counter's plate so the two right hand tiles match.
    'round' kicks on each shot, 'casing' is flung clear, 'reload' fills as the
    magazine is refilled and 'warn' rims the plate once it runs dry."""
    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(2, TC_PLATE_Y + 3, AC_W - 4, TC_PLATE_H, 7), BLACK, 0.45),
    ]))

    # Under the plate, a couple of pixels proud of it, so only a rim shows.
    layers.append(sprite("warn", [
        poly(cut_rect(0, TC_PLATE_Y - 3, AC_W, TC_PLATE_H + 6, 9), WARN, 0.35),
        poly(cut_rect(1, TC_PLATE_Y - 1, AC_W - 2, TC_PLATE_H + 2, 7), WARN),
    ]))

    layers.append(sprite("plate", [
        poly(cut_rect(2, TC_PLATE_Y, AC_W - 4, TC_PLATE_H, 7), EDGE),
        poly(cut_rect(3, TC_PLATE_Y + 1, AC_W - 6, TC_PLATE_H - 2, 6), BEVEL),
        poly(cut_rect(4, TC_PLATE_Y + 3, AC_W - 8, TC_PLATE_H - 4, 5), FACE),
    ]))

    layers.append(sprite("reloadtrack", [
        rect(RELOAD_X, RELOAD_Y, RELOAD_W, RELOAD_H, TRACK),
    ]))
    layers.append(sprite("reload", [
        rect(RELOAD_X, RELOAD_Y, RELOAD_W, RELOAD_H, RELOAD),
    ]))

    layers.append(sprite("round", cartridge(ROUND_X, ROUND_Y, ROUND_S)))
    layers.append(sprite("casing", casing(ROUND_X, ROUND_Y)))

    layers.append(sprite("glow", [
        poly(cut_rect(3, TC_PLATE_Y + 1, AC_W - 6, TC_PLATE_H - 2, 6), WHITE),
    ]))

    return layers


# --------------------------------------------------------- weapon carousel --

def _belt_icon(cx, cy, s):
    """Machine gun: a short length of belt, four rounds hanging off a link bar.
    Deliberately a different silhouette from the rifle's single cartridge --
    at twenty pixels the two have to be told apart by outline alone, not by
    detail, so one is tall and narrow and the other wide and toothed."""
    out = []
    bar_h = s * 0.24
    bar_w = s * 0.96
    out.append(rect(cx - bar_w / 2 - 1, cy - s * 0.5 - 1, bar_w + 2, bar_h + 2, BRASS_RIM))
    out.append(rect(cx - bar_w / 2, cy - s * 0.5, bar_w, bar_h, BRASS_LO))
    out.append(rect(cx - bar_w / 2, cy - s * 0.5, bar_w, bar_h * 0.35, BRASS_HI, 0.55))

    # Four rounds, tips down, spread across the bar.
    rounds = 4
    rw = s * 0.15
    top = cy - s * 0.5 + bar_h
    body_b = cy + s * 0.28
    tip_b = cy + s * 0.5
    for i in range(rounds):
        rx = cx - bar_w / 2 + bar_w * (i + 0.5) / rounds
        out.append(rect(rx - rw / 2 - 1, top, rw + 2, body_b - top + 1, BRASS_RIM))
        out.append(rect(rx - rw / 2, top, rw, body_b - top, BRASS))
        out.append(rect(rx - rw / 2, top, rw * 0.35, body_b - top, BRASS_HI, 0.6))
        out.append(poly([(rx - rw / 2, body_b), (rx + rw / 2, body_b), (rx, tip_b)], LEAD))
    return out


def _grenade_icon(cx, cy, s):
    """Grenades: scored olive body, steel spoon down the right flank and the
    pull ring proud of the neck. Round where the other two are angular, which
    is the whole of how it reads at this size."""
    r = s * 0.34
    body = [(cx + math.cos(math.radians(i * 30)) * r,
             cy + s * 0.09 + math.sin(math.radians(i * 30)) * r) for i in range(12)]
    rim = [(cx + math.cos(math.radians(i * 30)) * (r + 1.2),
            cy + s * 0.09 + math.sin(math.radians(i * 30)) * (r + 1.2)) for i in range(12)]
    cy_b = cy + s * 0.09
    return [
        poly(rim, GREN_RIM),
        poly(body, GREN_BODY),
        # Lit from the upper left, as every other plate on this HUD is.
        poly([(cx - r * 0.9, cy_b - r * 0.35), (cx - r * 0.35, cy_b - r * 0.85),
              (cx - r * 0.1, cy_b - r * 0.6), (cx - r * 0.7, cy_b - r * 0.05)], GREN_HI, 0.75),
        # Scoring: the pineapple grid, two bands and one rib.
        rect(cx - r, cy_b - r * 0.3, r * 2, s * 0.05, GREN_LO),
        rect(cx - r, cy_b + r * 0.3, r * 2, s * 0.05, GREN_LO),
        rect(cx - s * 0.02, cy_b - r, s * 0.05, r * 2, GREN_LO),
        # Neck, spoon and ring.
        rect(cx - s * 0.11, cy - s * 0.5, s * 0.22, s * 0.18, GREN_METAL_LO),
        poly([(cx + s * 0.09, cy - s * 0.46), (cx + s * 0.2, cy - s * 0.44),
              (cx + s * 0.17, cy + s * 0.06), (cx + s * 0.07, cy + s * 0.02)], GREN_METAL),
        circle(cx - s * 0.2, cy - s * 0.42, s * 0.13, GREN_METAL),
        circle(cx - s * 0.2, cy - s * 0.42, s * 0.06, GREN_METAL_LO),
    ]


def weapon_carousel():
    """One tile for the whole arsenal: three seats riding a shallow ring, the
    selected weapon at the front of it in a lit socket and the other two small
    and dim behind. Cycling spins the ring rather than moving a highlight, so
    which weapon is in hand is carried by *where* it is as well as by how it is
    lit -- see Scripts/Widgets/WeaponCarousel, which drives every layer here.

    Each seat is authored at the front of the ring (WC_SEAT_X/Y) and translated
    into place by the widget, exactly as the minimap's markers are, so all three
    sit on top of each other in the document. Toggle them to edit one."""
    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(2, TC_PLATE_Y + 3, WC_W - 4, TC_PLATE_H, 7), BLACK, 0.45),
    ]))

    # Under the plate, a couple of pixels proud of it, so only a rim shows -- the
    # ammunition counter's warning, kept for the weapon that is in hand.
    layers.append(sprite("warn", [
        poly(cut_rect(0, TC_PLATE_Y - 3, WC_W, TC_PLATE_H + 6, 9), WARN, 0.35),
        poly(cut_rect(1, TC_PLATE_Y - 1, WC_W - 2, TC_PLATE_H + 2, 7), WARN),
    ]))

    layers.append(sprite("plate", [
        poly(cut_rect(2, TC_PLATE_Y, WC_W - 4, TC_PLATE_H, 7), EDGE),
        poly(cut_rect(3, TC_PLATE_Y + 1, WC_W - 6, TC_PLATE_H - 2, 6), BEVEL),
        poly(cut_rect(4, TC_PLATE_Y + 3, WC_W - 8, TC_PLATE_H - 4, 5), FACE),
    ]))

    # The ring itself, as a groove the seats travel in: an ellipse a shade under
    # the plate face with the face punched back out of it, and a lit lower arc so
    # the near side of the ring reads as nearer.
    layers.append(sprite("rail", [
        ellipse(WC_CX, WC_CY, WC_RX + 1.5, WC_RY + 1.5, RAIL),
        ellipse(WC_CX, WC_CY, WC_RX - 1.5, WC_RY - 1.5, FACE),
        ellipse(WC_CX, WC_CY + 1, WC_RX + 1.5, WC_RY + 1.5, BEVEL, 0.16),
        ellipse(WC_CX, WC_CY, WC_RX - 1.5, WC_RY - 1.5, FACE),
    ]))

    # The seat at the front of the ring. Static: the ring turns, this does not.
    layers.append(sprite("socket", [
        ellipse(WC_SEAT_X, WC_SOCKET_Y, WC_SOCKET_W / 2 + 2, WC_SOCKET_H / 2 + 2, EDGE),
        ellipse(WC_SEAT_X, WC_SOCKET_Y, WC_SOCKET_W / 2, WC_SOCKET_H / 2, SEL_RIM),
        ellipse(WC_SEAT_X, WC_SOCKET_Y, WC_SOCKET_W / 2 - 1.5, WC_SOCKET_H / 2 - 1.5, FACE),
    ]))
    # ...and its light, which the widget pulses as a weapon comes to the front.
    layers.append(sprite("socketglow", [
        ellipse(WC_SEAT_X, WC_SOCKET_Y, WC_SOCKET_W / 2 + 1, WC_SOCKET_H / 2 + 1, SEL),
    ]))

    # The three weapons. Document order is depth order, but the widget sorts
    # these by how far round the ring they are on every frame that moves them
    # (DisplayList:swap), so whichever is at the front is drawn over the rest.
    layers.append(sprite("seat_rifle", cartridge(WC_SEAT_X, WC_SEAT_Y, WC_ICON)))
    layers.append(sprite("seat_machinegun", _belt_icon(WC_SEAT_X, WC_SEAT_Y, WC_ICON)))
    layers.append(sprite("seat_grenades", _grenade_icon(WC_SEAT_X, WC_SEAT_Y, WC_ICON)))

    # Spent brass, flung clear of the socket on each shot.
    layers.append(sprite("casing", casing(WC_SEAT_X, WC_SEAT_Y)))

    layers.append(sprite("reloadtrack", [
        rect(WC_RELOAD_X, WC_RELOAD_Y, WC_RELOAD_W, WC_RELOAD_H, TRACK),
    ]))
    layers.append(sprite("reload", [
        rect(WC_RELOAD_X, WC_RELOAD_Y, WC_RELOAD_W, WC_RELOAD_H, RELOAD),
    ]))

    layers.append(sprite("glow", [
        poly(cut_rect(3, TC_PLATE_Y + 1, WC_W - 6, TC_PLATE_H - 2, 6), WHITE),
    ]))

    return layers


# -------------------------------------------------------- objective marker --

def _needle_shape(cx, cy, r, w, tail):
    """Arrowhead pointing up, straddling the pivot: tip ahead of it, barbs and
    notch behind, so it reads as a compass needle swinging about (cx, cy)."""
    return [(cx, cy - r), (cx + w, cy + tail), (cx, cy + tail * 0.35), (cx - w, cy + tail)]


def objective_marker():
    """Same plate as the other right hand tiles, carrying a compass dial. Only
    'needle' moves -- the widget spins it about (DIAL_X, DIAL_Y) by the bearing to
    the current objective. The ticks stay put: the top one is 'straight ahead'."""
    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(2, TC_PLATE_Y + 3, OM_W - 4, TC_PLATE_H, 7), BLACK, 0.45),
    ]))

    layers.append(sprite("plate", [
        poly(cut_rect(2, TC_PLATE_Y, OM_W - 4, TC_PLATE_H, 7), EDGE),
        poly(cut_rect(3, TC_PLATE_Y + 1, OM_W - 6, TC_PLATE_H - 2, 6), BEVEL),
        poly(cut_rect(4, TC_PLATE_Y + 3, OM_W - 8, TC_PLATE_H - 4, 5), FACE),
    ]))

    layers.append(sprite("dial", [
        circle(DIAL_X, DIAL_Y, DIAL_R + 2, EDGE),
        circle(DIAL_X, DIAL_Y, DIAL_R, DIAL_RIM),
        circle(DIAL_X, DIAL_Y, DIAL_R - 2, DIAL_FACE),
        # Fixed heading marks: a prominent one for dead ahead, faint for the sides
        # and behind, so a needle pointing anywhere but up is obvious at a glance.
        poly([(DIAL_X - 3, DIAL_Y - DIAL_R + 1), (DIAL_X + 3, DIAL_Y - DIAL_R + 1),
              (DIAL_X, DIAL_Y - DIAL_R + 6)], DIAL_TICK),
        rect(DIAL_X + DIAL_R - 6, DIAL_Y - 1, 4, 2, DIAL_TICK, 0.55),
        rect(DIAL_X - DIAL_R + 2, DIAL_Y - 1, 4, 2, DIAL_TICK, 0.55),
        rect(DIAL_X - 1, DIAL_Y + DIAL_R - 6, 2, 4, DIAL_TICK, 0.55),
    ]))

    layers.append(sprite("needle", [
        poly(_needle_shape(DIAL_X, DIAL_Y, NEEDLE_R + 1.5, 7.4, 6.2), NEEDLE_RIM),
        poly(_needle_shape(DIAL_X, DIAL_Y, NEEDLE_R, 6, 5), NEEDLE),
        # Lit down the left flank, like every other plate on the HUD.
        poly([(DIAL_X, DIAL_Y - NEEDLE_R), (DIAL_X, DIAL_Y + 1.75),
              (DIAL_X - 6, DIAL_Y + 5)], NEEDLE_HI, 0.55),
        circle(DIAL_X, DIAL_Y, 2.2, NEEDLE_RIM),
    ]))

    layers.append(sprite("glow", [
        poly(cut_rect(3, TC_PLATE_Y + 1, OM_W - 6, TC_PLATE_H - 2, 6), WHITE),
    ]))

    return layers


# -------------------------------------------------------------- crosshair ---

def _tick(x, y, w, h):
    """One crosshair tick: a dark bar with a light core inset a pixel."""
    return [rect(x, y, w, h, CH_DARK), rect(x + 1, y + 1, w - 2, h - 2, CH_LIGHT)]


def crosshair():
    """Four ticks around a centre dot, authored centred in a CH_W box. Each tick
    is its own child sprite so Crosshair can spring it outward on a shot and let
    it ease back; the dot never moves."""
    a = CH_C - CH_GAP - CH_ARM      # outer end of the up and left ticks
    b = CH_C + CH_GAP               # inner end of the down and right ticks
    t = CH_C - CH_TH / 2            # cross axis, shared by all four

    return [
        sprite("up", _tick(t, a, CH_TH, CH_ARM)),
        sprite("down", _tick(t, b, CH_TH, CH_ARM)),
        sprite("left", _tick(a, t, CH_ARM, CH_TH)),
        sprite("right", _tick(b, t, CH_ARM, CH_TH)),
        sprite("dot", [
            circle(CH_C, CH_C, 2.8, CH_DARK),
            circle(CH_C, CH_C, 1.8, CH_LIGHT),
        ]),
    ]


# ---------------------------------------------------------------- minimap ---

def minimap():
    """The map plate: a chamfered frame around a dark ground with a quarter grid
    and edge ticks. Nothing in here moves. Everything that does -- relics, wards,
    landmarks, the player -- is one of the MC_Map* exports below, which the widget
    instantiates as many of as the scene needs and positions itself."""
    inner = MM_BEZEL + 1
    fw = MM_W - inner * 2
    c = MM_W / 2.0

    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(2, 3, MM_W - 4, MM_W - 4, MM_CUT), BLACK, 0.45),
    ]))

    layers.append(sprite("plate", [
        poly(cut_rect(0, 0, MM_W, MM_W, MM_CUT), EDGE),
        poly(cut_rect(1, 1, MM_W - 2, MM_W - 2, MM_CUT - 1), BEVEL),
        poly(cut_rect(3, 3, MM_W - 6, MM_W - 6, MM_CUT - 2), FACE),
    ]))

    # Left a little translucent so the forest reads through it and the map does
    # not sit on the view like a solid tile.
    layers.append(sprite("face", [
        rect(inner, inner, fw, fw, MAP_FACE, 0.92),
    ]))

    # Quarters, so a distance across the map can be judged without labelling
    # anything: one cell is a quarter of the extent Main fits to the scene.
    grid = []
    for k in range(1, 4):
        o = inner + fw * k / 4.0
        grid.append(rect(inner, o, fw, 1, MAP_GRID))
        grid.append(rect(o, inner, 1, fw, MAP_GRID))
    layers.append(sprite("grid", grid))

    # North is up. Prominent at the top, faint on the other three edges, exactly
    # as the compass dial marks dead ahead.
    layers.append(sprite("ticks", [
        poly([(c - 4, inner + 1), (c + 4, inner + 1), (c, inner + 8)], MAP_TICK),
        rect(MM_W - inner - 7, c - 1, 6, 2, MAP_TICK, 0.5),
        rect(inner + 1, c - 1, 6, 2, MAP_TICK, 0.5),
        rect(c - 1, MM_W - inner - 7, 2, 6, MAP_TICK, 0.5),
    ]))

    return layers


def map_ward_area():
    """The consecrated ground a lit ward buys. Drawn under every other marker and
    kept faint, so two overlapping wards visibly stack -- which is the whole read
    the player needs when deciding where the next relic should go."""
    return [sprite("disc", [
        circle(BLIP_C, BLIP_C, WARD_AREA_R, MAP_WARD_AREA, 0.14),
    ])]


def map_ward():
    """Ward post: a ring, hollow while the post is dark and filled once it burns.
    Both states are authored and the widget shows one of them, which keeps colour
    transforms out of it."""
    r = 6
    return [
        sprite("dark", [
            circle(BLIP_C, BLIP_C, r, EDGE),
            circle(BLIP_C, BLIP_C, r - 1, MAP_WARD_DARK),
            circle(BLIP_C, BLIP_C, r - 3, MAP_FACE),
        ]),
        sprite("lit", [
            circle(BLIP_C, BLIP_C, r + 1, EDGE),
            circle(BLIP_C, BLIP_C, r, MAP_WARD_LIT),
            circle(BLIP_C, BLIP_C, r - 3, MAP_WARD_CORE),
        ]),
    ]


def map_relic():
    """Relic: a small upright cross, echoing the mesh the player is carrying, on a
    dark outline so it holds up over a lit ward's disc."""
    a, b = 2.0, 7.0
    return [sprite("mark", [
        rect(BLIP_C - b - 1, BLIP_C - a - 1, b * 2 + 2, a * 2 + 2, EDGE),
        rect(BLIP_C - a - 1, BLIP_C - b - 1, a * 2 + 2, b * 2 + 2, EDGE),
        rect(BLIP_C - b, BLIP_C - a, b * 2, a * 2, MAP_RELIC),
        rect(BLIP_C - a, BLIP_C - b, a * 2, b * 2, MAP_RELIC),
    ])]


def map_landmark():
    """Landmark: a hollow square, deliberately plainer and cooler than the two
    markers that are actually objectives."""
    r = 5
    return [sprite("mark", [
        rect(BLIP_C - r, BLIP_C - r, r * 2, r * 2, EDGE),
        rect(BLIP_C - r + 1, BLIP_C - r + 1, r * 2 - 2, r * 2 - 2, MAP_LANDMARK),
        rect(BLIP_C - r + 3, BLIP_C - r + 3, r * 2 - 6, r * 2 - 6, MAP_FACE),
    ])]


def map_player():
    """Player arrow, pointing up at rest. The widget spins it about BLIP_C by the
    player's heading -- the map is north up, so the arrow is the one thing on it
    that says which way they are facing. Same shape as the compass needle."""
    return [sprite("arrow", [
        poly(_needle_shape(BLIP_C, BLIP_C, 10, 6, 5), EDGE),
        poly(_needle_shape(BLIP_C, BLIP_C, 8.5, 4.6, 4), MAP_PLAYER),
    ])]


def main():
    doc = []
    # NOTE: no double hyphen anywhere inside these comments; that is illegal in
    # XML and the importer's parser rejects the whole document over it.
    doc.append('<?xml version="1.0" encoding="UTF-8" standalone="no"?>')
    doc.append('<!-- HUD widget art, generated by tools/gen_hud_svg.py -->')
    doc.append('<!-- Each top level layer is one Spark export: the widget')
    doc.append('     scripts instantiate it by id and animate the named child')
    doc.append('     sprites inside it. Both are authored at the origin and so')
    doc.append('     overlap; toggle the layers to edit them. -->')
    doc.append('<svg width="%d" height="%d" viewBox="0 0 %d %d" version="1.1"'
               % (DOC_W, DOC_H, DOC_W, DOC_H))
    doc.append('   xmlns="http://www.w3.org/2000/svg"')
    doc.append('   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"')
    doc.append('   xmlns:traktor="http://traktor/svg">')

    doc.append('  <g id="MC_HealthBar" inkscape:label="MC_HealthBar"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:inline">')
    doc.extend(health_bar())
    doc.append('  </g>')

    doc.append('  <g id="MC_TreasureCounter" inkscape:label="MC_TreasureCounter"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:none">')
    doc.extend(treasure_counter())
    doc.append('  </g>')

    doc.append('  <g id="MC_AmmoCounter" inkscape:label="MC_AmmoCounter"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:none">')
    doc.extend(ammo_counter())
    doc.append('  </g>')

    doc.append('  <g id="MC_WeaponCarousel" inkscape:label="MC_WeaponCarousel"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:none">')
    doc.extend(weapon_carousel())
    doc.append('  </g>')

    doc.append('  <g id="MC_ObjectiveMarker" inkscape:label="MC_ObjectiveMarker"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:none">')
    doc.extend(objective_marker())
    doc.append('  </g>')

    for eid, builder in (
        ("MC_Crosshair", crosshair),
        ("MC_Minimap", minimap),
        ("MC_MapWardArea", map_ward_area),
        ("MC_MapWard", map_ward),
        ("MC_MapRelic", map_relic),
        ("MC_MapLandmark", map_landmark),
        ("MC_MapPlayer", map_player),
    ):
        doc.append('  <g id="%s" inkscape:label="%s"' % (eid, eid))
        doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
        doc.append('     style="display:none">')
        doc.extend(builder())
        doc.append('  </g>')

    doc.append('</svg>')

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(os.path.normpath(OUT), "w") as f:
        f.write("\n".join(doc) + "\n")
    print("wrote", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
