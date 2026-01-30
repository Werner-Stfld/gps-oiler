# GPS-Oiler 3.0 #

Zur Version 3

## WEB-Server ##

Der Webinhalt nutzt vueJs wie hier beschrieben. [How to use VueJs with ESP8266 or ESP32](https://www.luisllamas.es/en/esp8266-vuejs/)
Die Dateien des Web-Servers sind als "LittleFs" Dateisystem auf dem Target hinterlegt.

Das Dateisystem muss erstellt und anschießend auf das Gerät geladen werden. Der Inhalt des Dateisystems wird aus dem data Verzeichnis genommen.

### Erzeugen und Hochladen des Dateisystems mit platform.io ###

Wichtig: platformio.ini benötigt einen Eintrag in platformio.ini damit das korrekte Dateisystem-Format erstellt wird.

```
board_build.filesystem = littlefs
```

Ich habe die Projekt-Task von platform.io verwendet: 
"Upload Filesystem Image".

## WEB-Oberfläche ##

- Die WEB-Oberfläche ist in dem statischen Dateisystem des Geräts hinterlegt. Die aktuellen Einstellungen werden per REST-API ausgetauscht.
- Die Größe der Oberfläche ist an das Endgerät angepasst. 
- Alle Einstellungen werden über eine Schaltfläche in das Gerät übertragen. 
- Die Aktualisierung erfolgt ebenfalls über eine Schaltfläche.

## Anzeige ##

-  Der erste Bildschirm enthält den QR-Code mit der URL der WEB-Seite.
- Die Anzeige des Sattelitenempfangs wird mit einer grafischen Intensitätsanzeige gemacht. Grundlage ist die Anzahl empfagener Satteliten. 
-  Den Abstand zu der nächsten Ölung wird über einen Balken dargestellt. Der Balken wird während der Ölanzeige mit der Ölkanne überblendet.
-  Die gefahrenen Kilometer seit Rücksetzen des Tanks wurde ersetzt durch die Zeitanzeige.

## Öler ##

Als erfahrener Anwender eines Ölers bin ich der Meinung, dass ein Ölen aufgrund des Neustarts nicht notwendig ist. Es sollte jedoch sichergestellt werden Ölen nach einem Regen nicht zu "vergessen". Moped im Regen abgestellt. Bei Weiterfahrt mit schönem Wetter sollte die Kette nachgeölt werden.

## Debuggen ##

Trotz der Tatsache, dass der gps-oiler mit dem arduino framework implementiert ist kann der Debugger genutzt werden.

WICHTIG:

Installation des korrekten JTAG Treibers:
Hinweis: Habe viel Zeit damit verbracht auf ChatGPT zu hören das immter wieder behauptet hatte, man müsste darauf achten ein MI_01 bei der Hardware-ID zu finden. MI_02 wäre absolut falsch. => Das stimmt nicht. Debuggen mit MI_02 funktioniert bei mir.

In Powershell (admin) ausführen.
Lädt den Treiber für jtag interface.
Invoke-WebRequest 'https://dl.espressif.com/dl/idf-env/idf-env.exe' -OutFile .\idf-env.exe; .\idf-env.exe driver install --espressif

Ich habe anschließend den Treiber der jtag Schnittstelle aktualisiert. 
=> in dem Windows Device Manager mit verbundenem  esp c3.
=> Auswahl der Schnittstelle under Universal Seriell Bus Device. USB JTAG/serial debug unit auswählen.
=> Properties, Tab Driver öffnen. Dort Update Driver aktivieren und Option Search automatically for drivers.
   Der richtige Treiber hat: 
     Driver provider: libwdi
     Digital Signer:  Espressif Systems (Shanghei) Co. Ltd.

Debugger starten:

WICHTIG:

-  Wegen einer Schwäche der boot Reihenfolge mit dem arduino framework ist es wichtig den Debugger nach manuellem Upload exlizit zu starten. Das Funktioniert mit der Variante POI Debug (skip Pre-Debug). Falls man das nicht macht, greifen die Breakpoints meistens nicht. (Timing)
-  Breakpoints auf **setup** setzen funktioniert nicht, da in dem Binars viele Setup Funktionen enthalten sind. Da reichen die 3 verfügbaren Hardware-Breakpoints nicht aus. Deshalb in platform.ini folgende Angabe verwenden: debug_init_break = break src/main.cpp:xx (xx: Zeilennummer)
Leere Angabe ist auch gut, damit nicht initial setup genommen wird.
debug_init_break =

Hinweis:

Nach Starten der Debug-Sitzung hält das System an und zeigt im Call-Stack ??@0x40000000 Unknown Source und Status "Paused on exception".
Spätestens jetzt sollten Breakpoints gesetzt werden.
Mit (Continue) F5 kann das Programm fortgesetzt werden.

