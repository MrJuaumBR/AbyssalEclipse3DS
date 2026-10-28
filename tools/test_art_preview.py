import json,re,unittest
from pathlib import Path
from art_preview import raster,SIZES
from art_raster import rects
class ArtTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.data=json.loads((Path(__file__).parent / "data" / "art.json").read_text())
 def test_all_art_is_rect_only_and_valid(self):
  for g,(w,h) in SIZES.items():
   for n,s in self.data[g].items():
    self.assertIsNone(re.search(r"<(?!rect\b|/rect\b)[A-Za-z]",s),f"{g}.{n}")
    raster(s,w,h)
 def test_hat_bounds_are_inside_canvas(self):
  for name, art in self.data["HAT"].items():
   pixels = rects(art, size=32)
   for x, y, w, h, color in pixels:
    self.assertGreaterEqual(x, 0, name)
    self.assertGreaterEqual(y, 0, name)
    self.assertLessEqual(x + w, 32, name)
    self.assertLessEqual(y + h, 32, name)
 def test_weapon_keys(self):
  for n in ("pistol","shotgun","katana","machine_gun"):self.assertIn(n,self.data["ICO"])
 def test_expected_sprite_groups(self):
  self.assertEqual(set(SIZES),{"ENEMY","HAT","FACE","ICO"})
if __name__=="__main__":unittest.main()
