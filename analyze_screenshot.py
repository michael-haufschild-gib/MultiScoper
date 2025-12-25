from PIL import Image
import numpy as np
import sys

try:
    img = Image.open("/tmp/debug_fail_existence.png")
    print(f"Image format: {img.format}, Size: {img.size}, Mode: {img.mode}")
    
    arr = np.array(img)
    print(f"Shape: {arr.shape}")
    
    # Calculate unique colors
    # Reshape to list of pixels
    pixels = arr.reshape(-1, arr.shape[-1])
    unique_colors, counts = np.unique(pixels, axis=0, return_counts=True)
    
    print(f"Unique colors count: {len(unique_colors)}")
    
    # Sort by count
    sorted_indices = np.argsort(-counts)
    
    print("Top 10 colors:")
    for i in range(min(10, len(unique_colors))):
        idx = sorted_indices[i]
        color = unique_colors[idx]
        count = counts[idx]
        pct = count / len(pixels) * 100
        print(f"  {color}: {count} ({pct:.2f}%)")
        
    # Check if completely black/transparent
    if len(unique_colors) == 1:
        print("IMAGE IS SOLID COLOR!")
        
except Exception as e:
    print(f"Error analyzing image: {e}")
