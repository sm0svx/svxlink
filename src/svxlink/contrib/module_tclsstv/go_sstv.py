#!/usr/bin/env python
import time
import subprocess
import StringIO
from PIL import Image, ImageFont, ImageDraw
 
mycallsign = "F5UII" # ham radio callsign
size= 320,256
font = ImageFont.load_default()
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18)
 
 
img = Image.open("/tmp/phot.jpg")
img.thumbnail(size, Image.ANTIALIAS)
draw = ImageDraw.Draw(img)
localtime = time.strftime("%d/%m/%Y %H:%M:%S", time.localtime(time.time()) )
draw.text((7, 7), mycallsign , (242,242,242), font=font)
draw.text((250, 7), mycallsign , (32,32,32), font=font)
draw.text((7, 220), localtime, (121,219,197), font=font)
img.save("/tmp/phot2.jpg")