/*********************************************************************/
/* Copyright (c) 2016, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#include "WebbrowserContent.h"

#include "serialization/utils.h"

BOOST_CLASS_EXPORT_IMPLEMENT(WebbrowserContent)

IMPLEMENT_SERIALIZE_FOR_XML(WebbrowserContent)

namespace
{
const bool showKeyboardAction = false;
const QString title{"Web Browser"};
}

WebbrowserContent::WebbrowserContent(const QString& uri)
    : PixelStreamContent(uri, showKeyboardAction)
{
}

WebbrowserContent::WebbrowserContent()
    : PixelStreamContent(showKeyboardAction)
{
}

CONTENT_TYPE WebbrowserContent::getType() const
{
    return CONTENT_TYPE_WEBBROWSER;
}

QString WebbrowserContent::getTitle() const
{
    if (_pageTitle.isEmpty())
        return title;
    return QString("%1 - %2").arg(title, _pageTitle);
}

bool WebbrowserContent::hasFixedAspectRatio() const
{
    return false;
}

int WebbrowserContent::getPage() const
{
    return _history.currentItemIndex();
}

int WebbrowserContent::getPageCount() const
{
    return _history.items().size();
}

QString WebbrowserContent::getUrl() const
{
    return _history.currentItem();
}

void WebbrowserContent::setUrl(const QString& url)
{
    _history = WebbrowserHistory({url}, 0);

    emit pageChanged();
    emit pageCountChanged();
    emit modified();
}

void WebbrowserContent::parseData(const QByteArray data)
{
    serialization::fromBinary(data, _history, _pageTitle);

    emit pageChanged();
    emit pageCountChanged();
    emit titleChanged(getTitle());
    emit modified();
}

QByteArray WebbrowserContent::serializeData(const WebbrowserHistory& history,
                                            const QString& pageTitle)
{
    const auto string = serialization::toBinary(history, pageTitle);
    return QByteArray::fromStdString(string);
}
