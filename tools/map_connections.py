#!/usr/bin/env python3
"""Create a simple SVG dependency map connecting C++ modules to sprite groups.
Usage: python3 tools/map_connections.py [project-root] [-o connections.svg]
The map is heuristic: it uses gfx:: accessor calls and generated ART_* names.
"""
import argparse, pathlib, re

def main():
 ap=argparse.ArgumentParser();ap.add_argument('root',nargs='?',default='.');ap.add_argument('-o','--output',default='sprite_code_connections.svg');a=ap.parse_args();root=pathlib.Path(a.root)
 groups={'Enemies':'enemy','Icons':'icon','Spell art':'spellArt','Spell icons':'spellIcon','Projectiles':'proj','Player cosmetics':'knight'}
 files=sorted((root/'source').glob('*.cpp'))
 links=[]
 for f in files:
  text=f.read_text(errors='ignore')
  for label,fn in groups.items():
   if re.search(r'gfx::\s*'+re.escape(fn)+r'\s*\(',text): links.append((f.name,label))
  for macro,label in [('ART_ENEMY','Enemies'),('ART_ICON','Icons'),('ART_SPELL','Spell art'),('ART_HAT|ART_FACE','Player cosmetics')]:
   if re.search(macro,text): links.append((f.name,label))
 nodes=sorted({x for pair in links for x in pair}); left=sorted({x[0] for x in links}); right=sorted({x[1] for x in links})
 width=940;height=max(220,80+max(len(left),len(right))*56); coords={}
 for i,n in enumerate(left):coords[n]=(20,60+i*56)
 for i,n in enumerate(right):coords[n]=(650,60+i*56)
 paths=[]
 for src,dst in sorted(set(links)):
  x,y=coords[src];X,Y=coords[dst];paths.append(f'<path d="M {x+250} {y+18} C 470 {y+18}, 470 {Y+18}, {X} {Y+18}" fill="none" stroke="#7769a9" stroke-width="2" opacity=".8"/>')
 boxes=[]
 for n,(x,y) in coords.items():
  w=250 if x<100 else 250;color='#28213f' if x<100 else '#34284d';stroke='#5c4a80'
  boxes.append(f'<rect x="{x}" y="{y}" width="{w}" height="36" rx="8" fill="{color}" stroke="{stroke}"/><text x="{x+12}" y="{y+23}" fill="#f0ead6" font-family="sans-serif" font-size="14">{n}</text>')
 svg=f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}"><rect width="100%" height="100%" fill="#100d1d"/><text x="20" y="30" fill="#e8b54b" font-family="sans-serif" font-size="20">Abyssal Eclipse — Source / Sprite Connections</text><text x="20" y="48" fill="#a99fc2" font-family="sans-serif" font-size="11">Heuristic references; generated data arrays are included where referenced.</text>{''.join(paths)}{''.join(boxes)}</svg>'
 pathlib.Path(a.output).write_text(svg,encoding='utf-8');print(f'wrote {a.output} ({len(set(links))} connections)')
if __name__=='__main__':main()
