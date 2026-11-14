# Lampshade Patterns (CAD Patterns)

This directory contains the scripts and elevation/heightmap textures used to generate the textured spherical lampshades (e.g. Moon, Earth, or Mars) for the SmartLamp.

## Included Files

* **`NASA_Moon_512px.png`**: Elevation heightmap of the Moon's surface (NASA Elevation Data, inverted & normalized, 512×256).
* **`NASA_WorldMap_1024px.png`**: Coarser, 3D-print-optimized topographic map of Earth (1024×512, smoothed coastlines to prevent printer jitter/micro-extrusions). *Default for the `--preset earth`.*
* **`NASA_WorldMap_512px.png`**: Extra-coarse topographic map of Earth (512×256, matches Moon resolution).
* **`NASA_WorldMap2.png`**: Ultra-high-resolution original topographic relief map of Earth (4320×2160).
* **`mars_1k_topo.jpg`**: Topographic elevation map of the Martian surface (inverted & normalized, 1024×512).
* **`geodesic_mapped_double.py`**: Python script by Uwe Zimmermann for generating a geodesic spherical mesh with relief carving (`inner_depth` / `outer_depth`) based on grayscale heightmaps. Generates STL meshes (e.g. `Moon.stl`, `Earth.stl`, or `Mars.stl`).

## Lithophane Principle & Heightmap Orientation

The SmartLamp lampshade relies on internal lithophane carving:
* **The outer surface is a smooth sphere** (`outer_depth = 0.0`), giving the lamp an elegant, clean globe appearance when powered off.
* **The relief is carved into the interior wall** (`inner_depth = 2.2`):
  * **White / bright pixels (`255`)**: The inner surface is carved closer to the outer shell, leaving only a thin base wall (`wall - inner_depth`). This allows light from the internal LEDs to shine through brightly.
  * **Dark / black pixels (`0`)**: Material is preserved up to the full wall thickness (`wall`), creating thicker relief protruding inward into the sphere cavity that blocks more light and appears darker.

### Visual Rule: High structures appear dark, depressions appear bright
In this project, elevated structures (mountains, crater rims, volcanoes, continents) are designed to appear **darker** when illuminated, while depressions (oceans, lunar maria, low plains) shine **brightly**:

| Celestial Body | Bright / Thin Wall (Light Transmitted) | Dark / Thick Wall (Silhouettes / Shadows) |
| :--- | :--- | :--- |
| **Earth** (`NASA_WorldMap2.png`) | Oceans & seas (pixels ~255) | Continents & mountain ranges (pixels ~100–120) |
| **Moon** (`NASA_Moon_512px.png`) | Lunar maria / basalt plains (pixels ~195) | Highlands, mountain ranges & crater rims (pixels ~100) |
| **Mars** (`mars_1k_topo.jpg`) | Northern lowlands & Hellas basin (pixels ~220) | Olympus Mons & Tharsis volcanic shields (pixels ~48) |

> **Tip for Custom Heightmaps:** Standard scientific DEMs (Digital Elevation Models) typically map high elevations to white and low depressions to black. When using third-party DEMs, pass the `--invert` flag (and optionally `--normalize`) so that mountains become dark and lowlands become bright:
> ```bash
> python cad/patterns/geodesic_mapped_double.py -i custom_topo.png -o Custom.stl --invert --normalize
> ```

## Generating STLs with Python

### Prerequisites
```bash
pip install numpy numpy-stl matplotlib pillow tqdm
```

### Usage via CLI Presets

You can run the script from any directory; output STL files are saved directly into `cad/patterns/` by default.

* **Generate Moon:**
  ```bash
  python cad/patterns/geodesic_mapped_double.py --preset moon
  ```
* **Generate Earth:**
  ```bash
  python cad/patterns/geodesic_mapped_double.py --preset earth
  ```
* **Generate Mars:**
  ```bash
  python cad/patterns/geodesic_mapped_double.py --preset mars
  ```

### Advanced / Custom Parameters

```bash
# Fast low-res preview render (freq=25 instead of 200)
python cad/patterns/geodesic_mapped_double.py --preset moon --freq 25 -o Moon_preview.stl

# Custom heightmap image and dimensions
python cad/patterns/geodesic_mapped_double.py -i my_map.png -o Custom.stl --dia 116 --wall 2.0 --inner-depth 2.2 --invert --normalize
```

Available options (`python cad/patterns/geodesic_mapped_double.py --help`):
* `-p, --preset {moon,earth,mars}`: Select predefined texture map and output filename.
* `-i, --input INPUT`: Input grayscale image path.
* `-o, --output OUTPUT`: Output STL file path.
* `-d, --dia DIA`: Outer diameter in mm (default: 116.0).
* `-w, --wall WALL`: Base wall thickness in mm (default: 2.0).
* `--inner-depth`: Carving depth on inside in mm (default: 2.2).
* `--outer-depth`: Carving depth on outside in mm (default: 0.0).
* `-f, --freq FREQ`: Subdivision factor / mesh resolution (default: 200; use 20–50 for quick test renders).
* `-n, --normalize`: Automatically stretch/normalize the heightmap dynamic range to the full 0–255 scale (useful for low-contrast images).
* `--invert`: Invert heightmap values (makes high areas dark and low areas bright, or vice versa).
* `--no-progress`: Disable CLI progress bar.

## Usage in OpenSCAD

The generated STL file is automatically imported in `cad/SmartLamp.scad` within the `bulb()` module via `pattern_dir = "patterns/";`:
```scad
if(Scene == "Moon")
    import(str(pattern_dir, "Moon.stl"));
else if(Scene == "Earth")
    import(str(pattern_dir, "Earth.stl"));
else if(Scene == "Mars")
    import(str(pattern_dir, "Mars.stl"));
```

> [!IMPORTANT]
> **Bottom Trimming in `SmartLamp.scad`:**  
> When `SmartLamp.scad` renders the `bulb()` module, the bottom portion of the sphere (everything south of approximately -35° latitude, including the South Pole) is cut away by cylindrical difference operations to create the mounting collar that fits onto the lamp enclosure and LED ring seat. Keep this in mind when designing or inspecting heightmaps: any features near the south pole of the map (e.g. Antarctica on Earth or the South Pole-Aitken basin on the Moon) will reside in this trimmed base area and will not appear on the finished lampshade.

> **Note on File Sizes:** The generated STL files are approximately 80 MB each and are therefore not tracked in Git (`*.stl` in `.gitignore`).
