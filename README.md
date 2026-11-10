# VisualHUD — автономный тренировочный HUD

Демонстрационное Windows-приложение с собственным симулятором целей. Не подключается к CS2, не читает память процессов или сетевой трафик и не взаимодействует с VAC.

## Возможности
- Движущиеся тренировочные цели.
- ESP-подобные рамки, подписи и дистанции для объектов симулятора.
- Мини-радар, переключатели ESP / Radar / Names.
- Настройка количества целей и пауза.

## Запуск локально
Требуются Windows 10/11 и Python 3.10+.

```powershell
py -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
py src\main.py
```

## EXE через GitHub Actions
При push в main или ручном запуске workflow собирает `VisualHUD.exe` на Windows и публикует его как Actions artifact (доступен на странице Actions конкретного запуска).

## Сборка локально
```powershell
pip install -r requirements.txt
pyinstaller --noconfirm --clean --onefile --windowed --name VisualHUD src\main.py
```
Результат: `dist\VisualHUD.exe`.

## Ограничения
HUD отображает только синтетические цели собственного симулятора. Он не раскрывает позиции игроков CS2 и не содержит обхода античита.
