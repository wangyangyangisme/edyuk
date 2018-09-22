#!/bin/sh

# update ts files

# core lib
lupdate src/lib/lib.pro

# plugins (do that AFTER building Edyuk or some translations will be missing)
lupdate src/plugins/assistant -ts translations/assistant_*.ts
lupdate src/plugins/default -ts translations/default_*.ts
lupdate src/plugins/designer -ts translations/designer_*.ts
lupdate src/plugins/gdb -ts translations/gdb_*.ts

