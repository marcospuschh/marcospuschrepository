# Aurora Guitar Studio (VST3 / Standalone)

Cadeia de guitarra: Gate -> Pedal (overdrive) -> Amp (2 estágios + tone stack + power amp com sag)
-> Cabine (carrega seus IRs .wav) -> Delay -> Reverb. Distorção com oversampling 4x.

## Compilar
Requisitos: CMake 3.22+, Git e um compilador C++17
(Windows: Visual Studio 2022 | macOS: Xcode | Linux: build-essential + libs do JUCE).

    cd AuroraGuitarStudio
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

O VST3 é copiado para a pasta padrão de plugins ao final. Também gera um app Standalone para testar.

## Gerar o .dmg sem Xcode (GitHub Actions)
1. Crie um repositório no GitHub e envie esta pasta (o arquivo .github/workflows/build.yml vai junto).
2. Abra a aba Actions, aguarde o build "Build plugin" terminar (uns 10 a 15 min).
3. Baixe o artefato AuroraGuitarStudio-mac (contém o .dmg com VST3, AU e app Standalone).

## Instalar no Mac
- VST3: ~/Library/Audio/Plug-Ins/VST3/
- AU (Logic, GarageBand): ~/Library/Audio/Plug-Ins/Components/
- Como não é assinado pela Apple, rode uma vez no Terminal:
  xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Aurora Guitar Studio.vst3"
  (e o mesmo para o .component). Depois reinicie a DAW e refaça o scan de plugins.
