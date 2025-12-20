# DirectX 12 Frank Luna: Chapters with Answers

This repo is my **DirectX 12 learning playground** built while following the **Frank Luna DirectX 12 textbook**.

The goal is not “just make it work”, but **understand every step**.

So I intentionally:
- **avoid helper-heavy workflows**
- **do not rely on DirectXTK / convenience libraries**
- **repeat code on purpose** across chapters (it’s hella slow)  
  - because rewriting it forces me to think about *each pipeline step* again instead of autopiloting (but helps think about possible design decisions later <3).

On top of the textbook work, I’m building an **ImGui-driven chapter/scene selector** so I can run the project like a mini “chapter browser”, and tweak scene parameters per chapter.

---

## Current Output

### Chapter 4
![Chapter 4 Output](outputs/chapter-4.gif)

> Chapter 4 in the book doesn’t include exercise questions, so there are **no answers** for that chapter — only the working output.

---

## What’s Inside

- **Chapter-based scenes** (each chapter is treated like a selectable scene/module)
- **Exercise answers** (where the book provides them)
- **ImGui controls** for:
    - selecting which chapter scene runs
    - editing scene parameters (I’ll keep expanding controls over time)

---

## Build & Run

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
# Build (Debug or Release both are fine)
cmake --build build --config Release
```
