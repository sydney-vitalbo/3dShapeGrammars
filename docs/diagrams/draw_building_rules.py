"""Render the BuildingRules.cpp hierarchy with explanatory wireframe sketches.

Run from any directory with Python + matplotlib. Outputs sit beside this script.
This is an authored documentation diagram; update it when productions change.
"""
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
matplotlib.rcParams["svg.fonttype"] = "none"
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Polygon, Rectangle

OUT = Path(__file__).resolve().parent
BG, INK, MUTED = "#f5f7fb", "#172b46", "#53667e"
BLUE, BLUE_BG = "#2c66ad", "#e9f1fc"
AMBER, AMBER_BG = "#a86716", "#fff2db"
GREEN, GREEN_BG = "#28785b", "#e9f7ef"
PURPLE = "#7852a1"
fig, ax = plt.subplots(figsize=(26, 18))
fig.patch.set_facecolor(BG)
ax.set_facecolor(BG)
ax.set(xlim=(0, 26), ylim=(0, 18))
ax.set_aspect("equal")
ax.axis("off")
plt.subplots_adjust(left=0.015, right=0.985, top=0.985, bottom=0.015)


def text(x, y, label, size=15, color=INK, ha="left", va="center", weight="normal", **kw):
    return ax.text(x, y, label, fontsize=size, color=color, ha=ha, va=va,
                   fontfamily="DejaVu Sans", fontweight=weight, **kw)


def card(x, y, w, h, title, subtitle="", terminal=False, optional=False, badge=None):
    color = GREEN if optional else AMBER if terminal else BLUE
    fill = GREEN_BG if optional else AMBER_BG if terminal else BLUE_BG
    ax.add_patch(FancyBboxPatch((x-w/2, y-h/2), w, h,
        boxstyle="round,pad=0.035,rounding_size=0.12", linewidth=1.6,
        edgecolor=color, facecolor=fill, linestyle="--" if optional else "-", zorder=3))
    text(x, y+(0.16 if subtitle else 0), title, 18, ha="center", weight="bold", zorder=4)
    if subtitle:
        text(x, y-0.24, subtitle, 12.5, MUTED, ha="center", linespacing=1.45, zorder=4)
    if badge:
        text(x+w/2-0.14, y+h/2-0.08, badge, 12, color, ha="right", va="top", weight="bold", zorder=4)
    return (x, y, w, h)


def edge(start, end, label=None, label_at=None, color=BLUE, style="-", bend=0):
    ax.add_patch(FancyArrowPatch(start, end, arrowstyle="-|>", mutation_scale=17,
        linewidth=1.5, color=color, linestyle=style,
        connectionstyle=f"arc3,rad={bend}", zorder=1))
    if label and label_at:
        text(*label_at, label, 12.5, color, ha="center", zorder=5,
             bbox=dict(facecolor=BG, edgecolor="none", pad=2))


def bus(parent, children, junction_y, color=BLUE):
    px, py = parent
    ax.plot([px, px], [py, junction_y], color=color, lw=1.5, zorder=1)
    xs = [p[0] for p in children]
    ax.plot([min(xs+[px]), max(xs+[px])], [junction_y]*2, color=color, lw=1.5, zorder=1)
    for x, y in children:
        edge((x, junction_y), (x, y), color=color)


def box(x, y, w, h, d=0.35, color=BLUE, fill=None, lw=1.6):
    # Front rectangle and its isometric top/right faces.
    dx, dy = d, d*0.65
    if fill:
        ax.add_patch(Rectangle((x, y), w, h, facecolor=fill, edgecolor="none", zorder=3))
    ax.add_patch(Rectangle((x, y), w, h, fill=False, edgecolor=color, lw=lw, zorder=4))
    ax.add_patch(Polygon([(x,y+h),(x+dx,y+h+dy),(x+w+dx,y+h+dy),(x+w,y+h)],
        closed=True, fill=False, edgecolor=color, lw=lw, zorder=4))
    ax.add_patch(Polygon([(x+w,y),(x+w+dx,y+dy),(x+w+dx,y+h+dy),(x+w,y+h)],
        closed=True, fill=False, edgecolor=color, lw=lw, zorder=4))


def window_sketch(x, y, w, h, color=BLUE):
    # Seven boxes: top, left/right sides, mullion, sill, two glass panes.
    side, trim, mullion = .06*w, .065*h, .035*w
    opening_h = h-2*trim
    pane_w = (w-2*side-mullion)/2
    for px in [x+side, x+side+pane_w+mullion]:
        ax.add_patch(Rectangle((px,y+trim), pane_w, opening_h,
            facecolor="#d9edf6", edgecolor=color, lw=1.2, zorder=4))
        ax.plot([px+.1*pane_w,px+.8*pane_w], [y+.25*h,y+.7*h],
            color="#86b9d1", lw=1.1, zorder=5)
    for px, py, ww, hh in [
        (x,y+h-trim,w,trim), (x,y+trim,side,opening_h),
        (x+w-side,y+trim,side,opening_h),
        (x+(w-mullion)/2,y+trim,mullion,opening_h),
        (x-.03*w,y,w*1.06,trim)]:
        ax.add_patch(Rectangle((px,py),ww,hh,facecolor="#edd8ba",edgecolor=color,lw=1.1,zorder=6))


def iso_block(center, size, origin, color=BLUE, fill="#eef3f9"):
    cx, cy, cz = center
    w, h, d = size
    def p(x, y, z):
        return (origin[0]+.9*(x+.7*z), origin[1]+.9*(y+.4*z))
    corners = [p(cx+x*w/2,cy+y*h/2,cz+z*d/2)
        for x,y,z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),
                      (-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    for face in [(0,1,2,3),(1,5,6,2),(3,2,6,7)]:
        ax.add_patch(Polygon([corners[i] for i in face],closed=True,
            facecolor=fill,edgecolor=color,lw=1.25,zorder=4))


def panel(x, y, w, h, heading, subtitle):
    ax.add_patch(FancyBboxPatch((x,y),w,h,boxstyle="round,pad=0.03,rounding_size=0.16",
        facecolor="white", edgecolor="#ced9e6", linewidth=1.4, zorder=2))
    text(x+.25,y+h-.36,heading,19,weight="bold",zorder=5)
    text(x+.25,y+h-.76,subtitle,12.5,MUTED,zorder=5)


# Header and key.
text(.5,17.55,"BUILDING RULES",33,weight="bold")
text(.52,17.02,"Current production hierarchy  /  BuildingRules.cpp",16,MUTED)
text(15.9,17.48,"BLUE  nonterminal",13,BLUE,ha="right",weight="bold")
text(15.9,17.08,"AMBER  terminal Slab",13,AMBER,ha="right",weight="bold")
text(15.9,16.68,"GREEN  optional nonterminal",12,GREEN,ha="right",weight="bold")

# Main hierarchy. Repeated type references indicate separate node instances.
root = card(7.7,15.7,8.4,1.2,"Office  /  Apartment",
    "makeOffice / makeApartment → unexpanded root")
floor = card(5.7,13.72,7.1,1.05,"Floor × floors","expandBuilding: split Y + repeat Y")
roof = card(13.25,13.72,3.3,1.05,"Roof","height = 0.24",badge="A")
bus((7.7,15.08),[(5.7,14.27),(13.25,14.27)],14.72)
text(.55,15.62,"Office",14,BLUE,weight="bold")
text(.55,15.19,"Wide windows\nWood frames",12.5,MUTED,linespacing=1.5)
text(12.45,15.9,"Apartment",14,GREEN,weight="bold")
text(12.45,15.43,"Smaller windows · Stone frames\nOptional front balconies",12.5,MUTED,linespacing=1.5)

body = card(1.98,11.55,3.1,1.15,"Wall body","Stone · 1 box",terminal=True)
front = card(5.57,11.55,3.35,1.15,"WallRegion: front","+depth / 2 · Y rotation 0°")
rear = card(9.25,11.55,3.35,1.15,"WallRegion: rear","−depth / 2 · Y rotation 180°")
band = card(13.05,11.55,3.5,1.15,"Floor band","height 0.12 · 1 box\nOffice Wood / Apartment Stone",terminal=True)
bus((5.7,13.18),[(1.98,12.15),(5.57,12.15),(9.25,12.15),(13.05,12.15)],12.67)
text(2.1,12.87,"expandFloor: split Y into wall height + band",12.5,PURPLE)

facade = card(7.45,9.55,6.1,1.05,"Facade","expandWallRegion: 1 facade per wall region")
edge((5.57,10.96),(6.25,10.1))
edge((9.25,10.96),(8.65,10.1))
text(11.1,10.15,"Both use local +Z outward.\nSeparate subtrees; same rule.",12.5,MUTED,linespacing=1.5)

text(3.75,8.13,"GROUND FLOOR + STREET FACING",13,BLUE,ha="center",weight="bold")
text(11.75,8.13,"ALL OTHER FACADES",13,BLUE,ha="center",weight="bold")
edge((6.0,9.0),(3.75,8.45))
edge((8.9,9.0),(11.75,8.45))
text(3.75,7.71,"split X: side | entrance | side",12.5,PURPLE,ha="center")
text(11.75,7.71,"repeat X: bays equal regions",12.5,PURPLE,ha="center")

groups = card(2.45,6.48,3.8,1.1,"BayGroup × 2","One on each side of entrance")
entrance = card(6.75,6.48,3.8,1.1,"Entrance","Door Window + canopy")
bay = card(12.1,6.48,4.6,1.1,"Bay","1 Window + optional Balcony")
bus((3.75,7.4),[(2.45,7.06),(6.75,7.06)],7.27)
edge((11.75,7.4),(12.1,7.06))

# Bay groups repeat into the same Bay production shown at right.
ax.plot([2.45,2.45,9.35],[5.9,5.45,5.45],color=BLUE,lw=1.5,zorder=1)
edge((9.35,5.45),(10.05,5.91))
text(5.35,5.45,"repeat X: max(1, floor(bays / 2))",12.5,PURPLE,ha="center",
     bbox=dict(facecolor=BG,edgecolor="none",pad=2),zorder=5)

canopy = card(3.7,3.72,3.1,1.0,"Canopy","1 box",terminal=True)
window = card(8.0,3.72,4.1,1.0,"Window","7 terminal boxes",badge="B")
balcony = card(12.8,3.72,3.7,1.0,"Balcony","4 terminal boxes",optional=True,badge="C")
edge((6.15,5.9),(3.7,4.25))
edge((7.3,5.9),(8.0,4.25))
edge((11.3,5.9),(8.8,4.25))
edge((12.8,5.9),(12.8,4.25),color=GREEN,style="--")
text(13.5,5.02,"only if eligible",12,GREEN,ha="center",
     bbox=dict(facecolor=BG,edgecolor="none",pad=1))
text(12.75,2.66,"Apartment + upper floor\n+ street facing + balconies enabled",12,GREEN,
     ha="center",linespacing=1.5)
text(7.8,2.48,"Entrance reuses Window\nwith door proportions",12.5,MUTED,ha="center",linespacing=1.5)
text(3.5,2.48,"Entrance width:\nOffice 1.5 · Apartment 1.0",12.5,MUTED,ha="center",linespacing=1.5)

# Three terminal-detail panels with code-faithful sketches.
panel(16.4,12.15,9.1,4.25,"A  ROOF → 5 Slab terminals","expandRoof · inset deck + four parapet walls")
# Roof tray: separate deck and four parapets, seen from above.
roof_origin = (18.45,13.55)
iso_block((0,0,.85),(2.8,.35,.1),roof_origin)
iso_block((-1.35,0,0),(.1,.35,1.6),roof_origin)
iso_block((0,-.15,0),(2.6,.1,1.6),roof_origin,AMBER,"#fff1d7")
iso_block((1.35,0,0),(.1,.35,1.6),roof_origin)
iso_block((0,0,-.85),(2.8,.35,.1),roof_origin)
text(20.65,14.65,"1 × roof deck",16,AMBER,weight="bold",zorder=5)
text(20.65,14.08,"4 × parapet walls",16,AMBER,weight="bold",zorder=5)
text(20.65,13.35,"Deck: Material::None\nEdges: Office Stone\n             Apartment Wood",13,MUTED,linespacing=1.55,zorder=5)
text(16.75,12.54,"All five children are terminal boxes.",12.5,MUTED,zorder=5)

panel(16.4,6.9,9.1,4.85,"B  WINDOW → 7 Slab terminals","expandWindow · also used for entrance doors")
window_sketch(17.0,8.18,2.55,2.18)
text(20.65,10.18,"1 × top frame",15,AMBER,zorder=5)
text(20.65,9.72,"2 × side frames",15,AMBER,zorder=5)
text(20.65,9.26,"1 × centre mullion",15,AMBER,zorder=5)
text(20.65,8.80,"1 × sill / bottom frame",15,AMBER,zorder=5)
text(20.65,8.34,"2 × Glass panes",15,BLUE,weight="bold",zorder=5)
text(16.75,7.45,"Frame + sill: Office Wood / Apartment Stone",12.5,MUTED,zorder=5)

panel(16.4,1.55,9.1,4.85,"C  BALCONY → 4 Slab terminals","expandBalcony · optional apartment front component")
box(17.0,2.9,2.8,.15,.55,AMBER,"#fff1d7")
# Side guards and front guard shown as separate architectural blocks.
box(17.0,3.05,.12,.88,.55,GREEN,"#e9f7ef")
box(19.68,3.05,.12,.88,.55,GREEN,"#e9f7ef")
box(17.12,3.05,2.56,.88,.10,GREEN,"#eff8f3")
text(20.65,4.84,"1 × floor slab",16,AMBER,weight="bold",zorder=5)
text(20.65,4.24,"1 × front guard",15,GREEN,zorder=5)
text(20.65,3.74,"2 × side guards",15,GREEN,zorder=5)
text(20.65,3.05,"Slab: Stone\nGuards: Wood",13,MUTED,linespacing=1.5,zorder=5)
text(16.75,2.03,"No balcony on the rear or ground-floor facade.",12.5,MUTED,zorder=5)

# Footer explains the engine/rule boundary and coordinate convention.
ax.plot([.5,25.5],[1.17,1.17],color="#cbd6e3",lw=1)
text(.55,.77,"ENGINE",12.5,PURPLE,weight="bold")
text(1.65,.77,"GenerateTree → Expand(node) → create immediate children → recurse",13,INK)
text(.55,.31,"POSES",12.5,PURPLE,weight="bold")
text(1.65,.31,"Tree: local position + rotation, absolute size.  CollectTerminals: compose world poses.  Every Slab draws a scaled unit box.",12.5,MUTED)
text(25.45,17.55,"RULE MAP",12,MUTED,ha="right",weight="bold")
text(25.45,17.10,"Based on current source · 2026-11-03",12,MUTED,ha="right")

fig.savefig(OUT/"building-rules-hierarchy.png",dpi=180,facecolor=BG)
fig.savefig(OUT/"building-rules-hierarchy.svg",facecolor=BG)
plt.close(fig)
print(OUT/"building-rules-hierarchy.png")
print(OUT/"building-rules-hierarchy.svg")
