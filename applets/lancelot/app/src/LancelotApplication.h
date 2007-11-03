/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser/Library General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser/Library General Public License for more details
 *
 *   You should have received a copy of the GNU Lesser/Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LANCELOTAPPLICATION_H_
#define LANCELOTAPPLICATION_H_

#include <kuniqueapplication.h>
#include <QTimer>
#include <QSet>

class LancelotWindow;

class LancelotApplication: public KUniqueApplication
{
    Q_OBJECT
    //CLASSINFO("D-Bus Interface", "org.kde.lancelot")
public:
	static int main(int argc, char **argv);
	static LancelotApplication * application();

	bool event(QEvent * e);

public Q_SLOTS:
    bool show();
    bool hide(bool immediate = false);
    bool showItem(QString name);
    void search(const QString & string);

    int addClient();
    bool removeClient(int id);

protected:
    static LancelotApplication * m_application;
    LancelotWindow * window;

private:
    LancelotApplication(int argc, char **argv);
    LancelotApplication(Display * display,
        Qt::HANDLE visual = 0,
        Qt::HANDLE colormap = 0,
        bool configUnique = false);
    void init();

    virtual ~LancelotApplication();

    int m_clientsNumber;
    int m_lastID;
    QSet<int> m_clients;

};

#endif /*LANCELOTAPPLICATION_H_*/
