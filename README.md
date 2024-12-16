# Ocean Simulation in OpenGL (9조)

팀원

* 2023-20349 박민준
* 2024-21854 송우석
* 2024-29163 이상엽

## 실행 방법

Windows에서 실행하는 것을 기준으로 작성했습니다.

```powershell
mkdir build
cd build
cmake ..
```

위와 같이 cmake를 실행한 뒤 `build` 폴더 내의 `wave-tool.sln` 솔루션을 통해 visual studio를 실행, `Ctrl + F5`로 컴파일 및 실행한다.

> 테스트한 실행 환경
>
> * Windows 10
> * visual studio 2022
> * cmake 3.31.2
> * CPU: intel 12400 / GPU: RTX 3060

## 개인별 구현 내용

### 2023-20349 박민준

static wave 구현

1. height map을 통한 wave grid 구현
    * `water-grid.tes` (Line 58-67)에서 height map sampling을 통해 wave 구현 (baseline code의 vertex shader 참고)
    * `render-engine.cpp` (Line 672-902)에서 wave에 해당하는 grid 생성. 기존 baseline code를 사용하되 반복되는 계산을 없애고 약간의 최적화 진행.

2. tessellation 구현
   * `water-grid.tcs` 와 `water-grid.tes` 새롭게 구현
   * `render-engine.cpp` (Line 971-976) 에서 camera 각도에 따른 tessellation level 지정

### 2024-21854 송우석

realistic visual effect 구현

1. wave foam effect 구현
   * `water-grid.tes` (Line 69-82, 94-96)에서 wave-vertex position displacement jacobian 계산
   * `water-grid.frag` (Line 83)에서 reflection color를 통해 jacobian에 따른 wave foam 구현
   * `water-grid.frag` (Line -)에서 변수 선언 및 함수 계산 등을 더욱 직관적으로 개선 및 약간의 최적화 진행

2. local reflection 구현
   * `water-grid.frag` (Line 60-81), `render-engine.cpp` (Line 145-170, 225-226, 425, 947-948, 1258-1264), `render-engine.h` (Line 297-300)에서 local reflection framebuffer 및 동작 구현

3. skybox texture 제작
   * `assets\textures\skyboxes\wwwtyro-space-3d\2drp4i9sx0lc-nebulae-2048` 폴더 내의 skybox images를 open sourse skybox images를 base로 edit하여 현 프로젝트에 알맞도록 제작

### 2024-29163 이상엽

dynamic wave 구현

1. dynamic wave 구현
   * `water-grid.tes` (Line 39-50, 105-106) height map sampling하는 위치에 시간에 따라 변화하는 offset을 더함으로써 높낮이 변경
   * `water-grid.tes` (Line 115-116) 시간에따라 전체 수면 높이를 위아래로 움직여 자연스러운 움직임 연출

2. multi-frequency Gerstner wave 구현
   * `render-engine.cpp` (Line 921-936) multi-frequency rendering
   * `program.cpp` (Line 193-218, 609-626) imgui를 통해 여러 종류의 파도를 랜덤하게 생성
   * shared pointer를 통한 메모리 관리
