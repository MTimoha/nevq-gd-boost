# GD Boost

Windows-only мод для Geometry Dash 2.2081, який налаштовує поведінку гри на рівні системи. Візуально нічого не змінює: частинки, шейдери й ефекти залишаються як є.

## Що робить

| Опція | Що дає |
|---|---|
| Force high-performance GPU | Windows запускає гру на найпотужнішій відеокарті (корисно на ноутах і ПК з інтегрованою + дискретною). Діє з наступного запуску |
| Process priority | Вищий пріоритет процесу гри (Normal / Above normal / High) |
| Disable power throttling | Вимикає Efficiency Mode (EcoQoS), через який гра може потрапляти на слабкі ядра або отримувати нижчі частоти |
| 1 ms timer resolution | Точніші таймери, стабільніший frame pacing |
| Boost render/logic thread | Вищий пріоритет головного потоку + профіль MMCSS "Games" |

## Що НЕ робить (чесно)

- Не переписує рендер і не змінює завантаження рівнів. Це робота рівня Bismuth або AlgebraDash.
- Не змушує GD "використовувати весь процесор". Логіка гри працює в одному потоці, і розпаралелити її без поломки детермінізму не вийде.
- Якщо в тебе лише одна відеокарта, опція GPU нічого не змінить.

## Збірка

Вимоги: Windows, Visual Studio Build Tools (C++), CMake, [Geode CLI](https://docs.geode-sdk.org/getting-started/geode-cli), встановлений Geode SDK.

1. Відкрий `mod.json` і зміни `id` та `developer` (наприклад, `myname.gd-boost`; id тільки з латинських маленьких літер, цифр і `-`).
2. У папці мода виконай:
   ```
   geode project update
   geode build
   ```
   `geode project update` виставляє в `mod.json` версію Geode, що збігається з твоїм SDK (збірка вимагає збігу major/minor). Якщо SDK новіший за `5.11.2`, ця команда виправить поле `geode`.
3. Готовий файл `.geode` з'явиться в `build/`. Скопіюй його в `%LOCALAPPDATA%\GeometryDash\geode\mods\` або встанови через Geode ("Install from file").

Без локальної збірки: запуш папку на GitHub і запусти workflow з `.github/workflows/build.yml`, готовий `.geode` буде в Artifacts.

> Код не проходив компіляцію там, де я його писав (без доступу до Geode SDK). Якщо збірка видасть помилку, скинь її текст, виправимо.
