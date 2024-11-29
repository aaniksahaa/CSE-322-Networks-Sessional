from PIL import Image
import os
from reportlab.pdfgen import canvas
from util import *

def images_to_pdf(folder_path, output_pdf):
    # Get all image files
    image_files = []

    # specify ordering
    subdirs = ['aodv', 'raodv', 'two-comp', 'multi-comp']
    
    for subdir in subdirs:
        tmp = []
        for root, _, files in os.walk(f"{folder_path}/{subdir}"):
            for file in files:
                if file.endswith(('.png', '.jpg', '.jpeg')):
                    tmp.append(os.path.join(root, file))
        tmp.sort()
        image_files.extend(tmp)
    
    # Create PDF
    c = canvas.Canvas(output_pdf)
    
    # Add each image as a new page
    for image_path in image_files:
        img = Image.open(image_path)
        img_width, img_height = img.size
        
        # Set page size to image size
        c.setPageSize((img_width, img_height))
        
        # Draw image
        c.drawImage(image_path, 0, 0, width=img_width, height=img_height)
        c.showPage()
    
    c.save()

# Usage
images_to_pdf(PLOTS_DIR, 'report.pdf')