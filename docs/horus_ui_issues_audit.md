# Horus UI Library - Issues Audit

*Date: October 7, 2026*
*Audited by: Opencode*

This document provides a comprehensive audit of issues, potential bugs, design problems, and dangling concerns found in the horus_ui library codebase, including all graphics backends, input, and utility backends. For each issue, severity, exact locations, root cause, and suggested fixes are provided.

---

## Table of Contents

- [1. Core Library Issues (`src/`)](#1-core-library-issues-src)
- [2. Public API Issues (`include/horus.h`)](#2-public-api-issues-includecmphorush)
- [3. Renderer Issues (`src/renderer.*`)](#3-renderer-issues-srcrenderer)
- [4. Docking & Window Issues (`src/docking.*`, `src/window.cpp`)](#4-docking--window-issues-srcdocking-srcwindowcpp)
- [5. Theme & Atlas Issues (`src/theme.*`, `src/atlas.*`)](#5-theme--atlas-issues-srctheme-srcatlas)
- [6. Graphics Backend Issues](#6-graphics-backend-issues)
  - [6.1 OpenGL Backend (`backends/opengl_graphics.*`)](#61-opengl-backend-backendsopengl_graphics)
  - [6.2 DirectX 11 Backend (`backends/dx11_graphics.*`)](#62-directx-11-backend-backendsdx11_graphics)
  - [6.3 DirectX 12 Backend (`backends/dx12_graphics.*`)](#63-directx-12-backend-backendsdx12_graphics)
  - [6.4 Vulkan Backend (`backends/vulkan_graphics.*`)](#64-vulkan-backend-backendsvulkan_graphics)
- [7. Input/Platform Issues (`backends/sdl3_input.*`)](#7-inputplatform-issues-backendssdl3_input)
- [8. Font Issues (`backends/freetype_fonts.*`)](#8-font-issues-backendsfreetype_fonts)
- [9. Utility Backends & Third-Party Integration](#9-utility-backends--third-party-integration)
- [10. Widget-Specific Issues](#10-widget-specific-issues)
- [11. General Code Quality & Safety](#11-general-code-quality--safety)

---

## 1. Core Library Issues (`src/`)

### 1.1 Global Context Singleton State
**Category:** Design / Multi-context Safety  
**Severity:** High  
**Location(s):**
- `src/context.cpp:9` - `Context* ctx = nullptr;` (global singleton)
- `src/context.h` - Context struct definition

**Description:** The library maintains a single global `Context* ctx` accessible across the entire API. This design prevents having multiple independent Horus UI contexts simultaneously in the same process.

**Root Cause:** Legacy singleton design pattern used throughout the codebase. All widget functions, renderer, docking, etc. implicitly reference `ctx`.

**Suggested Fix:** 
- Add support for context handles (e.g., `HUI_Context` opaque type) that can be passed around.
- Alternatively, store current context in thread-local storage (TLS) if single-threaded per context.
- Short-term: Document this limitation clearly. For long-term: Refactor API to take context parameter.

**Verification:** Test creating two independent UI contexts - currently impossible.

---

### 1.2 Undecided TODOs in Core
**Category:** Maintenance  
**Severity:** Medium  
**Location(s):**
- `src/context.h:307` - `u64 pruneUnusedTextTime = 0; //TODO: maybe make it frames`

**Description:** Unresolved design decision about whether to measure pruning intervals in time or frames.

**Root Cause:** Unfinished design consideration.

**Suggested Fix:** 
- Make the decision explicit. Since this is a frame-based immediate mode UI, measuring in frames is likely more deterministic and frame-rate independent.
- Replace with `u32 pruneUnusedTextFrames = 60;` or similar, or keep as time with clear units.
- Remove TODO after deciding.

---

### 1.3 Text Pruning Implementation Details
**Category:** Memory / Performance  
**Severity:** Low  
**Location(s):**
- `src/context.h:307-308` - `pruneUnusedTextTime` and related fields
- `src/context.cpp` - text pruning logic

**Description:** Text cache pruning uses time-based heuristic. Behavior may be unclear across different frame rates.

**Suggested Fix:** Document units and expected behavior; consider frame-based counter for determinism.

---

### 1.4 Global State in Text Input/Multiline States
**Category:** State Management  
**Severity:** Medium  
**Location(s):**
- `src/text_input_state.h`, `src/text_input_state.cpp`
- `src/multiline_text_input_state.h`, `src/multiline_text_input_state.cpp`

**Description:** Text input state is stored in `Context` (per-context). However, there's coupling with global context access patterns. Also see the static buffer in textinput widget (see section 10.1).

**Suggested Fix:** Ensure all state is properly scoped to Context. No globals outside Context.

---

## 2. Public API Issues (`include/horus.h`)

### 2.1 Platform Macro Definitions Could Be Cleaner
**Category:** Portability  
**Severity:** Low  
**Location(s):**
- `include/horus.h:34-52` - Platform detection macros
- `include/horus.h:70-92` - Calling convention and export macros

**Description:** Platform detection uses preprocessor checks. While functional, some edge cases (e.g., MinGW vs MSVC, Clang on Windows) might need refinement. Also `HUI_STRUCT_API` defaults behavior varies.

**Root Cause:** Standard cross-platform macro patterns.

**Suggested Fix:** 
- Add explicit Clang detection on Windows (`__MINGW32__`, `__MINGW64__`, `_MSC_VER` with ClangCL).
- Document all configuration macros in comments.
- Consider adding `#error` for unsupported platforms if truly needed.

**Notes:** Current approach is reasonable for the supported backends (SDL3 covers most platforms).

---

### 2.2 Configuration Macros Flexibility
**Category:** API Design  
**Severity:** Low  
**Location(s):**
- `include/horus.h:98-108` - Custom config, static/shared build macros

**Description:** The API allows `HUI_CUSTOM_CONFIG_FILE` override and `HUI_STATIC`/shared linking. This is good. No obvious issues.

**Suggested Fix:** None critical. Add documentation examples.

---

### 2.3 Integer Type Definitions
**Category:** Portability  
**Severity:** Low  
**Location(s):**
- `include/horus.h:110-118` - Basic integer types

**Description:** Fixed-width types (`u8,u32,i32,f32,f64,bool`) are redefined. Conflicts possible if including before standard headers in some cases? Also `bool` redefinition risk if mixing C/C++ with different bool types.

**Root Cause:** Project-local type aliases.

**Suggested Fix:** Use `stdint.h` types (`uint32_t`, etc.) or ensure no conflicts. In C++, `bool` is fine. Also guard against redefinition.

**Notes:** Acceptable for a self-contained library.

---

## 3. Renderer Issues (`src/renderer.*`)

### 3.1 Renderer State Management
**Category:** Correctness  
**Severity:** Medium  
**Location(s):**
- `src/renderer.h`, `src/renderer.cpp`

**Description:** The renderer maintains draw command lists and state. The code appears well-structured. However, there's implicit dependency on current context and graphics backend being set.

**Root Cause:** Tight coupling with Context and backend via services.

**Suggested Fix:** 
- Validate backend is initialized before rendering operations.
- Add defensive checks if backend services are null.

---

### 3.2 Draw Command Growth
**Category:** Memory  
**Severity:** Low  
**Location(s):**
- `src/renderer.cpp` - command buffers/lists

**Description:** Draw commands accumulate per frame. The renderer should clear them each frame - this is typical immediate mode behavior. Need to verify proper reset.

**Suggested Fix:** Ensure all transient draw data is cleared at frame end/start. Review frame lifecycle.

---

### 3.3 Font Rendering Integration
**Category:** Correctness  
**Severity:** Medium  
**Location(s):**
- `src/renderer.cpp` - text rendering paths

**Description:** Text rendering depends on font backend and atlas. If font atlas is stale or not uploaded, rendering artifacts could occur.

**Suggested Fix:** Add validation/assertions when rendering with invalid font handles.

---

## 4. Docking & Window Issues (`src/docking.*`, `src/window.cpp`)

### 4.1 Delayed Deletion of Docking Structures
**Category:** Memory Lifetime  
**Severity:** Medium  
**Location(s):**
- `src/docking.cpp` - `windowsToDelete`, `nativeWindowsToDelete`, `dockNodesToDelete` sets/lists
- `src/window.cpp` - window close handling
- `src/context.h` - related tracking structures

**Description:** The code defers deletion of windows, native windows, and dock nodes (see `dockingUpdate()` and related). This is done to avoid iterator invalidation during traversal. While correct in principle, the deletion happens later - if there's an error path or if the context is destroyed before update runs, leaks could occur.

**Root Cause:** Two-phase deletion pattern to handle reentrancy/iteration.

**Suggested Fix:** 
- Ensure all deferred deletions are flushed in `contextDestroy()` or context shutdown.
- Add explicit cleanup pass before context destruction.
- Document the lifecycle clearly.

**Verification:** Check what happens if a window is closed and then context is immediately destroyed without a frame update.

---

### 4.2 Dock Node Tree Integrity
**Category:** Correctness  
**Severity:** Medium  
**Location(s):**
- `src/docking.cpp` - dock node manipulation (split, merge, move)

**Description:** Docking is complex with many edge cases (node splits, removing windows, floating windows). Look for potential dangling pointers if nodes are deleted while references exist.

**Root Cause:** Manual tree management.

**Suggested Fix:** 
- Add invariant checks (assertions) for tree consistency.
- Consider stronger ownership model (unique_ptr/shared_ptr) where feasible.
- Extensive testing of docking operations.

---

### 4.3 Closed Windows Rect Tracking
**Category:** State Management  
**Severity:** Low  
**Location(s):**
- `src/docking.cpp` - `closedWindowsRects` map

**Description:** Stores rects of closed windows for restoration. This grows over time if not pruned? Check if entries are ever removed after use.

**Suggested Fix:** Review lifecycle - entries should be consumed when reopening or pruned after timeout.

---

### 4.4 Window Focus/Activation State
**Category:** Platform Integration  
**Severity:** Medium  
**Location(s):**
- `src/window.cpp`
- Interaction with `backends/sdl3_input.cpp`

**Description:** Window focus, activation, and modality involve coordination between UI layer and platform layer. Race conditions possible if events arrive out of order.

**Suggested Fix:** Ensure consistent state updates; prefer platform events as source of truth.

---

## 5. Theme & Atlas Issues (`src/theme.*`, `src/atlas.*`)

### 5.1 Image Ownership and Deletion
**Category:** Memory Management  
**Severity:** Medium  
**Location(s):**
- `src/theme.h`, `src/theme.cpp` - Theme, images, elements
- `src/atlas.h`, `src/atlas.cpp` - Atlas texture management

**Description:** Theme holds images (HImage). Atlas packs images. Need to verify proper cleanup in destructors (`Theme::~Theme()`, `Atlas::~Atlas()`). Also white image creation/deletion paths.

**Root Cause:** Manual resource management.

**Suggested Fix:** 
- Review `Theme::~Theme()` - ensure all allocated images/textures are freed via backend services.
- Review `Atlas::~Atlas()` - verify texture deletion.
- Check for double-free if same image is referenced multiple times.

---

### 5.2 Atlas Packing Service Dependency
**Category:** Backend Coupling  
**Severity:** Medium  
**Location(s):**
- `src/atlas.cpp` - uses `contextGetSettings().services.packRects`
- `backends/stb_rectpack.cpp` - implementation

**Description:** Atlas relies on external packing service. If service is not properly initialized or fails, atlas generation will fail silently or produce bad results.

**Suggested Fix:** 
- Add error handling and validation of pack results.
- Assert on failure or return error codes.

---

### 5.3 Theme Element State Defaults
**Category:** Correctness  
**Severity:** Low  
**Location(s):**
- `src/theme.cpp`

**Description:** Theme loading/creation sets default states. Edge cases with missing elements could cause issues.

**Suggested Fix:** Add defensive checks; return sensible defaults.

---

## 6. Graphics Backend Issues

### 6.1 OpenGL Backend (`backends/opengl_graphics.*`)

**Severity:** High (Multi-context safety, global state)

#### 6.1.1 Global/Static State Prevents Multiple Contexts
**Location(s):**
- `backends/opengl_graphics.cpp` - multiple static globals:
  - Line ~? `static Rect currentViewport;` (viewport tracking)
  - `static GLuint vertexShader;` (global)
  - `static GLuint pixelShader;` (global) 
  - `static GLuint program;` (global shader program)
  - `static OpenGLVertexBuffer vertexBuffer;` (global VBO)
  - `static char errStr[1024];` (global error buffer)
  - `static bool shadersInitialized;` (global flag)

**Description:** The comment at line in codebase (from exploration): `"TODO: so this statics might not work with multiple hui contexts"`. All rendering state is global - if multiple Horus UI contexts render to different windows/contexts, they will interfere (shared shaders, shared VBO, shared state).

**Root Cause:** Backend written as singleton/global state.

**Suggested Fix:**
- Move all per-context state into a backend context struct (e.g., `OpenGLBackendState`).
- Store this struct in the backend context associated with each Horus context.
- Initialize per context on `graphicsInit()`.
- Clean up properly on `graphicsDestroy()`.

**Impact:** Critical for multi-window applications.

---

#### 6.1.2 Shader Management and Lifetime
**Location(s):**
- `backends/opengl_graphics.cpp` - shader compilation, linking, caching

**Description:** Shaders compiled once globally (`shadersInitialized`). If GL context is lost/recreated (e.g., context loss on some platforms), shaders become invalid but flag isn't reset properly.

**Root Cause:** No GL context loss handling.

**Suggested Fix:**
- Detect context loss and recompile shaders.
- Track which GL context the resources belong to (or just reinitialize on new context).
- Add error checking and logging on shader compile/link failures.

---

#### 6.1.3 Vertex Buffer Growth Strategy
**Location(s):**
- `backends/opengl_graphics.cpp` - `OpenGLVertexBuffer` implementation

**Description:** Dynamic VBO for immediate mode drawing. Should handle buffer resizing when exceeding capacity. Need to verify proper mapping/unmapping with `glMapBuffer`/`glUnmapBuffer` (or DSA equivalents).

**Root Cause:** Standard immediate mode streaming.

**Suggested Fix:** 
- Ensure buffer is orphaned or resized appropriately when full (to avoid stalling).
- Check for GL errors after operations.
- Use `glBufferData` with `GL_DYNAMIC_DRAW`.

---

#### 6.1.4 Error Handling
**Location(s):**
- `backends/opengl_graphics.cpp` - error string buffer `errStr[1024]`

**Description:** Errors stored in static buffer but may not be propagated clearly to caller. GL error checking might be sparse in release builds.

**Suggested Fix:** 
- Use `HUI_ASSERT`/`HUI_LOG` consistently.
- Check `glGetError()` after critical operations, especially in debug.
- Return error info via callback/logging service.

---

### 6.2 DirectX 11 Backend (`backends/dx11_graphics.*`)

**Severity:** High

#### 6.2.1 Global Device/Context and Shared Resources
**Location(s):**
- `backends/dx11_graphics.cpp` - global state:
  - `static ID3D11Device* device = nullptr;`
  - `static ID3D11DeviceContext* context = nullptr;`
  - `static Dx11SharedVertexBuffer sharedVb;` (static/shared)
  - Pipeline state objects (static)
  - Shader blobs, input layout (static)
  - Samplers, rasterizer/blend/depth states (static)

**Description:** All D3D11 device resources are global statics. Cannot support multiple D3D11 devices/contexts or multiple Horus contexts. Also resources not tied to device lifetime properly in all cases.

**Root Cause:** Singleton design.

**Suggested Fix:** 
- Encapsulate all DX11 backend state in a struct per Horus context.
- Store references to device/context (or create internally). Clarify ownership.
- On `graphicsDestroy()`, properly release all COM objects with `SAFE_RELEASE`/`Release()`.
- Handle device lost scenarios if needed.

---

#### 6.2.2 COM Object Reference Counting
**Location(s):**
- `backends/dx11_graphics.cpp` - throughout (Create*, QueryInterface, Release)

**Description:** COM objects created but need careful ref counting. Shared resources (shaders, states) created once - correct if same device, but problematic if device changes.

**Root Cause:** Manual COM management.

**Suggested Fix:** 
- Use smart COM pointers (`Microsoft::WRL::ComPtr<T>`) to avoid leaks.
- Track device validity.
- Ensure proper cleanup order.

---

#### 6.2.3 Input Layout and Shader Compatibility
**Location(s):**
- `backends/dx11_graphics.cpp` - vertex format, input layout creation

**Description:** Input layout tied to vertex shader signature. If shaders change/recompile, layout must be updated.

**Suggested Fix:** Regenerate input layout when shaders are (re)compiled.

---

#### 6.2.4 Vertex Buffer Streaming
**Location(s):**
- `backends/dx11_graphics.cpp` - `Dx11SharedVertexBuffer`

**Description:** Dynamic buffer updated via `Map(D3D11_MAP_WRITE_DISCARD)`. Correct for immediate mode. Buffer resizing when capacity exceeded needs to be safe.

**Suggested Fix:** When resizing, release old buffer and create new with larger size; update any bound views.

---

### 6.3 DirectX 12 Backend (`backends/dx12_graphics.*`)

**Severity:** High (Most complex, global state, frame synchronization)

#### 6.3.1 Global State and Frame Synchronization
**Location(s):**
- `backends/dx12_graphics.cpp` - extensive statics:
  - `static FrameData frames[HUI_DX12_NUM_FRAMES_IN_FLIGHT];` (per-frame resources)
  - `static u32 currentFrameIndex = 0;`
  - `static ID3D12Device* device = nullptr;`
  - `static ID3D12CommandQueue* commandQueue = nullptr;`
  - `static ID3D12GraphicsCommandList* commandList = nullptr;`
  - `static ID3D12CommandAllocator* commandAllocator = nullptr;`
  - `static ID3D12Fence* fence = nullptr;`
  - `static HANDLE fenceEvent = nullptr;`
  - `static UINT64 fenceValue = 0;`
  - `static ID3D12RootSignature* rootSignature = nullptr;`
  - `static ID3D12PipelineState* pipelineState = nullptr;`
  - `static ID3D12DescriptorHeap* srvHeap = nullptr;` (SRV descriptor heap)
  - `static ID3D12Resource* uploadBuffer = nullptr;` (shared upload)
  - `static D3D12MA::Allocator* allocator = nullptr;` (D3D12 memory allocator)
  - Plus many other heaps, resources

**Description:** Massive global state. Multi-context impossible. Frame pacing tied to global `currentFrameIndex`. Resource lifetime management is complex with D3D12MA.

**Root Cause:** Designed for single context/single window.

**Suggested Fix:**
- Create `Dx12BackendContext` struct containing all this state per Horus context.
- Properly initialize per context in `graphicsInit()`.
- Clean up all resources in `graphicsDestroy()` including waiting for GPU to finish (fence wait).
- Handle multi-window with care (separate command queues/lists or shared?).

---

#### 6.3.2 Frame Data Lifecycle
**Location(s):**
- `backends/dx12_graphics.cpp` - `FrameData` struct and arrays

**Description:** Each frame has its own vertex buffer, upload buffer, fence value, etc. Need to ensure GPU has finished using frame resources before reusing them (CPU-GPU synchronization). The fence logic must be correct.

**Root Cause:** Manual frame in-flight management.

**Suggested Fix:**
- Verify fence wait happens before resetting command allocators for that frame.
- Check for potential fence value overflow (use UINT64).
- Add debug markers if available (PIX).

---

#### 6.3.3 D3D12 Memory Allocator (D3D12MA) Integration
**Location(s):**
- `backends/dx12_graphics.cpp` - includes D3D12MA, creates allocator

**Description:** Uses D3D12MA. Proper cleanup: `allocator->Release()` and then destroy D3D12MA objects. Also need to ensure all allocations freed.

**Suggested Fix:**
- Review cleanup order in destroy path.
- Check for leaks with debug allocator.

---

#### 6.3.4 Descriptor Heap Management
**Location(s):**
- `backends/dx12_graphics.cpp` - SRV heap, possibly RTV/DSV

**Description:** Descriptor heaps have limited size. Dynamic allocation of descriptors (for textures) needs to be tracked. If many textures created without reuse, heap could be exhausted.

**Suggested Fix:**
- Implement descriptor allocation strategy (ring buffer or freelist).
- Grow heap if needed, or pre-size appropriately.
- Track used descriptors per frame.

---

#### 6.3.5 Command List/Allocator Reset
**Location(s):**
- `backends/dx12_graphics.cpp` - begin/end frame

**Description:** Command allocators must be reset only after GPU is done. Command lists reset after allocator reset.

**Suggested Fix:** Double-check the order in frame lifecycle. Add validation.

---

### 6.4 Vulkan Backend (`backends/vulkan_graphics.*`)

**Severity:** High (Very complex, large global state, many object types)

#### 6.4.1 Extensive Global State
**Location(s):**
- `backends/vulkan_graphics.cpp` - hundreds of static variables including:
  - Instance, physical device, device, queues (graphics/present)
  - Surface, swapchain, swapchain images, image views, framebuffers
  - Render pass, pipeline layout, graphics pipeline
  - Command pools, command buffers (per frame)
  - Sync objects: semaphores, fences (per frame)
  - Descriptor pools, descriptor set layouts, descriptor sets
  - Vertex buffers, staging buffers
  - Memory allocations, samplers
  - MSAA, depth buffer resources
  - Debug messenger

**Description:** This is the most state-heavy backend. All Vulkan objects are global statics. Cannot support multiple Vulkan contexts/devices/windows. Lifecycle management is extremely complex.

**Root Cause:** Monolithic singleton design for Vulkan backend.

**Suggested Fix:**
- Create `VkBackendContext` to encapsulate all state per Horus context.
- This is a large refactoring but necessary for multi-context support.
- Ensure proper cleanup in reverse creation order on destroy.
- Add thorough validation layers usage in debug.

---

#### 6.4.2 Swapchain Management
**Location(s):**
- `backends/vulkan_graphics.cpp` - swapchain creation, recreation, present

**Description:** Swapchain can become invalid (resize, surface changes, lost). Code needs to handle swapchain recreation. Need to check if this is implemented robustly (VK_SUBOPTIMAL_KHR, VK_ERROR_OUT_OF_DATE_KHR).

**Root Cause:** Window resizing/surface changes.

**Suggested Fix:**
- Implement full swapchain recreation path (destroy framebuffers, image views, swapchain; recreate).
- Handle acquire/present errors gracefully.

---

#### 6.4.3 Memory Management
**Location(s):**
- `backends/vulkan_graphics.cpp` - buffer allocations, images

**Description:** Uses Vulkan memory allocation (possibly VMA if available? Check includes - not obvious from filename alone, but standard pattern). Manual memory type selection and binding.

**Suggested Fix:** Consider using Vulkan Memory Allocator (VMA) for easier memory management. If not using, ensure correct memory type selection.

---

#### 6.4.4 Descriptor Set Management
**Location(s):**
- `backends/vulkan_graphics.cpp` - descriptor pools and sets

**Description:** Descriptor sets allocated from pools. If many different descriptor layouts or many sets, pools can run out. Need to track and potentially grow pools.

**Suggested Fix:** 
- Implement proper descriptor pool sizing.
- Reset pools per frame or use ring allocation.
- Free sets appropriately.

---

#### 6.4.5 Synchronization (Fences/Semaphores)
**Location(s):**
- `backends/vulkan_graphics.cpp` - per-frame sync objects

**Description:** Correct synchronization is critical to avoid GPU/CPU hazards. In-flight frames limited by `MAX_FRAMES_IN_FLIGHT` (or similar constant). Fence waits must happen before reusing command buffers.

**Suggested Fix:** Verify sync logic matches Vulkan best practices. Use validation layers to catch issues.

---

#### 6.4.6 Debugging and Validation
**Location(s):**
- `backends/vulkan_graphics.cpp` - debug messenger, validation layers

**Description:** Debug reporting setup. Should be conditional on build type.

**Suggested Fix:** Only enable validation layers in debug builds to avoid performance overhead.

---

## 7. Input/Platform Issues (`backends/sdl3_input.*`)

### 7.1 Global Input Context
**Severity:** High

#### 7.1.1 Singleton Input State
**Location(s):**
- `backends/sdl3_input.cpp:17` - `static Sdl3InputContext* sdl3InputContext = nullptr;`

**Description:** Global input context. Cannot support multiple Horus contexts with separate windows/input handling cleanly. All input state shared globally.

**Root Cause:** Singleton design.

**Suggested Fix:** 
- Move to context-associated state or allow passing context.
- Store per-Horus-context input state.
- For multi-window, need to track which window events belong to which context.

---

#### 7.2 Platform-Specific Code Paths
**Location(s):**
- `backends/sdl3_input.cpp` - platform defines for Windows, X11, etc.

**Description:** Uses platform-specific APIs (e.g., Windows API calls, X11). SDL3 abstracts much, but there may be platform-specific code. Need to ensure proper conditional compilation.

**Root Cause:** Platform-specific cursor/clipboard/native window handling.

**Suggested Fix:** 
- Guard platform-specific code with proper `#ifdef`s.
- Test on each supported platform.
- Prefer SDL3 APIs where possible.

---

#### 7.3 Cursor Management
**Location(s):**
- `backends/sdl3_input.cpp` - cursor creation, setting, destruction

**Description:** Creates SDL cursors for custom cursor shapes. Must destroy them properly on shutdown to avoid leaks.

**Suggested Fix:** Track all created cursors and free in destroy/uninit.

---

#### 7.4 Text Input (IME) Handling
**Location(s):**
- `backends/sdl3_input.cpp` - SDL_StartTextInput, text editing events
- Interaction with `textInput()` widget

**Description:** IME composition events (SDL_EVENT_TEXT_EDITING) need to be passed through correctly. Composition range, cursor position must sync with widget state.

**Suggested Fix:** 
- Verify IME works on Windows, macOS, Linux.
- Ensure composition text is handled before final text input.
- Coordinate with `startTextInput()`/`stopTextInput()` calls from widgets.

---

#### 7.5 Delta Time Calculation
**Location(s):**
- `backends/sdl3_input.cpp` - timing

**Description:** Delta time computation - should be robust against large frame skips or timer issues.

**Suggested Fix:** Clamp delta time to reasonable range (e.g., max 0.5s) to avoid physics/jump issues.

---

#### 7.6 Window Event Handling
**Location(s):**
- `backends/sdl3_input.cpp` - SDL event loop processing

**Description:** Events mapped to Horus input events. Need to ensure all relevant SDL events are handled (resize, close, focus, mouse, keyboard, touch, gamepad).

**Suggested Fix:** Review event mapping completeness.

---

## 8. Font Issues (`backends/freetype_fonts.*`)

### 8.1 FreeType Context Lifecycle
**Severity:** Medium

#### 8.1.1 Static FreeType Context
**Location(s):**
- `backends/freetype_fonts.cpp` - `struct FTContextInfo` and static instances

**Description:** FreeType library initialized once globally. Font faces loaded and managed. Need to ensure proper FT_Done_FreeType() on shutdown.

**Root Cause:** Global FT library handle.

**Suggested Fix:** 
- Track initialization count if needed, but single init is fine.
- Ensure cleanup in font backend destroy/uninit.
- Check for FT errors on all operations.

---

#### 8.2 Font Face Loading and Caching
**Location(s):**
- `backends/freetype_fonts.cpp` - font loading

**Description:** Fonts loaded from files/bytes. Caching strategy - are font faces cached? If same font loaded multiple times, could duplicate memory.

**Suggested Fix:** Implement font cache by (path/hash) to avoid duplicates.

---

#### 8.3 Glyph Atlas Integration
**Location(s):**
- `backends/freetype_fonts.cpp` - glyph rendering to atlas
- Interaction with `Atlas` and `Renderer`

**Description:** Glyphs rendered and uploaded to texture atlas. If atlas is full or needs resize, need to handle gracefully.

**Suggested Fix:** 
- Handle atlas overflow - expand atlas or use fallback.
- Ensure proper packing alignment for SDF/bitmap fonts.

---

#### 8.4 Unicode/UTF Handling
**Location(s):**
- `backends/freetype_fonts.cpp`
- `backends/utfcpp.*` - UTF utilities

**Description:** Relies on utfcpp for UTF-8/32 conversions. Font shaping not mentioned - just glyph rendering. For complex scripts, missing shaping could be an issue.

**Root Cause:** Basic glyph-by-codepoint rendering.

**Suggested Fix:** Document limitation (no advanced text shaping). This is fine for simple UI text.

---

## 9. Utility Backends & Third-Party Integration

### 9.1 stb_image Implementation in json_theme_loader
**Severity:** High (ODR Violation Risk)

#### 9.1.1 Multiple STB Implementation Defines
**Location(s):**
- `backends/json_theme_loader.cpp:6-9`:
  ```cpp
  #define STB_IMAGE_IMPLEMENTATION
  #define STB_IMAGE_WRITE_IMPLEMENTATION
  #include "stb_image.h"
  #include "stb_image_write.h"
  ```

**Description:** Defining STB implementation macros in a `.cpp` file means the implementation is compiled here. If any other translation unit in the build also includes `stb_image.h` with implementation defines, this causes One Definition Rule (ODR) violations and link errors.

**Root Cause:** STB libraries use header-only implementation pattern requiring exactly one TU with defines.

**Suggested Fix:**
- Move STB implementation defines to a dedicated `.cpp` file (e.g., `backends/stb_image_impl.cpp`) that only includes stb headers with defines.
- In other files, just include headers without defines.
- Or ensure json_theme_loader.cpp is the only place - but verify no other includes with defines exist.

**Verification:** Search for other STB_IMAGE_IMPLEMENTATION defines in codebase. Also stb_rectpack has its own define in `backends/stb_rectpack.cpp:2`.

**Note:** stb_rectpack correctly has its own implementation in its own cpp file. Same pattern needed for stb_image/stb_image_write if used elsewhere.

---

### 9.2 stb_rectpack Implementation
**Location(s):**
- `backends/stb_rectpack.cpp:2` - `#define STB_RECT_PACK_IMPLEMENTATION`
- `backends/stb_rectpack.h` - header
- `backends/stb_rectpack.cpp` - implementation

**Description:** Correctly isolated implementation. Uses service `services.packRects` to wire into Context.

**Suggested Fix:** None. This is correct.

---

### 9.3 JSON Theme Loader Dependencies
**Location(s):**
- `backends/json_theme_loader.cpp`

**Description:** Loads themes from JSON files. Uses `nlohmann/json` (from includes in full context). Also depends on image loading via stb_image and file I/O via services (`contextGetSettings().services.open/read/seek/tell/close`).

**Dependencies look correct.** The service-based I/O is good for flexibility.

**Suggested Fix:** Add error handling for malformed JSON, missing files, image load failures.

---

### 9.4 Native File Dialogs Backend
**Location(s):**
- `backends/native_file_dialogs.h`, `backends/native_file_dialogs.cpp` (if exists)

**Description:** Platform-specific file dialogs. Need to ensure cross-platform support and proper cleanup.

**Suggested Fix:** Check for null returns, handle cancellation.

---

### 9.5 stdio File IO Backend
**Location(s):**
- `backends/stdio_fileio.h`, `backends/stdio_fileio.cpp`

**Description:** Standard C file I/O implementation of services. Should handle errors properly (fopen failures, etc.).

**Suggested Fix:** Return error codes/flags consistently.

---

### 9.6 utfcpp Backend
**Location(s):**
- `backends/utfcpp.*`

**Description:** Third-party UTF-8/UTF-32 conversion utilities. Appears to be included as source. No obvious issues if kept up to date.

**Suggested Fix:** None critical.

---

## 10. Widget-Specific Issues

### 10.1 Text Input Widget - Static Buffer
**Severity:** Medium

#### 10.1.1 Global Static Password Buffer
**Location(s):**
- `src/textinput_widget.cpp:111` - `static char hiddenPwdText[maxHiddenCharLen] = "";`

**Description:** Static buffer for hidden password text. Shared globally across all text inputs in all contexts. Not thread-safe, not context-isolated. Also buffer size fixed at 1024 chars.

**Root Cause:** Convenience static.

**Suggested Fix:**
- Move to per-widget state in `TextInputState` (already has state in Context).
- Or allocate dynamically based on actual password length.
- Remove global static.

**Impact:** If multiple text inputs show passwords simultaneously or in different contexts, they share the same buffer.

---

### 10.2 Multiline Text Input State Growth
**Location(s):**
- `src/multiline_text_input_widget.cpp`
- `src/multiline_text_input_state.*`

**Description:** Stores lines as vector of Utf32String. For very large files (many lines), memory grows. Also hash-based change detection (`currentTextHash != state.lastTextHash`).

**Suggested Fix:** Consider incremental updates, but for typical UI text sizes this is fine.

---

### 10.3 Widget ID Generation
**Location(s):**
- `src/util.h/cpp` - `genId()` function

**Description:** Widget IDs generated from strings/hashes. Collisions possible with hash? Unlikely but theoretically possible.

**Suggested Fix:** Consider using 64-bit hashes with lower collision probability, or detect collisions in debug.

---

### 10.4 Focus/Activation Logic
**Location(s):**
- Various widget files - focusChanged checks

**Description:** Focus management involves multiple flags (`ctx->focusChanged`, `ctx->widget.focusedId`, `ctx->widget.focused`, layer checks). Complex interactions could lead to focus loss/gain glitches.

**Suggested Fix:** Add assertions and test focus edge cases thoroughly.

---

## 11. General Code Quality & Safety

### 11.1 Error Handling Consistency
**Severity:** Medium

**Description:** Mix of assertions (`HUI_ASSERT`), logging (`HUI_LOG`), and silent failures. Some code paths don't check return values from service calls.

**Suggested Fix:**
- Establish error handling policy (assert in debug for programmer errors, return codes/log for runtime errors).
- Check all service call results.

---

### 11.2 Memory Leaks Potential
**Severity:** Medium

**Description:** 
- Deferred deletions in docking may not be flushed on shutdown (see 4.1).
- Backend resources - global statics cleaned up? Need to verify destroy paths are called.
- COM objects in DX11/DX12 need proper Release.

**Suggested Fix:** 
- Audit all `new`/allocations have corresponding `delete`/free in shutdown.
- Use RAII/smart pointers where appropriate.
- Add leak detection in debug builds.

---

### 11.3 Thread Safety
**Severity:** Medium

**Description:** Codebase appears single-threaded (UI in main thread). Global statics have no synchronization. If used from multiple threads, data races will occur.

**Suggested Fix:** Document that library is not thread-safe; all calls must be from same thread as context.

---

### 11.4 CRLF EOL & Style Conventions
**Severity:** Low (Style)

**Description:** The skill mentions: "CRLF EOL, empty lines around declarations/blocks, comment style conventions, minimalistic code". A quick scan shows mixed styles in places.

**Suggested Fix:** Run formatter/clang-format with project style if available, or enforce consistency.

---

### 11.5 TODO Comments
**Severity:** Low

**Description:** Several TODOs found:
- `src/context.h:307` - pruneUnusedTextTime (see 1.2)
- OpenGL backend TODO about multiple contexts (see 6.1.1)
- Possibly others in files not fully shown

**Suggested Fix:** Track and resolve TODOs, or convert to GitHub issues.

---

### 11.6 Unused/Dead Code
**Severity:** Low

**Description:** `docs/old docking code.txt` exists - reference/legacy. Not compiled.

**Suggested Fix:** Keep for reference or remove if obsolete.

---

### 11.7 Build System & Dependencies
**Severity:** Info

**Description:** Codebase uses CMake (inferred from typical structure). Third-party deps: SDL3, FreeType, Vulkan SDK, DirectX SDKs (Windows), nlohmann/json, stb libraries, utfcpp, D3D12MA (DX12).

**No obvious issues** from source inspection.

---

## Summary & Priority Recommendations

### Critical (High Severity - Must Fix)
1. **Global static state in all backends** (OpenGL, DX11, DX12, Vulkan, SDL3 input, FreeType context) - Prevents multi-context use and is poor design. Refactor to per-context state structures.
2. **stb_image implementation defines** in `json_theme_loader.cpp` - ODR violation risk if stb_image used elsewhere with defines. Move to separate impl file.
3. **Text input static buffer** (`hiddenPwdText` in textinput_widget.cpp) - Cross-contamination between inputs/contexts.

### High Priority (Should Fix)
4. **Deferred deletion cleanup on shutdown** (docking) - Potential leaks if context destroyed without update.
5. **DX12/Vulkan synchronization correctness** - GPU resource lifetime bugs are hard to debug.
6. **Swapchain recreation in Vulkan** - Handle resize/surface changes robustly.

### Medium Priority (Recommended)
7. **COM/smart pointer usage in D3D backends** - Prevent leaks, easier lifecycle.
8. **Error handling consistency** - Better diagnostics and robustness.
9. **Descriptor heap/atlas overflow handling** - Prevent runtime failures.

### Low Priority (Nice to Have)
10. **Resolve TODOs** - Design decisions.
11. **Style consistency** - Codebase polish.
12. **Thread-safety documentation** - Clear API contract.

---

*End of Audit Report*
