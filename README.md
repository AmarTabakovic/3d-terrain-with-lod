# 3d-terrain-level-of-detail
Project 2 by Amar Tabakovic

## Repository Structure
This repository is structured as follows:

- [`src`](src): contains the source code of the terrain LOD demo application written in C++ and OpenGL (the name *atlod* (A Terrain LOD (Renderer)) is just a temporary name I came up with for the demo application)
- [`lib`](lib): contains third-party libraries for OpenGL (GLFW, GLEW, GLM) in form of Git submodules
- [`report`](report): contains the $\text{\LaTeX}$ source code and the generated PDF of the project report
- [`research`](research): contains relevant research publications on terrain LOD (mainly for learning about the various approaches)

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
