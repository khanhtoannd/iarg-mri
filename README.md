# IARG-MRI (C++)

This repository provides a C++ implementation of **IARG-MRI** (`iarg-mri.cpp`) plus the inputs, ground-truth masks, generated masks, and numeric reports used for **Fig.1–Fig.3** and baseline comparisons.

## Repository layout

- `iarg-mri.cpp` — main implementation (Windows-oriented C++17).
- `inputs/` — input MRI images (**images**).
- `ground-truth mask/` — ground-truth segmentation masks (**images**).
- `outputs/` — predicted masks produced by the method (**images**).
- `output/` — numeric results / logs (**text files**), e.g.:
  - `ketqua.txt` — per-image **Dice** and **IoU** in a Markdown table + an average row.
  - `report.txt` — list of images that have **no ground truth** available.
- `fig1 and results/`, `fig2 and results/`, `fig3 and results/` — per-figure assets (**images + numeric results**) used to reproduce the figures.
- `result_of_baselines/` — outputs of baseline methods (images and/or numeric reports, depending on the baseline).

## Requirements

- A C++17 compiler.
- **Windows**: the current code uses `windows.h` + `FindFirstFileA()` for folder traversal and `_mkdir()`.
- `stb_image.h` and `stb_image_write.h` must be available in the include path (often placed next to `iarg-mri.cpp`).

## Build

### MSVC (Developer Command Prompt)

```bat
cl /O2 /std:c++17 iarg-mri.cpp
