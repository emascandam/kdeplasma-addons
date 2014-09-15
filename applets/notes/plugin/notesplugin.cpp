/*
    Copyright (C) 2014 David Edmundson <davidedmundson@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "notesplugin.h"
#include "documenthandler.h"
#include "notemanager.h"
#include "note.h"

#include <QtQml>

void NotesPlugin::registerTypes (const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.private.notes"));
    qmlRegisterType<DocumentHandler>(uri, 0, 1, "DocumentHandler");
    qmlRegisterType<NoteManager>(uri, 0, 1, "NoteManager");
    qmlRegisterUncreatableType<Note>(uri, 0, 1, "Note", "Create through NoteManager");

}
