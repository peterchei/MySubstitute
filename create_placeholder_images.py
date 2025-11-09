#!/usr/bin/env python3
"""
Create placeholder images for PersonReplacementProcessor

This script creates simple placeholder images that can be used
as default targets for face swap and full body replacement.

Usage:
    python create_placeholder_images.py
"""

import cv2
import numpy as np
import os

def create_face_placeholder():
    """Create a simple face placeholder image"""
    # Create 512x512 image
    img = np.ones((512, 512, 3), dtype=np.uint8) * 240
    
    # Draw face circle
    center = (256, 256)
    radius = 150
    cv2.circle(img, center, radius, (200, 180, 160), -1)
    
    # Draw eyes
    left_eye = (200, 220)
    right_eye = (312, 220)
    cv2.circle(img, left_eye, 20, (50, 50, 50), -1)
    cv2.circle(img, right_eye, 20, (50, 50, 50), -1)
    cv2.circle(img, left_eye, 8, (255, 255, 255), -1)
    cv2.circle(img, right_eye, 8, (255, 255, 255), -1)
    
    # Draw nose
    nose_pts = np.array([[256, 250], [240, 290], [256, 285], [272, 290]], np.int32)
    cv2.polylines(img, [nose_pts], False, (150, 130, 110), 2)
    
    # Draw mouth (smile)
    cv2.ellipse(img, (256, 300), (60, 40), 0, 0, 180, (150, 50, 50), 3)
    
    # Add text
    text = "PLACEHOLDER FACE"
    font = cv2.FONT_HERSHEY_SIMPLEX
    text_size = cv2.getTextSize(text, font, 0.7, 2)[0]
    text_x = (512 - text_size[0]) // 2
    cv2.putText(img, text, (text_x, 450), font, 0.7, (100, 100, 100), 2)
    
    text2 = "Replace with your own image"
    text_size2 = cv2.getTextSize(text2, font, 0.5, 1)[0]
    text_x2 = (512 - text_size2[0]) // 2
    cv2.putText(img, text2, (text_x2, 480), font, 0.5, (120, 120, 120), 1)
    
    return img

def create_person_placeholder():
    """Create a simple full body placeholder image"""
    # Create 512x768 image (portrait orientation)
    img = np.ones((768, 512, 3), dtype=np.uint8) * 240
    
    # Draw head
    head_center = (256, 150)
    cv2.circle(img, head_center, 80, (200, 180, 160), -1)
    
    # Draw eyes
    cv2.circle(img, (230, 140), 12, (50, 50, 50), -1)
    cv2.circle(img, (282, 140), 12, (50, 50, 50), -1)
    
    # Draw body (simple rectangle)
    body_color = (100, 120, 140)
    cv2.rectangle(img, (180, 230), (332, 550), body_color, -1)
    
    # Draw arms
    cv2.rectangle(img, (120, 250), (180, 500), body_color, -1)
    cv2.rectangle(img, (332, 250), (392, 500), body_color, -1)
    
    # Draw legs
    cv2.rectangle(img, (200, 550), (270, 750), (60, 80, 100), -1)
    cv2.rectangle(img, (280, 550), (350, 750), (60, 80, 100), -1)
    
    # Add text
    text = "PLACEHOLDER PERSON"
    font = cv2.FONT_HERSHEY_SIMPLEX
    text_size = cv2.getTextSize(text, font, 0.9, 2)[0]
    text_x = (512 - text_size[0]) // 2
    
    # Draw text background
    cv2.rectangle(img, (text_x - 10, 600 - 35), (text_x + text_size[0] + 10, 600 + 10), 
                 (255, 255, 255), -1)
    cv2.putText(img, text, (text_x, 600), font, 0.9, (100, 100, 100), 2)
    
    text2 = "Replace with your own image"
    text_size2 = cv2.getTextSize(text2, font, 0.6, 1)[0]
    text_x2 = (512 - text_size2[0]) // 2
    cv2.rectangle(img, (text_x2 - 5, 650 - 25), (text_x2 + text_size2[0] + 5, 650 + 5), 
                 (255, 255, 255), -1)
    cv2.putText(img, text2, (text_x2, 650), font, 0.6, (120, 120, 120), 1)
    
    return img

def main():
    """Generate placeholder images"""
    # Create assets directory if it doesn't exist
    os.makedirs('assets', exist_ok=True)
    
    # Create face placeholder
    print("Creating face placeholder...")
    face_img = create_face_placeholder()
    face_path = os.path.join('assets', 'default_face.jpg')
    cv2.imwrite(face_path, face_img)
    print(f"✓ Created: {face_path}")
    
    # Create person placeholder
    print("Creating person placeholder...")
    person_img = create_person_placeholder()
    person_path = os.path.join('assets', 'default_person.jpg')
    cv2.imwrite(person_path, person_img)
    print(f"✓ Created: {person_path}")
    
    print("\n✓ Placeholder images created successfully!")
    print("\nUsage:")
    print("  - Face swap: Replace 'assets/default_face.jpg' with a face photo")
    print("  - Full body: Replace 'assets/default_person.jpg' with a full-body photo")
    print("\nTips:")
    print("  - Use high-quality images (at least 512x512 for faces)")
    print("  - Face images should be frontal-facing and well-lit")
    print("  - Full body images should show the entire person")
    print("  - JPG or PNG formats are supported")

if __name__ == '__main__':
    main()
