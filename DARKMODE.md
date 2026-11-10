# VeraCrypt 1.26.31 – Dark Mode (Modversion 1.0.0)

Dieser Stand von VeraCrypt hat für die Windows-Oberfläche einen **Dark Mode, der standardmäßig aktiv ist**.
An der Verschlüsselung (Algorithmen, Header, Schlüsselableitung, Treiber) wurde **nichts** geändert.

## Ergebnis des Builds

GitHub Actions erzeugt den **kompletten Original-Installer**:

`VeraCrypt Setup 1.26.31-DarkMode.exe`

Er funktioniert wie der offizielle Installer: Sprache wählen, Lizenz, **Installieren** oder
**Entpacken** (portable Nutzung), Installationsordner, Optionen, Treiberinstallation,
Startmenü-/Desktop-Verknüpfungen, Eintrag in „Apps & Features“ inkl. Deinstallation.
Eine vorherige Installation des Originals ist **nicht** nötig.

Enthalten sind (wie im Original):
- `VeraCrypt.exe`, `VeraCrypt Format.exe`, `VeraCryptExpander.exe` – für x64 und ARM64, mit Dark Mode
- Treiber `veracrypt-x64.sys` / `veracrypt-arm64.sys` mit `.cat` und `.inf` – die von Microsoft signierten
  Originaldateien aus dem Repository (`src/Release/Setup Files`), unverändert
- Sprachdateien (`Languages.zip`) und Dokumentation (`docs.zip`)

## Was ist dunkel?

- Alle Dialoge von VeraCrypt, VeraCrypt Format und VeraCrypt Expander
- Titelleiste, Menüleiste, Kontext- und Tray-Menüs
- Eingabefelder, Auswahllisten, Laufwerksliste inkl. Spaltenköpfe, Tabs (Einstellungen), Textfenster
- Checkboxen, Optionsfelder und Gruppenrahmen werden im klassischen Windows-Stil gezeichnet,
  damit ihre Beschriftung hell dargestellt werden kann.

Nicht dunkel: der Installer selbst (bleibt wie das Original), Windows-Standarddialoge
(MessageBox, Datei öffnen/speichern) und Fortschrittsbalken. Bei aktivem Windows-Kontrastmodus
bleibt der Dark Mode automatisch aus.

## Ein- und Ausschalten

- In VeraCrypt: **Einstellungen → Dunkler Modus** (wirkt nach einem Neustart des Programms)
- Oder per Registry: `HKCU\Software\VeraCrypt`, Wert `DarkMode` (DWORD) – `0` = aus, `1` = an.
  Ohne diesen Wert ist der Dark Mode an.

## Bauen mit GitHub Actions

1. Dieses Verzeichnis als Repository zu GitHub hochladen.
2. **Actions → „Windows Installer (Dark Mode)“** läuft bei jedem Push automatisch
   (oder manuell über „Run workflow“).
3. Ergebnis: Artefakt `VeraCrypt-Setup-1.26.31-DarkMode-1.0.0` mit dem Installer und `SHA256SUMS.txt`.
4. Wird ein Tag `v…` gepusht (z. B. `v1.0.0`), entsteht zusätzlich ein GitHub-Release.

Ablauf im Workflow (entspricht `src/Signing/sign.bat` des Originals, nur ohne Code-Signatur):
x64- und ARM64-Programme bauen → `VeraCrypt Setup.exe` (Win32) bauen → `Languages.zip` und
`docs.zip` erzeugen → `VeraCrypt Setup.exe /pq` verpackt alles zum fertigen Installer.
`/pq` ist die Original-Option `/p`, nur ohne Meldungsfenster (Protokoll in `VeraCryptPackage.log`).

## Hinweise

- Installer und Programme sind nicht mit dem IDRIX-Zertifikat signiert. Die Selbstprüfung der
  eigenen Programmdateien ist deshalb in diesem Build abgeschaltet
  (`VC_UNSIGNED_CUSTOM_BUILD` in `src/Common/Tcdefs.h`). Die CRC-Prüfung des Installationspakets
  und die Signaturprüfung der Treiberdateien bleiben aktiv.
- Windows SmartScreen warnt beim ersten Start des Installers („Unbekannter Herausgeber“).
- Die Versionsanzeige lautet `1.26.31-DarkMode`.
- Das MSI-Paket und „VeraCrypt Portable.exe“ werden nicht gebaut; für die portable Nutzung im
  Installer „Entpacken“ wählen.

## Geänderte Dateien

| Datei | Änderung |
|---|---|
| `src/Common/DarkMode.h` | neu – gesamte Dark-Mode-Logik |
| `src/Common/Dlgcode.c` | Dark Mode beim Start initialisieren und auf Dialoge anwenden; Selbstprüfung für unsignierten Build |
| `src/Common/Dlgcode.h` | Funktionsdeklarationen |
| `src/Common/Tcdefs.h` | Versionszusatz `-DarkMode`, Schalter `VC_UNSIGNED_CUSTOM_BUILD` |
| `src/Mount/Mount.c`, `Mount.rc`, `Resource.h` | Menüpunkt „Dunkler Modus“ |
| `src/Setup/Setup.c`, `src/Setup/SelfExtract.c` | Option `/pq` (Verpacken ohne Meldungsfenster für den automatischen Build) |
| `src/Common/Language.xml`, `Translations/Language.de.xml` | Texte für den Menüpunkt |
| `.github/workflows/build-windows-darkmode.yml` | Windows-Build des kompletten Installers |
