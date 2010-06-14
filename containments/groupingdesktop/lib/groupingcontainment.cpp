/*
 *   Copyright 2009 by Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "groupingcontainment.h"

#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QAction>

#include <KDebug>
#include <KMenu>
#include <KIcon>

#include "abstractgroup.h"
#include "abstractgroup_p.h"
#include "handle.h"
#include "gridgroup.h"
#include "floatinggroup.h"

class GroupingContainmentPrivate
{
    public:
        GroupingContainmentPrivate(GroupingContainment *containment)
            : q(containment),
              interestingGroup(0),
              mainGroup(0),
              mainGroupId(0),
              layout(0)
        {
            newGroupAction = new QAction(i18n("Add a new group"), q);
            newGroupMenu = new KMenu(i18n("Add a new group"), 0);
            newGroupAction->setMenu(newGroupMenu);
            newGridGroup = new QAction(i18n("Add a new grid group"), q);
            newGridGroup->setData("grid");
            newFloatingGroup = new QAction(i18n("Add a new floating group"), q);
            newFloatingGroup->setData("floating");
            newGroupMenu->addAction(newGridGroup);
            newGroupMenu->addAction(newFloatingGroup);

            deleteGroupAction = new QAction(i18n("Remove this group"), q);
            deleteGroupAction->setIcon(KIcon("edit-delete"));
            deleteGroupAction->setVisible(false);

            separator = new QAction(q);
            separator->setSeparator(true);

            q->connect(newGroupMenu, SIGNAL(triggered(QAction *)), q, SLOT(newGroupClicked(QAction *)));
            q->connect(deleteGroupAction, SIGNAL(triggered()), q, SLOT(deleteGroup()));
        }

        ~GroupingContainmentPrivate()
        {}

        AbstractGroup *createGroup(const QString &plugin, const QPointF &pos, unsigned int id)
        {
            AbstractGroup *group = 0;
            if (plugin == "grid") {
                group = new GridGroup(q);
            } else if (plugin == "floating") {
                group = new FloatingGroup(q);
            }

            if (!group) {
                return 0;
            }

            if (groups.contains(group)) {
                delete group;
                return 0;
            }

            if (id == 0) {
                id = groups.count() + 1;
            }
            group->d->id = id;

            groups << group;

            q->addGroup(group, pos);

            return group;
        }

        void handleDisappeared(Handle *handle)
        {
            if (handles.contains(handle->widget())) {
                handles.remove(handle->widget());
                handle->detachWidget();
                if (q->scene()) {
                    q->scene()->removeItem(handle);
                }
                handle->deleteLater();
            }
        }

        void onGroupRemoved(AbstractGroup *group)
        {
            kDebug()<<"Removed group"<<group->id();

            groups.removeAll(group);
            group->removeEventFilter(q);

            if (handles.contains(group)) {
                Handle *handle = handles.value(group);
                handles.remove(group);
                delete handle;
            }

            emit q->groupRemoved(group);
        }

        void onAppletRemoved(Plasma::Applet *applet)
        {
            kDebug()<<"Removed applet"<<applet->id();

            applet->removeEventFilter(q);

            if (handles.contains(applet)) {
                Handle *handle = handles.value(applet);
                handles.remove(applet);
                delete handle;
            }
        }

        AbstractGroup *groupAt(const QPointF &pos, QGraphicsItem *uppermostItem = 0)
        {
            if (pos.isNull()) {
                return 0;
            }

            QList<QGraphicsItem *> items = q->scene()->items(q->mapToScene(pos),
                                                             Qt::IntersectsItemShape,
                                                             Qt::DescendingOrder);

            bool goOn;
            if (uppermostItem) {
                do {
                    goOn = items.first() != uppermostItem;
                    items.removeFirst();
                } while (goOn);
            }
// kDebug()<<items;
            for (int i = 0; i < items.size(); ++i) {
                AbstractGroup *group = qgraphicsitem_cast<AbstractGroup *>(items.at(i));
                if (group) {
                    return group;
                }
            }

            return 0;
        }

        void manageApplet(Plasma::Applet *applet, const QPointF &pos)
        {
            const int APPLET_Z_MINIMUM = 10000;
            if (applet->zValue() < APPLET_Z_MINIMUM) {
                applet->setZValue(applet->zValue() + APPLET_Z_MINIMUM); //FIXME: i don't like this at all
            }                                                           // but the groupss need to be
                                                                        //behind the applets.
            AbstractGroup *group = groupAt(pos);

            if (group) {
                group->addApplet(applet);

                applet->installSceneEventFilter(q);
            } else {
                applet->installEventFilter(q);
            }

            q->connect(applet, SIGNAL(appletDestroyed(Plasma::Applet*)), q, SLOT(onAppletRemoved(Plasma::Applet*)));
        }

        void manageGroup(AbstractGroup *subGroup, const QPointF &pos)
        {
            //FIXME i don't like this setPos's at all
            subGroup->setPos(q->geometry().bottomRight());
            AbstractGroup *group = groupAt(pos);
            subGroup->setPos(pos);

            if (group && (group != subGroup)) {
                group->addSubGroup(subGroup);
            }

            subGroup->installEventFilter(q);
        }

        void newGroupClicked(QAction *action)
        {
            kDebug()<<action->data();
            createGroup(action->data().toString(), lastClick, 0);
        }

        void deleteGroup()
        {
            int id = deleteGroupAction->data().toInt();

            foreach (AbstractGroup *group, groups) {
                if ((int)group->id() == id) {
                    group->destroy();

                    return;
                }
            }
        }

        void onAppletRemovedFromGroup(Plasma::Applet *applet, AbstractGroup *group)
        {
            Q_UNUSED(group)

            if (applet->parentItem() == q) {
                applet->installEventFilter(q);
            }
        }

        void onSubGroupRemovedFromGroup(AbstractGroup *subGroup, AbstractGroup *group)
        {
            Q_UNUSED(group)

            if (subGroup->parentItem() == q) {
                subGroup->installEventFilter(q);
            }
        }

        void onWidgetMoved(QGraphicsWidget *widget)
        {
            if (interestingGroup) {

//                 widget->removeEventFilter(q);

                Plasma::Applet *applet = qobject_cast<Plasma::Applet *>(widget);
                AbstractGroup *group = static_cast<AbstractGroup *>(widget);
                if (applet) {
                    interestingGroup->addApplet(applet);
                } else if (!group->isAncestorOf(interestingGroup)) {
                    interestingGroup->addSubGroup(group);
                }

                interestingGroup->layoutChild(widget, q->mapToItem(interestingGroup, widget->geometry().center()));

                Handle *h = handles.value(widget);
                if (h) {
                    h->deleteLater();
                    handles.remove(widget);
                }
                interestingGroup = 0;
            }
        }

        GroupingContainment *q;
        QList<AbstractGroup *> groups;
        AbstractGroup *interestingGroup;
        QMap<QGraphicsWidget *, Handle *> handles;
        QAction *newGroupAction;
        KMenu *newGroupMenu;
        QAction *newGridGroup;
        QAction *newFloatingGroup;
        QAction *separator;
        QAction *deleteGroupAction;
        QPointF lastClick;
        AbstractGroup *mainGroup;
        unsigned int mainGroupId;
        QGraphicsLinearLayout *layout;
        QString mainGroupPlugin;
};

GroupingContainment::GroupingContainment(QObject* parent, const QVariantList& args)
               : Containment(parent, args),
                 d(new GroupingContainmentPrivate(this))
{
    setContainmentType(Plasma::Containment::NoContainmentType);
}

GroupingContainment::~GroupingContainment()
{
    delete d;
}

void GroupingContainment::init()
{
    Plasma::Containment::init();

    connect(this, SIGNAL(appletAdded(Plasma::Applet*, QPointF)),
            this, SLOT(manageApplet(Plasma::Applet*, QPointF)));

//     addGroup("grid", QPointF(100,100), 0);
}

void GroupingContainment::constraintsEvent(Plasma::Constraints constraints)
{
    if ((constraints & Plasma::StartupCompletedConstraint) && !d->mainGroupPlugin.isEmpty() && !d->mainGroup) {
        AbstractGroup *group = addGroup(d->mainGroupPlugin);
        setMainGroup(group);
    }
}

AbstractGroup *GroupingContainment::addGroup(const QString &plugin, const QPointF &pos, int id)
{
    return d->createGroup(plugin, pos, id);
}

void GroupingContainment::addGroup(AbstractGroup *group, const QPointF &pos)
{
    if (!group) {
        return;
    }

    kDebug()<<"adding group"<<group->id();
    group->setImmutability(immutability());
    group->d->containment = this;
    connect(this, SIGNAL(immutabilityChanged(Plasma::ImmutabilityType)),
            group, SLOT(setImmutability(Plasma::ImmutabilityType)));
    connect(group, SIGNAL(groupDestroyed(AbstractGroup*)),
            this, SLOT(onGroupRemoved(AbstractGroup*)));
    connect(group, SIGNAL(appletRemovedFromGroup(Plasma::Applet*,AbstractGroup*)),
            this, SLOT(onAppletRemovedFromGroup(Plasma::Applet*,AbstractGroup*)));
    connect(group, SIGNAL(subGroupRemovedFromGroup(AbstractGroup*,AbstractGroup*)),
            this, SLOT(onSubGroupRemovedFromGroup(AbstractGroup*,AbstractGroup*)));
    connect(group, SIGNAL(configNeedsSaving()), this, SIGNAL(configNeedsSaving()));
    group->setPos(pos);
    d->manageGroup(group, pos);

    if (containmentType() == Plasma::Containment::DesktopContainment) {
        group->installSceneEventFilter(this);
    }

    emit groupAdded(group, pos);
    emit configNeedsSaving();
}

QList<AbstractGroup *> GroupingContainment::groups() const
{
    return d->groups;
}

QList<QAction *> GroupingContainment::contextualActions()
{
    QList<QAction *> list;
    list << d->newGroupAction << d->separator << d->deleteGroupAction;
    return list;
}

void GroupingContainment::useMainGroup(const QString &name)
{
    d->mainGroupPlugin = name;
}

void GroupingContainment::setMainGroup(AbstractGroup *group)
{
    d->mainGroup = group;
    if (!d->layout) {
        d->layout = new QGraphicsLinearLayout(this);
        d->layout->setContentsMargins(0, 0, 0, 0);
    }
    d->layout->addItem(group);
    group->setIsMainGroup(true);
}

AbstractGroup *GroupingContainment::mainGroup() const
{
    return d->mainGroup;
}

bool GroupingContainment::sceneEventFilter(QGraphicsItem* watched, QEvent* event)
{
    Plasma::Applet *applet = qgraphicsitem_cast<Plasma::Applet *>(watched);
    AbstractGroup *group = qgraphicsitem_cast<AbstractGroup *>(watched);

    if (group && !group->isMainGroup()) {
        if ((immutability() == Plasma::Mutable) && (group->immutability() == Plasma::Mutable)) {
            AbstractGroup *parentGroup = qgraphicsitem_cast<AbstractGroup *>(group->parentItem());

            if (!parentGroup || (parentGroup && parentGroup->groupType() != AbstractGroup::ConstrainedGroup)) {
                switch (event->type()) {
                    case QEvent::GraphicsSceneHoverEnter: {
                        QGraphicsSceneHoverEvent *he = static_cast<QGraphicsSceneHoverEvent *>(event);
                        if (d->handles.contains(group)) {
                            Handle *handle = d->handles.value(group);
                            if (handle) {
                                handle->setHoverPos(he->pos());
                            }
                        } else {
    //                         kDebug() << "generated group handle";
                            Handle *handle = new Handle(this, group, he->pos());
                            d->handles[group] = handle;
                            connect(handle, SIGNAL(disappearDone(Handle*)),
                                    this, SLOT(handleDisappeared(Handle*)));
                            connect(group, SIGNAL(geometryChanged()),
                                    handle, SLOT(widgetResized()));
                            connect(handle, SIGNAL(widgetMoved(QGraphicsWidget*)),
                                    this, SLOT(onWidgetMoved(QGraphicsWidget*)));
                        }
                    }
                    break;

                    case QEvent::GraphicsSceneHoverMove: {
                        QGraphicsSceneHoverEvent *he = static_cast<QGraphicsSceneHoverEvent *>(event);
                        if (d->handles.contains(group)) {
                            Handle *handle = d->handles.value(group);
                            if (handle) {
                                handle->setHoverPos(he->pos());
                            }
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
        }

        return false;
    }

    if (applet) {
        if ((immutability() == Plasma::Mutable) && (applet->immutability() == Plasma::Mutable)) {
            AbstractGroup *parentGroup = qgraphicsitem_cast<AbstractGroup *>(applet->parentItem());

            if (!parentGroup || (parentGroup && parentGroup->groupType() != AbstractGroup::ConstrainedGroup)) {
                switch (event->type()) {
                case QEvent::GraphicsSceneHoverMove: {
                        QGraphicsSceneHoverEvent *he = static_cast<QGraphicsSceneHoverEvent *>(event);
                        if (d->handles.contains(applet)) {
                            Handle *handle = d->handles.value(applet);
                            handle->setHoverPos(he->pos());
                        } else {
                            Handle *handle = new Handle(this, applet, he->pos());
                            d->handles[applet] = handle;
                            connect(handle, SIGNAL(disappearDone(Handle*)),
                                    this, SLOT(handleDisappeared(Handle*)));
                            connect(applet, SIGNAL(geometryChanged()),
                                    handle, SLOT(widgetResized()));
                            connect(handle, SIGNAL(widgetMoved(QGraphicsWidget*)),
                                    this, SLOT(onWidgetMoved(QGraphicsWidget*)));
                        }
                    }
                    break;

                case QEvent::GraphicsSceneHoverEnter: {
                        QGraphicsSceneHoverEvent *he = static_cast<QGraphicsSceneHoverEvent *>(event);
                        if (d->handles.contains(applet)) {
                            Handle *handle = d->handles.value(applet);
                            handle->setHoverPos(he->pos());
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        }

        return Plasma::Applet::sceneEventFilter(watched, event);
    }

    return Plasma::Containment::sceneEventFilter(watched, event);
}

bool GroupingContainment::eventFilter(QObject *obj, QEvent *event)
{
    AbstractGroup *group = qobject_cast<AbstractGroup *>(obj);
    Plasma::Applet *applet = qobject_cast<Plasma::Applet *>(obj);

    QGraphicsWidget *widget = 0;
    if (applet) {
        widget = applet;
    } else if (group) {
        widget = group;
    }
// kDebug()<<widget;
    if (widget) {
        switch (event->type()) {
            case QEvent::GraphicsSceneMousePress:
                if (group && !group->isMainGroup()) {
                    group->raise();
                }

                break;

            case QEvent::GraphicsSceneMove: {
                    AbstractGroup *parentGroup = d->groupAt(mapFromItem(widget, widget->contentsRect().center()), widget);

                    if (d->interestingGroup /*&& d->interestingGroup != parentGroup*/) {
                        d->interestingGroup->showDropZone(QPointF());
                        d->interestingGroup = 0;
                    }
                    if (parentGroup) {
                        QPointF c = widget->contentsRect().center();
                        c += mapFromScene(widget->scenePos());
                        QPointF pos = mapToItem(parentGroup, c);
                        kDebug()<<widget->contentsRect().center()<<widget->scenePos()<<pos;
                        if (pos.x() > 0 && pos.y() > 0) {
                            parentGroup->showDropZone(pos);
                            d->interestingGroup = parentGroup;
                        }
                    }
                }

                break;

            case QEvent::GraphicsSceneMouseRelease:
                d->onWidgetMoved(widget);

                break;

            default:
                break;
        }
    }

    return Plasma::Containment::eventFilter(obj, event);
}

void GroupingContainment::save(KConfigGroup &g) const
{
    KConfigGroup group = g;
    if (!group.isValid()) {
        group = config();
    }

    Plasma::Containment::save(group);

    if (d->mainGroup) {
        group.writeEntry("mainGroup", d->mainGroup->id());
    }
}

void GroupingContainment::saveContents(KConfigGroup &group) const
{
    Plasma::Containment::saveContents(group);

    KConfigGroup groupsConfig(&group, "Groups");
    foreach (AbstractGroup *group, d->groups) {
        KConfigGroup groupConfig(&groupsConfig, QString::number(group->id()));
        groupConfig.writeEntry("plugin", group->pluginName());
        QRectF rect = group->contentsRect();
        rect.translate(mapToItem(this, group->pos()));
        groupConfig.writeEntry("geometry", rect);
        group->save(groupConfig);
    }

    foreach (AbstractGroup *group, d->groups) {
        foreach (Plasma::Applet *applet, group->applets()) {
            KConfigGroup appletConfig = applet->config().parent();
            KConfigGroup groupConfig(&appletConfig, QString("GroupInformation"));
            groupConfig.writeEntry("Group", group->id());
            group->saveChildGroupInfo(applet, groupConfig);

            groupConfig.sync();
        }
        foreach (AbstractGroup *subGroup, group->subGroups()) {
            KConfigGroup subGroupConfig = subGroup->config().parent();
            KConfigGroup groupInfoConfig(&subGroupConfig, QString("GroupInformation"));
            groupInfoConfig.writeEntry("Group", group->id());
            group->saveChildGroupInfo(subGroup, groupInfoConfig);

            subGroupConfig.sync();
        }
    }
}

void GroupingContainment::restore(KConfigGroup &group)
{
    d->mainGroupId = group.readEntry("mainGroup", 0);

    Plasma::Containment::restore(group);
}

void GroupingContainment::restoreContents(KConfigGroup& group)
{
    Plasma::Containment::restoreContents(group);

    KConfigGroup groupsConfig(&group, "Groups");
    foreach (QString groupId, groupsConfig.groupList()) {
        int id = groupId.toInt();
        KConfigGroup groupConfig(&groupsConfig, groupId);
        QRectF geometry = groupConfig.readEntry("geometry", QRectF());
        QString plugin = groupConfig.readEntry("plugin", QString());

        AbstractGroup *group = d->createGroup(plugin, geometry.topLeft(), id);
        if (group) {
            group->resize(geometry.size());
            group->restore(groupConfig);
        }
    }

    if (d->mainGroupId != 0) {
        foreach (AbstractGroup *group, d->groups) {
            if (group->id() == d->mainGroupId) {
                setMainGroup(group);
            }
        }
    }

    //restore nested groups
    foreach (AbstractGroup *group, d->groups) {
        KConfigGroup groupConfig(&groupsConfig, QString::number(group->id()));
        KConfigGroup groupInfoConfig(&groupConfig, "GroupInformation");

        if (groupInfoConfig.isValid()) {
            int groupId = groupInfoConfig.readEntry("Group", -1);

            if (groupId != -1) {
                AbstractGroup *parentGroup = 0;
                foreach (AbstractGroup *g, d->groups) {
                    if ((int)g->id() == groupId) {
                        parentGroup = g;
                        break;
                    }
                }
                if (parentGroup) {
                    parentGroup->addSubGroup(group, false);
                    parentGroup->restoreChildGroupInfo(group, groupInfoConfig);
                }
            }
        }
    }

    KConfigGroup appletsConfig(&group, "Applets");
    foreach (Applet *applet, applets()) {
        KConfigGroup appletConfig(&appletsConfig, QString::number(applet->id()));
        KConfigGroup groupConfig(&appletConfig, "GroupInformation");

        if (groupConfig.isValid()) {
            int groupId = groupConfig.readEntry("Group", -1);

            if (groupId != -1) {
                AbstractGroup *group = 0;
                foreach (AbstractGroup *g, d->groups) {
                    if ((int)g->id() == groupId) {
                        group = g;
                        break;
                    }
                }
                if (group) {
                    group->addApplet(applet, false);
                    group->restoreChildGroupInfo(applet, groupConfig);
                }
            }
        }
    }
}

void GroupingContainment::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    d->deleteGroupAction->setVisible(false);
    d->lastClick = event->pos();

    Plasma::Containment::mousePressEvent(event);
}

void GroupingContainment::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    AbstractGroup *group = d->groupAt(event->pos());

    if (group && (immutability() == Plasma::Mutable) && (group->immutability() == Plasma::Mutable) && !group->isMainGroup()) {
        d->deleteGroupAction->setVisible(true);
        d->deleteGroupAction->setData(group->id());
        d->lastClick = event->pos();
        showContextMenu(event->pos(), event->screenPos());
        return;
    }

    event->ignore();

    Plasma::Containment::contextMenuEvent(event);
}

#include "groupingcontainment.moc"