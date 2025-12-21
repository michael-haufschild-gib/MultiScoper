from PIL import Image
import numpy as np
import sys

try:
    img = Image.open("failed_render.png")
    print(f"Size: {img.size}")
    print(f"Mode: {img.mode}")
    
    data = np.array(img)
    print(f"Min: {data.min()}, Max: {data.max()}")
    print(f"Mean: {data.mean()}")
    
    # Check if all pixels are 0
    if data.max() == 0:
        print("Image is completely empty (all zeros)")
    else:
        print("Image has content")
        
except Exception as e:
    print(f"Error: {e}")
