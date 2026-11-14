#!/usr/bin/env python3
"""
Spherical geodesic lithophane generator.
Original algorithm (2018-03-01) by Uwe Zimmermann (uwe.zimmermann@sciencetronics.com).
Updated with CLI arguments, presets, and flexible paths for the SmartLamp project.
"""

import os
import sys
import argparse
from math import cos, sin, sqrt, acos, asin, atan, pi

# Lazy / checked imports for third-party packages
np = None
mesh = None
Image = None
tqdm = None

def check_dependencies():
    global np, mesh, Image, tqdm
    try:
        import numpy as _np
        from stl import mesh as _mesh
        from PIL import Image as _Image
        from tqdm import tqdm as _tqdm
        np = _np
        mesh = _mesh
        Image = _Image
        tqdm = _tqdm
    except ImportError as e:
        sys.exit(
            f"Error: Missing required dependency ({e}).\n"
            "Please install the required packages:\n"
            "    pip install numpy numpy-stl matplotlib pillow tqdm\n"
        )

script_dir = os.path.dirname(os.path.abspath(__file__))

PRESETS = {
    'moon': {
        'input': 'NASA_Moon_512px.png',
        'output': 'Moon.stl',
        'dia': 116.0,
        'wall': 2.56,
        'inner_depth': 1.76,
        'outer_depth': 0.0,
        'invert': True,
        'normalize': True,
    },
    'earth': {
        'input': 'NASA_WorldMap_1024px.png',
        'output': 'Earth.stl',
        'dia': 116.0,
        'wall': 2.56,
        'inner_depth': 1.76,
        'outer_depth': 0.0,
        'invert': False,
        'normalize': False,
    },
    'mars': {
        'input': 'mars_1k_topo.jpg',
        'output': 'Mars.stl',
        'dia': 116.0,
        'wall': 2.56,
        'inner_depth': 1.76,
        'outer_depth': 0.0,
        'invert': True,
        'normalize': True,
    },
}

def resolve_path(filename):
    if os.path.isabs(filename):
        return filename
    parent = os.path.dirname(filename)
    if os.path.exists(filename) or (parent and os.path.exists(parent)):
        return os.path.abspath(filename)
    return os.path.join(script_dir, filename)

def long(vector, longsteps):
    if vector[0] > 0:
        long_val = longsteps / 2 + atan(vector[1] / vector[0]) * longsteps / (2 * pi)
    elif vector[0] < 0:
        long_val = longsteps + atan(vector[1] / vector[0]) * longsteps / (2 * pi)
    else:
        long_val = longsteps / 2
    return int(long_val) % longsteps

def lat(vector, latsteps):
    longrad = sqrt(vector[0] * vector[0] + vector[1] * vector[1])
    if longrad != 0:
        lat_val = latsteps / 2 - atan(vector[2] / longrad) * latsteps / pi
    elif vector[2] > 0:
        lat_val = 0
    else:
        lat_val = latsteps - 1
    return int(lat_val)

def vnormalize(vector):
    result = np.empty([3])
    length = sqrt(np.dot(vector, vector))
    if length > 0:
        result[0] = vector[0] / length
        result[1] = vector[1] / length
        result[2] = vector[2] / length
    return result

def divide_triangle(corner_1, corner_2, corner_3, i, j, freq):
    corner_1 = vnormalize(corner_1)
    corner_2 = vnormalize(corner_2)
    corner_3 = vnormalize(corner_3)
    alpha_v1 = acos(np.dot(corner_1, corner_2))
    beta_v1  = i * alpha_v1 / freq
    alpha_v2 = acos(np.dot(corner_1, corner_3))
    beta_v2  = i * alpha_v2 / freq
    v1       = vnormalize(corner_1 + vnormalize(corner_2 - corner_1) * sin(beta_v1) / (sin(pi / 2 + alpha_v1 / 2 - beta_v1)))
    v2       = vnormalize(corner_1 + vnormalize(corner_3 - corner_1) * sin(beta_v2) / (sin(pi / 2 + alpha_v2 / 2 - beta_v2)))
    alpha_v3 = acos(np.dot(v1, v2))
    beta_v3  = j * alpha_v3 / i
    vertex   = vnormalize(v1 + vnormalize(v2 - v1) * sin(beta_v3) / (sin(pi / 2 + alpha_v3 / 2 - beta_v3)))
    return vertex

def geodesic(freq, pix, longsteps, latsteps, outer_radius, inner_radius, outer_delta, inner_delta, progress=True):
    corners = np.empty([12, 3])
    vertices = np.zeros([2, 3 * freq + 1, 5 * freq, 3])
    corners[0] = [0, 0, 1]
    a = 4 / sqrt(2 * (5 + sqrt(5)))
    zc = 1 - a * a / 2
    r = sqrt(1 - zc * zc)
    i = 0
    while i < 5:
        corners[i + 1] = [r * cos(i * 2 * pi / 5),       r * sin(i * 2 * pi / 5),        zc]
        corners[i + 6] = [r * cos((i - 0.5) * 2 * pi / 5), r * sin((i - 0.5) * 2 * pi / 5), -zc]
        i = i + 1
    corners[11] = [0, 0, -1]
    vertices[0, 0, 0] = corners[0]

    thisrange = tqdm(range(1, freq + 1), desc="Northern hemisphere") if progress else range(1, freq + 1)
    for i in thisrange:
        j = 0
        while j < i:
            k = 0
            while k < 5:
                vertices[0, i, j + k * i] = vnormalize(divide_triangle(corners[0],
                                                                        corners[(k % 5) + 1],
                                                                        corners[((k + 1) % 5) + 1],
                                                                        i, j, freq))
                k = k + 1
            j = j + 1

    thisrange = tqdm(range(1, freq), desc="Equatorial rows") if progress else range(1, freq + 1)
    for i in thisrange:
        j = 0
        while j < i:
            k = 0
            while k < 5:
                vertices[0, i + freq, j + k * freq] = vnormalize(divide_triangle(corners[k + 1],
                                                                                  corners[(k % 5) + 6],
                                                                                  corners[((k + 1) % 5) + 6],
                                                                                  i, j, freq))
                k = k + 1
            j = j + 1
        j = 0
        while j <= (freq - 1 - i):
            k = 0
            while k < 5:
                vertices[0, i + freq, j + k * freq + i] = vnormalize(divide_triangle(corners[((k + 1) % 5) + 6],
                                                                                      corners[(k % 5) + 1],
                                                                                      corners[((k + 1) % 5) + 1],
                                                                                      (freq - i), j, freq))
                k = k + 1
            j = j + 1

    i = 3 * freq
    vertices[0, i, 0] = vertices[0, 0, 0] * [1, 1, -1]
    rotsin = sin(-pi / 5)
    rotcos = cos(-pi / 5)

    thisrange = tqdm(range(1, freq + 1), desc="Southern hemisphere") if progress else range(1, freq + 1)
    for i in thisrange:
        j = 0
        while j < 5 * i:
            vertices[0, 3 * freq - i, j] = [vertices[0, i, j, 0] * rotcos - vertices[0, i, j, 1] * rotsin,
                                            vertices[0, i, j, 0] * rotsin + vertices[0, i, j, 1] * rotcos,
                                            -vertices[0, i, j, 2]]
            j = j + 1

    print('Outer radius = {:.1f} mm, Inner radius = {:.1f} mm'.format(outer_radius, inner_radius))

    thisrange = tqdm(range(3 * freq + 1), desc="Carving relief") if progress else range(3 * freq + 1)
    for i in thisrange:
        for k in range(5 * freq):
            vector = vertices[0, i, k].copy()
            pixel  = pix[long(vector, longsteps), lat(vector, latsteps), 0]
            vertices[0, i, k] = vector * (outer_radius - outer_delta * pixel)
            vertices[1, i, k] = vector * (inner_radius + inner_delta * pixel)
    return vertices

def geodesic_mesh(freq, pix, longsteps, latsteps, outer_radius, inner_radius, outer_delta, inner_delta, progress=True):
    obj = mesh.Mesh(np.zeros(20 * freq * freq * 2, dtype=mesh.Mesh.dtype))
    vertices = geodesic(freq, pix, longsteps, latsteps, outer_radius, inner_radius, outer_delta, inner_delta, progress)
    n = 0

    thisrange = tqdm(range(0, freq), desc="Building mesh polar") if progress else range(0, freq)
    for i in thisrange:
        j = 0
        while (j < i) or ((j == 0) and (i == 0)):
            k = 0
            while k < 5:
                obj.vectors[n] = [vertices[0, i, k * i + j],
                                  vertices[0, i + 1, k * (i + 1) + j],
                                  vertices[0, i + 1, (k * (i + 1) + j + 1) % (5 * (i + 1))]]
                n = n + 1

                obj.vectors[n] = [vertices[1, i + 1, k * (i + 1) + j],
                                  vertices[1, i, k * i + j],
                                  vertices[1, i + 1, (k * (i + 1) + j + 1) % (5 * (i + 1))]]
                n = n + 1

                if i > 0:
                    obj.vectors[n] = [vertices[0, i, k * i + j],
                                      vertices[0, i + 1, (k * (i + 1) + j + 1) % ((i + 1) * 5)],
                                      vertices[0, i, (k * i + j + 1) % (5 * i)]]
                    n = n + 1

                    obj.vectors[n] = [vertices[1, i + 1, (k * (i + 1) + j + 1) % ((i + 1) * 5)],
                                      vertices[1, i, k * i + j],
                                      vertices[1, i, (k * i + j + 1) % (5 * i)]]
                    n = n + 1

                if (j == 0) and (i > 0):
                    obj.vectors[n] = [vertices[0, i, k * i + j],
                                      vertices[0, i + 1, ((k + 5 - 1) % 5) * (i + 1) + i],
                                      vertices[0, i + 1, k * (i + 1) + j]]
                    n = n + 1

                    obj.vectors[n] = [vertices[1, i + 1, ((k + 5 - 1) % 5) * (i + 1) + i],
                                      vertices[1, i, k * i + j],
                                      vertices[1, i + 1, k * (i + 1) + j]]
                    n = n + 1

                # southern hemisphere
                obj.vectors[n] = [vertices[0, (5 - 2) * freq - (i + 1), k * (i + 1) + j],
                                  vertices[0, (5 - 2) * freq - i, k * i + j],
                                  vertices[0, (5 - 2) * freq - (i + 1), (k * (i + 1) + j + 1) % ((i + 1) * 5)]]
                n = n + 1

                obj.vectors[n] = [vertices[1, (5 - 2) * freq - i, k * i + j],
                                  vertices[1, (5 - 2) * freq - (i + 1), k * (i + 1) + j],
                                  vertices[1, (5 - 2) * freq - (i + 1), (k * (i + 1) + j + 1) % ((i + 1) * 5)]]
                n = n + 1

                if i > 0:
                    obj.vectors[n] = [vertices[0, (5 - 2) * freq - (i + 1), (k * (i + 1) + j + 1) % ((i + 1) * 5)],
                                      vertices[0, (5 - 2) * freq - i, k * i + j],
                                      vertices[0, (5 - 2) * freq - i, (k * i + j + 1) % (i * 5)]]
                    n = n + 1

                    obj.vectors[n] = [vertices[1, (5 - 2) * freq - i, k * i + j],
                                      vertices[1, (5 - 2) * freq - (i + 1), (k * (i + 1) + j + 1) % ((i + 1) * 5)],
                                      vertices[1, (5 - 2) * freq - i, (k * i + j + 1) % (i * 5)]]
                    n = n + 1

                if (j == 0) and (i > 0):
                    obj.vectors[n] = [vertices[0, (5 - 2) * freq - (i + 1), ((k + 5 - 1) % 5) * (i + 1) + i],
                                      vertices[0, (5 - 2) * freq - i, k * i + j],
                                      vertices[0, (5 - 2) * freq - (i + 1), k * (i + 1) + j]]
                    n = n + 1

                    obj.vectors[n] = [vertices[1, (5 - 2) * freq - i, k * i + j],
                                      vertices[1, (5 - 2) * freq - (i + 1), ((k + 5 - 1) % 5) * (i + 1) + i],
                                      vertices[1, (5 - 2) * freq - (i + 1), k * (i + 1) + j]]
                    n = n + 1

                k = k + 1
            j = j + 1

    thisrange = tqdm(range(0, freq), desc="Building mesh equatorial") if progress else range(0, freq)
    for i in thisrange:
        j = 0
        while j < freq * 5:
            obj.vectors[n] = [vertices[0, freq + i, j],
                              vertices[0, freq + i + 1, j],
                              vertices[0, freq + i + 1, (j + 1) % (5 * freq)]]
            n = n + 1

            obj.vectors[n] = [vertices[1, freq + i + 1, j],
                              vertices[1, freq + i, j],
                              vertices[1, freq + i + 1, (j + 1) % (5 * freq)]]
            n = n + 1

            obj.vectors[n] = [vertices[0, freq + i, j],
                              vertices[0, freq + i + 1, (j + 1) % (5 * freq)],
                              vertices[0, freq + i, (j + 1) % (5 * freq)]]
            n = n + 1

            obj.vectors[n] = [vertices[1, freq + i + 1, (j + 1) % (5 * freq)],
                              vertices[1, freq + i, j],
                              vertices[1, freq + i, (j + 1) % (5 * freq)]]
            n = n + 1
            j = j + 1
    return obj

def generate_mesh(mapfilename, savefilename, dia=116.0, wall=2.56, inner_depth=1.76, outer_depth=0.0, freq=200, normalize=False, invert=False, progress=True):
    check_dependencies()
    if inner_depth + outer_depth >= wall:
        raise ValueError(
            f"Total carving depth (inner={inner_depth} + outer={outer_depth} = {inner_depth + outer_depth:.2f} mm) "
            f"must be strictly less than wall thickness ({wall:.2f} mm) to prevent the inner shell from punching through the outer shell!"
        )
    in_path = resolve_path(mapfilename)
    out_path = resolve_path(savefilename)

    if not os.path.exists(in_path):
        raise FileNotFoundError(f"Input image not found: {in_path}")

    outer_radius = dia / 2.0
    inner_radius = outer_radius - wall

    print(f"Loading image from: {in_path}")
    img = Image.open(in_path)

    longsteps = 10 * freq
    latsteps  = 3 * freq + 1
    print(f"Resizing image to {longsteps}x{latsteps}...")
    img = img.convert(mode="L")
    img = img.resize((longsteps, latsteps), resample=Image.BICUBIC)

    print("Extracting pixel array...")
    pix = np.array(img.getdata(), dtype=np.float32).reshape(img.size[0], img.size[1], 1, order='F')

    if invert:
        print("Inverting heightmap values (255 - pixel)...")
        pix = 255.0 - pix

    if normalize:
        p_low = float(np.percentile(pix, 0.5))
        p_high = float(np.percentile(pix, 99.5))
        print(f"Normalizing dynamic range ({p_low:.1f}..{p_high:.1f} -> 0..255)...")
        if p_high > p_low:
            pix = np.clip((pix - p_low) / (p_high - p_low) * 255.0, 0, 255.0)

    max_pix = np.amax(pix)
    if max_pix == 0:
        max_pix = 1
    inner_delta = inner_depth / max_pix
    outer_delta = outer_depth / max_pix

    print(f"Generating geodesic mesh (freq={freq})...")
    c = geodesic_mesh(freq, pix, longsteps, latsteps, outer_radius, inner_radius, outer_delta, inner_delta, progress)

    print(f"Saving STL to: {out_path}")
    c.save(out_path)
    print("Done!")

def main():
    parser = argparse.ArgumentParser(
        description="Generate 3D spherical lithophane STL meshes from heightmap images."
    )
    parser.add_argument(
        "-p", "--preset",
        choices=["moon", "earth", "mars"],
        help="Use preset settings (moon, earth, or mars)"
    )
    parser.add_argument(
        "-i", "--input",
        help="Input heightmap image file (e.g. NASA_Moon_512px.png)"
    )
    parser.add_argument(
        "-o", "--output",
        help="Output STL file path (e.g. Moon.stl)"
    )
    parser.add_argument(
        "-d", "--dia",
        type=float,
        default=116.0,
        help="Outer globe diameter in mm (default: 116.0)"
    )
    parser.add_argument(
        "-w", "--wall",
        type=float,
        default=2.56,
        help="Undisturbed wall thickness in mm (default: 2.56)"
    )
    parser.add_argument(
        "--inner-depth",
        type=float,
        default=1.76,
        help="Carving depth on the inside in mm (default: 1.76)"
    )
    parser.add_argument(
        "--outer-depth",
        type=float,
        default=0.0,
        help="Carving depth on the outside in mm (default: 0.0)"
    )
    parser.add_argument(
        "-f", "--freq",
        type=int,
        default=200,
        help="Subdivision factor / resolution (default: 200; use 10-30 for fast previews)"
    )
    parser.add_argument(
        "-n", "--normalize",
        action="store_true",
        help="Normalize/stretch heightmap dynamic range to full 0..255 (useful for low-contrast images)"
    )
    parser.add_argument(
        "--invert",
        action="store_true",
        help="Invert heightmap values (makes light areas dark and dark areas light)"
    )
    parser.add_argument(
        "--no-progress",
        action="store_true",
        help="Disable progress bars"
    )

    args = parser.parse_args()

    mapfilename = args.input
    savefilename = args.output
    dia = args.dia
    wall = args.wall
    inner_depth = args.inner_depth
    outer_depth = args.outer_depth

    invert = args.invert
    normalize = args.normalize

    if args.preset:
        preset = PRESETS[args.preset]
        if not mapfilename:
            mapfilename = preset['input']
        if not savefilename:
            savefilename = preset['output']
        if args.dia == 116.0:
            dia = preset['dia']
        if args.wall == 2.56:
            wall = preset['wall']
        if args.inner_depth == 1.76:
            inner_depth = preset['inner_depth']
        if args.outer_depth == 0.0:
            outer_depth = preset['outer_depth']
        if not args.invert and 'invert' in preset:
            invert = preset['invert']
        if not args.normalize and 'normalize' in preset:
            normalize = preset['normalize']
    elif not mapfilename:
        print("No input or preset specified. Defaulting to '--preset moon'. Use -h for help.")
        preset = PRESETS['moon']
        mapfilename = preset['input']
        savefilename = savefilename or preset['output']
        if not args.invert and 'invert' in preset:
            invert = preset['invert']
        if not args.normalize and 'normalize' in preset:
            normalize = preset['normalize']

    if not savefilename:
        base = os.path.splitext(os.path.basename(mapfilename))[0]
        savefilename = f"{base}.stl"

    progress = not args.no_progress

    generate_mesh(
        mapfilename=mapfilename,
        savefilename=savefilename,
        dia=dia,
        wall=wall,
        inner_depth=inner_depth,
        outer_depth=outer_depth,
        freq=args.freq,
        normalize=normalize,
        invert=invert,
        progress=progress
    )

if __name__ == "__main__":
    main()
