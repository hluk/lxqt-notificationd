/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "notificationlayout.h"
#include "notificationarea.h"
#include <QtDebug>
#include <QtGui/QBrush>


NotificationLayout::NotificationLayout(QWidget *parent) :
    QWidget(parent)
{
    setObjectName("NotificationLayout");

    // Hack to ensure the fully transparent background
    QPalette palette;
    palette.setBrush(QPalette::Base, Qt::NoBrush);
    setPalette(palette);
    // Required to display wallpaper
    setAttribute(Qt::WA_TranslucentBackground);

    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(NOTIFICATION_SPACING);
    m_layout->setMargin(0);
    setLayout(m_layout);
}

void NotificationLayout::addNotification(uint id, const QString &application,
                                        const QString &summary, const QString &body,
                                        const QString &icon, int timeout,
                                        const QStringList& actions, const QVariantMap& hints)
{
//    qDebug() << "NotificationLayout::addNotification" << id << application << summary << body << icon << timeout;
    if (m_notifications.contains(id))
    {
        // TODO/FIXME: it can be deleted by timer in this block. Locking?
        Notification *n = m_notifications[id];
        n->setValues(application, summary, body, icon, timeout, actions, hints);
    }
    else
    {
        Notification *n = new Notification(application, summary, body, icon, timeout, actions, hints, this);
        connect(n, SIGNAL(timeout()), this, SLOT(removeNotificationTimeout()));
        connect(n, SIGNAL(userCanceled()), this, SLOT(removeNotificationUser()));
        m_notifications[id] = n;
        m_layout->addWidget(n);
        n->show();
    }

    checkHeight();
    emit notificationAvailable();
}

void NotificationLayout::removeNotificationTimeout()
{
    Notification *n = qobject_cast<Notification*>(sender());
    if (!n)
    {
        qDebug() << "Oooook! TIMEOUT Expecting instance of notification, got:" << sender();
        return;
    }

    removeNotification(m_notifications.key(n), 1);
}

void NotificationLayout::removeNotificationUser()
{
    Notification *n = qobject_cast<Notification*>(sender());
    if (!n)
    {
        qDebug() << "Oooook! USERCANCEL Expecting instance of notification, got:" << sender();
        return;
    }

    removeNotification(m_notifications.key(n), 2);
}

void NotificationLayout::removeNotification(uint key, uint reason)
{
    Notification *n = m_notifications.take(key);
    if (!n)
    {
        qDebug() << "Oooook! Expecting instance of notification, got:" << key;
        return;
    }

    int ix = m_layout->indexOf(n);
    if (ix == -1)
    {
        qDebug() << "Qooook! Widget not in layout. Impossible!" << n;
        return;
    }

    delete m_layout->takeAt(ix);
    n->deleteLater();
    emit notificationClosed(key, reason);

    if (m_notifications.count() == 0)
        emit allNotificationsClosed();

    checkHeight();
}

void NotificationLayout::checkHeight()
{
    int h = 0;
    QHashIterator<uint, Notification*> it(m_notifications);
    while (it.hasNext())
    {
        it.next();
        h +=it.value()->height() + NOTIFICATION_SPACING;
    }

    setMinimumSize(NOTIFICATION_WIDTH, h);
    setMaximumSize(NOTIFICATION_WIDTH, h);
    emit heightChanged(h);
}

