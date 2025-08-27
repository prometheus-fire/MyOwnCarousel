import argparse
import os
from PIL import Image

def split_cruciform_skybox(image_path, output_dir):
    img = Image.open(image_path)
    width, height = img.size

    # Calcola dimensione quadrata della faccia
    face_width = width // 4
    face_height = height // 3
    face_size = min(face_width, face_height)

    # Posizioni corrette nel layout cruciforme
    face_positions = {
        'pos_x':  (2 * face_size, face_size),  # right
        'neg_x':  (0 * face_size, face_size),  # left
        'pos_y':  (1 * face_size, 0 * face_size),  # top
        'neg_y':  (1 * face_size, 2 * face_size),  # bottom
        'pos_z':  (1 * face_size, 1 * face_size),  # front
        'neg_z':  (3 * face_size, 1 * face_size),  # back
    }

    os.makedirs(output_dir, exist_ok=True)

    for face_name, (x, y) in face_positions.items():
        cropped = img.crop((x, y, x + face_size, y + face_size))
        cropped.save(os.path.join(output_dir, f"{face_name}.png"))

    print(f"Skybox faces saved to '{output_dir}'.")

def main():
    parser = argparse.ArgumentParser(description="Split a cruciform skybox image into six cube face images.")
    parser.add_argument("input_image", help="Path to the cruciform skybox image.")
    parser.add_argument("output_dir", help="Directory to save the six face images.")
    args = parser.parse_args()

    split_cruciform_skybox(args.input_image, args.output_dir)

if __name__ == "__main__":
    main()
