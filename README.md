# endfield-il2dump

Оффлайн-дампер метаданных IL2CPP для Unity x64 на Windows. Читает
`GameAssembly.dll` и `global-metadata.dat` с диска, игру не запускает,
никаких инжектов и админских прав не требует.

Тестировался на Arknights: Endfield (Unity 2022, metadata v29). Должен
работать на любой Unity-сборке IL2CPP с метаданными версий 24..31, если
они не зашифрованы.

## Сборка

Visual Studio 2022, тулчейн v143, Windows SDK 10.

1. Открыть `endfield-il2dump.sln`
2. Собрать `Release | x64`

На выходе: `build\x64\Release\endfield-il2dump.exe`

## Структура

```
include/
  core/   pe.hpp, metadata.hpp           — PE-парсер и парсер metadata.dat
  io/     il2cpp_binary.hpp, executor.hpp — поиск Code/MetadataRegistration,
                                            рендер C#-имён
  dump/   dumper.hpp, utils.hpp           — формирование dump.cs и JSON
src/      то же зеркально
```

## Запуск

```
endfield-il2dump.exe [опции]
endfield-il2dump.exe <GameAssembly.dll> <global-metadata.dat> [output-dir]
endfield-il2dump.exe --game-dir <путь-к-папке-с-игрой> [-o output-dir]
```

| Флаг | Описание |
| ---- | -------- |
| `-o`, `--output <dir>` | Куда писать дамп (по умолчанию текущая папка) |
| `--game-dir <dir>`     | Найти оба файла внутри `<dir>` автоматически |
| `-h`, `--help`         | Помощь |

Без аргументов ищет `GameAssembly.dll` и
`<game>_Data\il2cpp_data\Metadata\global-metadata.dat` в текущей папке.

### Примеры

```powershell
endfield-il2dump.exe --game-dir "C:\Games\Endfield" -o "C:\dumps\endfield"

endfield-il2dump.exe `
    "C:\Games\Endfield\GameAssembly.dll" `
    "C:\Games\Endfield\Endfield_Data\il2cpp_data\Metadata\global-metadata.dat" `
    "C:\dumps\endfield"
```

## Что на выходе

- `dump.cs` — все сборки в одном файле:
  ```
  // Namespace: System
  public class Object // TypeDefIndex: 38983, Size: 0x10
  {
      // Methods
      // RVA: 0x03D71010 Offset: 0x03D6FA10 VA: 0x0000000183D71010 Slot: 0
      public virtual bool Equals(object obj) { }
      ...
  }
  ```
- `script.json` — список адресов методов с сигнатурами для импорта
  в IDA / Ghidra.
- `stringliteral.json` — все строковые литералы из метаданных.
- `dump.log` — лог запуска.

## Ограничения

- Только Windows x64.
- Зашифрованные `global-metadata.dat` не поддерживаются. Если sanity-поле
  не `0xFAB11BAF`, дампер выдаст ошибку.
- Custom attributes для v29+ не декодируются.

## Лицензия

MIT
