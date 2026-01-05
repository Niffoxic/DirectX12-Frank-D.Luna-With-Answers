# DirectX 12 Frank Luna: Chapters with Answers

This repo is my **Scene based dx12** implementation, built while studying the  
**Frank Luna – *Introduction to 3D Game Programming with DirectX 12*** textbook.

This is **not a copy of the book** — it’s a hands-on implementation space where
each chapter becomes a runnable scene, with exercises implemented and extended
where possible.

`NOT A REPLICA`
---

## Current Output

### Chapter 4: Direct3D Initialization & First Triangle
![Chapter 4 Output](outputs/chapter-4.gif)

> Chapter 4 has **no exercise questions** in the book, so this chapter focuses
> purely on getting a clean, working DirectX 12 pipeline and output.

---

### Chapter 6: Drawing in Direct3D (All Exercises Implemented)
![Chapter 6 Final Output](outputs/chapter-6/final-output.gif)

**Showcase Video:**  
[▶ Watch Chapter 6 Showcase](outputs/chapter-6/showcase.mp4)

This chapter includes **all programming exercises**, implemented and extended
for clarity and experimentation.

---

### Chapter 7: Textures, Materials & Terrain
![Chapter 7 Mountain Solid](outputs/chapter-7/mountain-output-solid.png)
![Chapter 7 Mountain Wireframe](outputs/chapter-7/mountain-output-wireframe.png)
![Chapter 8 Water](outputs/misc/river.gif)

---

### Chapter 8: Lighting
![Chapter 8 PointLight](outputs/chapter-8/point-light.gif)
![Chapter 8 DirectionalLight](outputs/chapter-8/directional-light.gif)
![Chapter 8 SpotLight](outputs/chapter-8/spot-light.gif)

---

### Chapter 9: Texture
![Chapter 9 Main](outputs/chapter-9/output-1.gif)

> ignore the black line on water m using free assets
---

## What’s Inside

- **Chapter-based scenes**  
  Each textbook chapter is implemented as its own selectable scene/module.

- **Exercise answers**  
  All chapters that include programming exercises have their answers implemented.

- **ImGui controls**  
  Runtime tweaking of parameters for learning and experimentation.
---

## Build & Run

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
# Build (Debug or Release both are fine)
cmake --build build --config Release
