#!/usr/bin/env python3
import argparse,json,math,re
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
SIZES={"ENEMY":(16,16),"HAT":(32,32),"FACE":(32,32),"ICO":(16,16)}
RR=re.compile(r'<rect\s+([^>]+)/>'); AR=re.compile(r'([A-Za-z]+)="([^"]*)"')
def raster(s,w,h):
 im=Image.new("RGBA",(w,h),(0,0,0,0)); d=ImageDraw.Draw(im)
 for tag in RR.findall(s):
  a=dict(AR.findall(tag)); x,y,rw,rh=[int(float(a[k])) for k in ("x","y","width","height")]; col=a["fill"]
  if not re.fullmatch(r"#[0-9A-Fa-f]{6}",col) or min(rw,rh)<=0 or min(x,y)<0 or x+rw>w or y+rh>h: raise ValueError(f"invalid rect {a}")
  d.rectangle((x,y,x+rw-1,y+rh-1),fill=tuple(bytes.fromhex(col[1:]))+(255,))
 return im
def make(items,path,cols=6,cw=112,ch=116,scale=3):
 im=Image.new("RGB",(cols*cw,math.ceil(len(items)/cols)*ch),(23,22,34)); d=ImageDraw.Draw(im); f=ImageFont.load_default()
 for i,(g,n,s) in enumerate(items):
  x=i%cols*cw;y=i//cols*ch;z=s.resize((s.width*scale,s.height*scale),Image.Resampling.NEAREST);im.paste(z,(x+(cw-z.width)//2,y+8),z)
  d.text((x+7,y+ch-26),f"{g}.{n}",font=f,fill=(238,233,220));d.text((x+7,y+ch-13),f"{s.width}x{s.height}",font=f,fill=(157,154,178))
 im.save(path)
def main():
 p=argparse.ArgumentParser();p.add_argument("--art",default="art.json");p.add_argument("--out",default="previews");p.add_argument("--strict",action="store_true");a=p.parse_args()
 d=json.loads(Path(a.art).read_text());o=Path(a.out);o.mkdir(parents=True,exist_ok=True);items=[];errors=[];count=0
 for g,(w,h) in SIZES.items():
  for n,art in d.get(g,{}).items():
   count+=1
   if re.search(r"<(?!rect\b|/rect\b)[A-Za-z]",art):errors.append(f"{g}.{n}: unsupported element")
   try:items.append((g,n,raster(art,w,h)))
   except Exception as e:errors.append(f"{g}.{n}: {e}")
 make(items,o/"contact_sheet_json_sprites.png")
 for g in SIZES:make([i for i in items if i[0]==g],o/f"{g.lower()}_sheet.png",5,128,126,4 if g in ("ENEMY","ICO") else 3)
 print(f"Checked {count} entries; rendered {len(items)}")
 print("PASS" if not errors else "\\n".join(errors))
 if errors and a.strict:raise SystemExit(1)
if __name__=="__main__":main()
