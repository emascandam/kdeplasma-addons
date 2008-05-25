/***************************************************************************
 *   Copyright (C) 2008 by Montel Laurent <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#ifndef _KONQPROFILESAPPLET_H_
#define _KONQPROFILESAPPLET_H_

#include <plasma/applet.h>
#include <plasma/dialog.h>

class QTreeView;
class QGraphicsProxyWidget;
class QStandardItemModel;
class QModelIndex;
class QGraphicsLinearLayout;

namespace Plasma
{
    class Icon;
}

class KonqProfilesApplet : public Plasma::Applet
{
    Q_OBJECT
public:
    KonqProfilesApplet(QObject *parent, const QVariantList &args);
    ~KonqProfilesApplet();

    void init();

    enum SpecificRoles {
        ProfilesName = Qt::UserRole+1
    };

protected slots:
    void slotOpenMenu();
    void slotOnItemClicked(const QModelIndex &index);
    void slotUpdateKonqProfiles();

protected:
    void initSessionFiles();
    void constraintsUpdated(Plasma::Constraints constraints);

private:
    Plasma::Dialog *m_widget;
    QTreeView *m_listView;
    Plasma::Icon *m_icon;
    QGraphicsLinearLayout *m_layout;
    QGraphicsProxyWidget * m_proxy;
    QStandardItemModel *m_konqModel;
    bool closePopup;
};

K_EXPORT_PLASMA_APPLET(konqprofilesapplet, KonqProfilesApplet )

#endif
