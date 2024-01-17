# 3d-terrain-level-of-detail
A small terrain renderer with level of detail (LOD), developed as part of the module "Project 2" at the Bern University of Applied Sciences

![ATLOD](doc/atlod-preview.png)

## Repository Structure
This repository is structured as follows:

- [`src`](src): contains the source code of the terrain LOD demo application written in C++ and OpenGL (the name *atlod* (A Terrain LOD (Renderer)) is just a temporary name I came up with for the demo application)
- [`lib`](lib): contains libraries in form of Git submodules
- [`data`](data): contains data relevant for ATLOD, such as heightmaps and textures
- [`report`](report): contains the LaTeX source code and the generated PDF of the project report
- [`research`](research): contains relevant research publications on terrain LOD (mainly for learning about the various approaches)

## Used APIs and Libraries
- OpenGL 3.3
- GLEW
- GLFW
- GLM
- Dear ImGui
- stb_image.h

## Building
### Linux and Mac OS
```plaintext
# 1. Clone repository and submodules from GitHub
git clone --recursive https://github.com/AmarTabakovic/3d-terrain-with-lod.git

# 2. Change into the repository directory
cd 3d-terrain-with-lod

# 3. Create build folder and change into it
mkdir build
cd build

4. Run CMake
cmake ..

5. Build with make
make
```

### Running
#### Usage
The `data` folder is organized in a special way:
```plaintext
└── data
    ├── heightmaps
    ├── overlays
    └── skybox
```

All heightmap image files must be located in `heightmaps`, all overlay texture files must located be in `overlays`
and all skybox folders must be located in `skybox`. The folder containing a specific skybox 
must contain six images stored as `front.png`, `back.png`, `left.png`, `right.png`, `top.png`, `bottom.png`.
Otherwise, see the default images.

The following options are mandatory:
- Data folder path: `--data_folder_path=<folder_path>`
- Heightmap filename (only the filename, not the full path): `--heightmap_file_name=<file_name>`
The following options can be passed optionally:
- Block size (for GeoMipMapping, must be of the form 2^n + 1): `--block_size=<block_size>` (default 257)
- Overlay texture file name: `--overlay_file_name=<file_name>`
- Skybox folder name: `--skybox_folder_name=<folder_name>` (default "simple-gradient")
- Load GeoMipMapping: `--geomipmapping=<0 or 1>` (default 1)
- Load naive rendering_ `--naive_rendering=<0 or 1>` (default 0)

Important: the arguments cannot contain spaces between the `=` symbol.

Example usage:
```plaintext
$ ./atlod --data_folder_path=../3d-terrain-with-lod/data --heightmap_file_name=14k-x-14k-switzerland.png --geomipmapping=1 --naive_rendering=1
```

#### Keyboard 
- `WASD`: Movement
- Arrow keys: Look around
- `O`: Toggle options windows
- `L`: Set light direction to current direction
- `F`: Start automatic camera flying
- `R`: Start automatic 360-degree camera rotation
- `Esc`: Quit
