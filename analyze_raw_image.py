from PIL import Image
import numpy as np
import sys

try:
    img = Image.open("/tmp/debug_fail_existence_raw.png")
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
        
    # Find dominant color (background)
    bg_color = unique_colors[sorted_indices[0]]
    
    # Check for Red/Blue presence
    # Red: [255, 0, 0] approx
    # Blue: [0, 0, 255] approx
    
    def find_color_match(target, tolerance=30):
        # target is [r, g, b]
        matches = 0
        for i in range(len(unique_colors)):
            c = unique_colors[i]
            # Ignore alpha for matching
            diff = np.abs(c[:3] - target)
            if np.all(diff < tolerance):
                matches += counts[i]
        return matches

    red_pixels = find_color_match(np.array([255, 0, 0]))
    blue_pixels = find_color_match(np.array([0, 0, 255]))
    
    print(f"Red-ish pixels: {red_pixels}")
    print(f"Blue-ish pixels: {blue_pixels}")

except Exception as e:
    print(f"Error analyzing image: {e}")
