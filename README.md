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
   * `water-grid.tes` (Line 69-82, 94-96)에서 jacobian 계산
   * `water-grid.frag` (Line 119-124)에서 jacobian에 따른 wave foam 색상 변경

### 2024-29163 이상엽

dynamic wave 구현

